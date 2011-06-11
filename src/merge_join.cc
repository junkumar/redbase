#include "merge_join.h"

MergeJoin::MergeJoin(Iterator*    lhsIt,      // access for left i/p to join -R
                     Iterator*    rhsIt,      // access for right i/p to join -S
                     RC& status,
                     int nJoinConds,
                     int equiCond,        // the join condition that can be used as
                     // the equality consideration.
                     const Condition joinConds_[])
  :NestedLoopJoin(lhsIt, rhsIt, status, nJoinConds, joinConds_),
   equiPos(equiCond), firstOpen(true), sameValueCross(false),
   lastCpit(false),  pcurrTuple(NULL), potherTuple(NULL)
{
  Condition* joinConds = new Condition[nJoinConds];
  for (int i = 0; i < nJoinConds; i++) {
    joinConds[i] = joinConds_[i];
  }

  // need at least one condition for the equijoin
  assert(nJoinConds > 0);
  assert(equiCond >= 0 && equiCond < nJoinConds &&
         joinConds[equiCond].op == EQ_OP);

  if(!lhsIt->IsSorted() || !rhsIt->IsSorted()) {
    status = -1;
    return;
  }
      
  if(lhsIt->IsDesc() != rhsIt->IsDesc()) {
    status = -1;
    return;
  } else {
    desc = lhsIt->IsDesc();
  }

  if((nJoinConds > 0) &&
     (equiCond >= 0 && equiCond < nJoinConds &&
      joinConds[equiCond].op == EQ_OP)) {
    explain.str("");
    explain << "MergeJoin\n";
    if(nJoinConds > 0) {
      explain << "   nJoinConds = " << nJoinConds << "\n";
      for (int i = 0; i < nJoinConds; i++)
        explain << "   joinConds[" << i << "]:" << joinConds[i] << "\n";
    }

    equi = joinConds[equiCond];
    // remove equi cond from conds so we can reuse NLJ::EvalJoin
    for (int i = 0; i < nJoinConds; i++) {
      if(i > equiCond) {
        joinConds[i-1] = joinConds[i];
      }
    }
    nJoinConds--;

    // populate sort rel, attr using equi
    bSorted = true;
    sortRel = string(equi.lhsAttr.relName);
    sortAttr = string(equi.lhsAttr.attrName);

    curr = lhsIt;
    other = rhsIt;
  } else {
    status = SM_BADTABLE;
  }
  delete [] joinConds;
}


RC MergeJoin::GetNext(Tuple &t)
{
  RC invalid = IsValid(); if(invalid) return invalid;
  if(!bIterOpen)
    return QL_FNOTOPEN;

  if(firstOpen) {
    pcurrTuple = new Tuple(left);
    // cerr << "pcurr is " << *pcurrTuple << endl;
    potherTuple = new Tuple(other->GetTuple());
    firstOpen = false;
  }


  RC left_rc = -1;
  bool joined = false;

  if(!sameValueCross) {

    while(!joined) {

      // use the stored values if the last returned record was for the last
      // cross product.
      if(!lastCpit) {
        left_rc = other->GetNext(*potherTuple);
        if(left_rc == other->Eof())
          break;
      } else {
        lastCpit = false;
      }

      if(potherTuple == NULL ||
         pcurrTuple == NULL)
        break; // also EOF

      // check equi cond and advance curr
      
      Predicate p(lKeys[equiPos].attrType,
                  lKeys[equiPos].attrLength,
                  lKeys[equiPos].offset,
                  equi.op,
                  NULL,
                  NO_HINT);

      // check for join
      void * b = NULL;
      const char * abuf;
      Tuple* saved = NULL;
      if(curr == lhsIt) {
        potherTuple->Get(rKeys[equiPos].attrName, b);
        pcurrTuple->GetData(abuf);
        saved = new Tuple(*pcurrTuple);
      } else {
        pcurrTuple->Get(rKeys[equiPos].attrName, b);
        potherTuple->GetData(abuf);
        saved = new Tuple(*potherTuple);
      }

      // if(curr == lhsIt) {
      //   cout << *pcurrTuple << " - " << *potherTuple << endl;
      // } else {
      //   cout << *potherTuple << " - " << *pcurrTuple << endl;
      // }

      bool recordIn;
      if(p.eval(abuf, (char*)b, equi.op)) {
        recordIn = true;
        // cout << "merge match - curr " << *pcurrTuple
        //      << " other " << *potherTuple << endl;

        // check for other matching records - get cross product using nested
        // loop before returning to normal sort-merge
        vector<Tuple> lvec;
        vector<Tuple> rvec;

        if(curr == lhsIt) {
          lvec.push_back(*pcurrTuple);
          rvec.push_back(*potherTuple);
        } else {
          lvec.push_back(*potherTuple);
          rvec.push_back(*pcurrTuple);
        }

        while(1) {
          Tuple l = lhsIt->GetTuple();
          RC rc = lhsIt->GetNext(l);
          if(rc == lhsIt->Eof()) {
            pcurrTuple = NULL;
            break;
          }
          const char * aabuf;
          saved->GetData(aabuf);
          void* bb = NULL;
          l.Get(lKeys[equiPos].attrName, bb);
          if(p.eval(aabuf, (char*)bb, EQ_OP)) {
            // cerr << "l pushback" << l << endl;
            lvec.push_back(l);
          } else {
            delete pcurrTuple;
            pcurrTuple = new Tuple(l);
            break;
          }
        }

        while(1) {
          Tuple r = rhsIt->GetTuple();
          RC rc = rhsIt->GetNext(r);
          if(rc == rhsIt->Eof()) {
            potherTuple = NULL;
            break;
          }
          const char * aabuf;
          saved->GetData(aabuf);
          void* bb = NULL;
          r.Get(rKeys[equiPos].attrName, bb);
          if(p.eval(aabuf, (char*)bb, EQ_OP)) {
            // cerr << "r pushback" << r << endl;
            rvec.push_back(r);
          } else {
            delete potherTuple;
            potherTuple = new Tuple(r);
            break;
          }
        }

        curr = lhsIt;
        other = rhsIt;

        delete saved;

        vector<Tuple>::iterator it, rit;
        for(it = lvec.begin(); it != lvec.end(); it++)
          for(rit = rvec.begin(); rit != rvec.end(); rit++) {
            Tuple tup = GetTuple();
            // check all other conds and decide on output
            // cerr << "considering cp " << *it << " - " << *rit << endl;
            bool evaled = false;
            EvalJoin(tup, evaled, &(*it), &(*rit));
            if(evaled) {
              // cerr << "cpvec pushing " << tup << endl;
              cpvec.push_back(tup);
            }
          }
        cpit = cpvec.begin();
        if(cpit != cpvec.end()) {
          joined = true;
        }
        /* if(pcurrTuple != NULL && potherTuple != NULL) */
        /* cerr << "----------------cp set computed - curr "  */
        /*      << *pcurrTuple  */
        /*      << " other " << *potherTuple << endl; */

        sameValueCross = true;
      } else { // no match - must advance one of the iterators
        recordIn = false;
        sameValueCross = false;
        
        CompOp op = LT_OP;
        if(desc) op = GT_OP;

        // comments that follow are for ascending merge
        if(p.eval(abuf, (char*)b, op)) {
          // a < b
          if(curr == lhsIt) { // curr < other - switch
            // cerr << "switch" << endl;
            curr = rhsIt;
            other = lhsIt;
            delete pcurrTuple;
            pcurrTuple = new Tuple(*potherTuple);
            // cerr << "after switch pcurr " << *pcurrTuple << endl;
            delete potherTuple;
            potherTuple = new Tuple(other->GetTuple());
          } else { // curr > other - no switching
          }

        } else {

          // a > b
          if(curr == lhsIt) { // curr > other - no switch
          } else { // curr < other -  switching
            // cerr << "switch" << endl;
            curr = lhsIt;
            other = rhsIt;
            delete pcurrTuple;
            pcurrTuple = new Tuple(*potherTuple);
            delete potherTuple;
            potherTuple = new Tuple(other->GetTuple());
          }

        }
        continue; // get next record for comparison

      }
    } // while not joined/eof
  } // if !sameValueCross

  if(sameValueCross && (cpit != cpvec.end())) {
    char * buf;
    t.GetData(buf);
    memset(buf, 0, TupleLength());

    char *itbuf;
    cpit->GetData(itbuf);
    memcpy(buf, itbuf, TupleLength());
     
    // cerr << t << " [cp]" << endl;
 
    cpit++;
    if(cpit == cpvec.end()) {
      cpvec.clear();
      cpit = cpvec.end();
      sameValueCross = false;
      lastCpit = true; // reuse potherTuple and pcurrTuple for tuples
    }
    return 0;
  }
    
  if(sameValueCross && (cpit == cpvec.end())) {
    // some matches for join cond, but other conds failed.
    cpvec.clear();
    cpit = cpvec.end();
    sameValueCross = false;
    lastCpit = true; // reuse potherTuple and pcurrTuple for tuples
  }

  return QL_EOF;
}
  
