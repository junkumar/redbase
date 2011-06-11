//
// File:        nested_loop_join.cc
//

#include "nested_loop_join.h"
#include "ql_error.h"
#include "sm.h"
#include <algorithm>
#include "predicate.h"
#include "parser.h"

using namespace std;

NestedLoopJoin::
NestedLoopJoin(Iterator *    lhsIt_,      // access for left i/p to join -R
               Iterator *    rhsIt_,      // access for right i/p to join -S
               RC& status,
               int nOutFilters,
               const Condition outFilters[]
               //FldSpec  * proj_list,
               // int        n_out_flds,
               )
  :lhsIt(lhsIt_), rhsIt(rhsIt_),
   left(lhsIt->GetAttrCount(), lhsIt->TupleLength()),
   right(rhsIt->GetAttrCount(), rhsIt->TupleLength()),
   nOFilters(nOutFilters), oFilters(NULL)
{
  if(lhsIt == NULL || rhsIt == NULL) {
    status = SM_NOSUCHTABLE;
    return;
  }

  attrCount = lhsIt->GetAttrCount() + rhsIt->GetAttrCount();
  assert(attrCount > 0);

  attrs = new DataAttrInfo[attrCount];
  
  std::copy(lhsIt->GetAttr(), lhsIt->GetAttr() + lhsIt->GetAttrCount(),
            attrs);

  DataAttrInfo * rattrs = rhsIt->GetAttr();
  for(int i = 0, j = lhsIt->GetAttrCount(); i < rhsIt->GetAttrCount(); i++) {
    attrs[j] = rattrs[i];
    attrs[j].offset += lhsIt->TupleLength();
    j++;
  }
  
  DataAttrInfo * lattrs = lhsIt->GetAttr();

  char * buf = new char[TupleLength()];
  memset(buf, 0, TupleLength());
  left.Set(buf);
  left.SetAttr(lhsIt->GetAttr());
  right.Set(buf);
  right.SetAttr(rhsIt->GetAttr());
  delete [] buf;

  if(nOFilters > 0) {
    oFilters = new Condition[nOFilters];
    lKeys = new DataAttrInfo[nOFilters];
    rKeys = new DataAttrInfo[nOFilters];
  }

  for(int i = 0; i < nOFilters; i++) {
    oFilters[i] = outFilters[i]; // shallow copy
    
    // cerr << oFilters[i] << endl;
    
    bool lfound = false;
    bool rfound = false;

    for(int k = 0; k < lhsIt->GetAttrCount(); k++) {
      // cerr << "lhs attr for " << k << "- " << lattrs[k].attrName << endl;
      // cerr << "cond attr for " << k << "- " << oFilters[i].lhsAttr.attrName << endl;
      if(strcmp(lattrs[k].attrName, oFilters[i].lhsAttr.attrName) == 0) {
        lKeys[i] = lattrs[k];
        lfound = true;
        continue;
      }
    }

    for(int k = 0; k < rhsIt->GetAttrCount(); k++) {
      // cerr << "rhs attr for " << k << "- " << rattrs[k].attrName << endl;
      // cerr << "cond attr for " << k << "- " << oFilters[i].rhsAttr.attrName << endl;
      if(strcmp(rattrs[k].attrName, oFilters[i].rhsAttr.attrName) == 0) {
        rKeys[i] = rattrs[k];
        rfound = true;
        continue;
      }
    }

    if(!lfound || !rfound) { // reverse pair and try
      lfound = false;
      rfound = false;

      for(int k = 0; k < lhsIt->GetAttrCount(); k++) {
        if(strcmp(lattrs[k].attrName, oFilters[i].rhsAttr.attrName) == 0) {
          lKeys[i] = lattrs[k];
          lfound = true;
          continue;
        }
      }

      for(int k = 0; k < rhsIt->GetAttrCount(); k++) {
        if(strcmp(rattrs[k].attrName, oFilters[i].lhsAttr.attrName) == 0) {
          rKeys[i] = rattrs[k];
          rfound = true;
          continue;
        }
      }
    }


    if(!lfound || !rfound) {
      if(!lfound)
        cerr << "bad lfound" << endl;
      status = QL_BADJOINKEY;
      return;
    }

    if(rKeys[i].offset == -1 || lKeys[i].offset == -1) {
      status = QL_BADJOINKEY;
      return;
    }
    
    // cerr << "right " << rKeys[i].offset << endl;
    // cerr << "left  " << lKeys[i].offset << endl;
    // cerr << "right " << rKeys[i].attrType << endl;
    // cerr << "left  " << lKeys[i].attrType << endl;
    // cerr << "right relName " << rKeys[i].relName << endl;
    // cerr << "left  relName " << lKeys[i].relName << endl;

    // cerr << "right " << rKeys[i].attrName << " " << outFilters[i].rhsAttr.attrName
    //      << endl;
    // cerr << "left  " << lKeys[i].attrName << " " << outFilters[i].lhsAttr.attrName
    //      << endl;

    if(rKeys[i].attrType != lKeys[i].attrType) {
      status = QL_JOINKEYTYPEMISMATCH;
      return;
    }
  }

  // NLJ get the same sort order as outer child - lhs
  if(lhsIt->IsSorted()) {
    bSorted = true;
    desc = lhsIt->IsDesc();
    sortRel = lhsIt->GetSortRel();
    sortAttr = lhsIt->GetSortAttr();
  }

  explain << "NestedLoopJoin\n";
  if(nOFilters > 0) {
    explain << "   nJoinConds = " << nOFilters << "\n";
    for (int i = 0; i < nOutFilters; i++)
      explain << "   joinConds[" << i << "]:" << oFilters[i] << "\n";
  }

  status = 0;
}

string NestedLoopJoin::Explain() {
  stringstream dyn;
  dyn << indent << explain.str();
  lhsIt->SetIndent(indent + "-----");
  dyn << lhsIt->Explain();
  rhsIt->SetIndent(indent + "-----");
  dyn << rhsIt->Explain();
  return dyn.str();
}

RC NestedLoopJoin::IsValid()
{
  return (attrCount != -1 && attrs != NULL) ? 0 : SM_BADTABLE;
}

NestedLoopJoin::~NestedLoopJoin()
{
  delete lhsIt;
  delete rhsIt;
  delete [] attrs;
  if(nOFilters != 0) {
    delete [] oFilters;
    delete [] rKeys;
    delete [] lKeys;
  }
}

RC NestedLoopJoin::Open()
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(bIterOpen)
    return QL_ALREADYOPEN;

  RC rc = lhsIt->Open();
  if(rc != 0) return rc;
  rc = rhsIt->Open();
  if(rc != 0) return rc;

  lhsIt->GetNext(left);
  bIterOpen = true;
  return 0;
}

RC NestedLoopJoin::Close()
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(!bIterOpen)
    return QL_FNOTOPEN;

  RC rc = lhsIt->Close();
  if(rc != 0) return rc;
  rc = rhsIt->Close();
  // may already be closed - so no need to check for errors

  char * buf = new char[TupleLength()];
  memset(buf, 0, TupleLength());
  left.Set(buf);
  right.Set(buf);
  delete [] buf;

  bIterOpen = false;
  return 0;
}

RC NestedLoopJoin::GetNext(Tuple &t)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(!bIterOpen)
    return QL_FNOTOPEN;
  
  bool joined = false;
  RC rc;

  while(!joined) {
    rc = rhsIt->GetNext(right);
    if (rc == rhsIt->Eof()) { // end of rhs - start again
      if((rc = rhsIt->Close()))
        return rc;
      rc = lhsIt->GetNext(left);
      if (rc == lhsIt->Eof()) { // end of both - exit
        return QL_EOF;
      }
      // cerr << "NestedLoopJoin::GetNext() leftvalue " << left << endl;
      rc = ReopenIfIndexJoin(left);
      rc = rhsIt->Open();
      rc = rhsIt->GetNext(right);
    }

    EvalJoin(t, joined, &left, &right);
  } // while

  return rc;
}

void NestedLoopJoin::EvalJoin(Tuple &t, bool& joined, Tuple* l, Tuple* r) {
  bool recordIn = true;
  for (int i = 0; i < nOFilters; i++) {
    Condition cond = oFilters[i];
    DataAttrInfo condAttr;

    Predicate p(lKeys[i].attrType,
                lKeys[i].attrLength,
                lKeys[i].offset,
                cond.op,
                NULL,
                NO_HINT);

    // check for join
    void * b = NULL;
    r->Get(rKeys[i].attrName, b);

    const char * abuf;
    l->GetData(abuf);
    // cerr << "EvalJoin():" << *l << " - " << *r << endl;

    if(p.eval(abuf, (char*)b, cond.op)) {
      recordIn = true;
    } else {
      recordIn = false;
      break;
    }
  } // for each filter

  if(recordIn) {
    char * buf;
    t.GetData(buf);
    memset(buf, 0, TupleLength());

    char *lbuf;
    l->GetData(lbuf);
    memcpy(buf, lbuf, l->GetLength());

    char *rbuf;
    r->GetData(rbuf);

    memcpy(buf + l->GetLength(), 
           rbuf,
           r->GetLength());

    joined = true;
  }
}
