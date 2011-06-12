//
// File:        Agg.h
//

#ifndef AGG_H
#define AGG_H

#include "redbase.h"
#include "iterator.h"
#include "sm.h"
#include "ql_error.h"
#include <list>
#include "predicate.h"

using namespace std;

// Single key, single pass agg operator. Uses memory directly.
class Agg: public Iterator {
 public:
   Agg(Iterator *  lhsIt,
       RelAttr     groupAttr,
       int         nSelAttrs,
       const AggRelAttr  selAttrs[],
       RC& status)
     :lhsIt(lhsIt)
    {
      if(lhsIt == NULL || 
         nSelAttrs <= 0 ||
         groupAttr.attrName == NULL ||
         !lhsIt->IsSorted() || 
         lhsIt->GetSortRel() != string(groupAttr.relName) ||
         lhsIt->GetSortAttr() != string(groupAttr.attrName)
         ) {
        status = SM_NOSUCHTABLE;
        return;
      }

      attrCount = nSelAttrs;
      attrs = new DataAttrInfo[attrCount];
      lattrs = new DataAttrInfo[attrCount];
      aggs = new AggFun[attrCount];
      attrCmps = new TupleCmp[attrCount];

      DataAttrInfo * itattrs = lhsIt->GetAttr();
      int offsetsofar = 0;

      for(int j = 0; j < attrCount; j++) {
        for(int i = 0; i < lhsIt->GetAttrCount(); i++) {
          if(strcmp(selAttrs[j].relName, itattrs[i].relName) == 0 &&
             strcmp(selAttrs[j].attrName, itattrs[i].attrName) == 0) {
            lattrs[j] = itattrs[i];
            attrs[j] = itattrs[i];
            attrs[j].func = selAttrs[j].func;
            if(attrs[j].func == COUNT_F)
              attrs[j].attrType = INT;
            attrs[j].offset = offsetsofar;
            offsetsofar += itattrs[i].attrLength;
            attrCmps[j] = TupleCmp(attrs[j].attrType, attrs[j].attrLength, attrs[j].offset, GT_OP);
            break;
          }
        }
        aggs[j] = selAttrs[j].func;
      }

      DataAttrInfo gattr;
      for(int i = 0; i < lhsIt->GetAttrCount(); i++) {
        if(strcmp(groupAttr.relName, itattrs[i].relName) == 0 &&
           strcmp(groupAttr.attrName, itattrs[i].attrName) == 0) {
          gattr = itattrs[i];
          break;
        }
      }

      cmp = TupleCmp(gattr.attrType, gattr.attrLength, gattr.offset, EQ_OP);

      // agg leaves sort order of child intact
      if(lhsIt->IsSorted()) { // always true for now
        bSorted = true;
        desc = lhsIt->IsDesc();
        sortRel = lhsIt->GetSortRel();
        sortAttr = lhsIt->GetSortAttr();
      }


      it = set.begin();

      explain << "Agg\n";
      explain << "   AggKey=" << groupAttr << endl;
      for(int i = 0; i < attrCount; i++) {
        explain << "   AggFun[" << i << "]=" << aggs[i] << endl; 
      }
      status = 0;
    }
    
  string Explain() {
    stringstream dyn;
    dyn << indent << explain.str();
    lhsIt->SetIndent(indent + "-----");
    dyn << lhsIt->Explain();
    return dyn.str();
  }

  virtual ~Agg() 
    {
      delete lhsIt;
      delete [] attrs;
      delete [] lattrs;
      delete [] aggs;
      delete [] attrCmps;
    }

  virtual RC Open() {
    if(bIterOpen)
      return RM_HANDLEOPEN;

    RC rc = lhsIt->Open();
    if(rc != 0 ) return rc;

    // init
    Tuple prev = lhsIt->GetTuple();
    bool firstTime = true;

    Tuple lt = lhsIt->GetTuple();
    Tuple t = this->GetTuple();

    char * buf;
    t.GetData(buf);
    char *lbuf;
    lt.GetData(lbuf);

    while(lhsIt->GetNext(lt) != lhsIt->Eof()) {

      // cout << "ltuple " << lt << endl;
      // same agg key
      if(firstTime || cmp(prev, lt)) {
        
        // cout << "===same key " << lt << " - " << t << endl;
        for(int i = 0; i < attrCount; i++) {
          if(aggs[i] == NO_F) {
            memcpy(buf + attrs[i].offset,
                   lbuf + lattrs[i].offset,
                   attrs[i].attrLength);
            /* cout << "A non-agg for attr " << i   */
            /*      << " offset " << attrs[i].offset << endl; */
          }
          if(aggs[i] == MAX_F) {
            if(attrCmps[i](prev, t)) {
              memcpy(buf + attrs[i].offset,
                     lbuf + lattrs[i].offset,
                     attrs[i].attrLength);
            }
          }
          if(aggs[i] == MIN_F) {
            if(attrCmps[i](t, prev)) {
              memcpy(buf + attrs[i].offset,
                     lbuf + lattrs[i].offset,
                     attrs[i].attrLength);
            }
            /* cout << "A min-agg for attr " << i   */
            /*      << " offset " << attrs[i].offset << endl; */
          }
          if(aggs[i] == COUNT_F) {
            int count = *((int*)(buf + attrs[i].offset));
            if(firstTime)
              count = 0;
            /* cout << "firstTime " << firstTime << endl; */
            /* cout << "A count agg for attr " << i << " count " << count   */
            /*      << " offset " << attrs[i].offset << endl; */
            ++(count);
            memcpy(buf + attrs[i].offset,
                   &count,
                   attrs[i].attrLength);
            /* cout << "A count agg for attr " << i << " count " << *((int*)(buf + attrs[i].offset)) << endl; */
          }
        }

        firstTime = false;
      } else {
        // new agg key
        // cout << "=== new agg key " << lt << endl;
        set.push_back(t);
        for(int i = 0; i < attrCount; i++) {
          if(aggs[i] == NO_F) {
            memcpy(buf + attrs[i].offset,
                   lbuf + lattrs[i].offset,
                   attrs[i].attrLength);
          }
          if(aggs[i] == MAX_F) {
            memcpy(buf + attrs[i].offset,
                   lbuf + lattrs[i].offset,
                   attrs[i].attrLength);
          }
          if(aggs[i] == MIN_F) {
            memcpy(buf + attrs[i].offset,
                   lbuf + lattrs[i].offset,
                   attrs[i].attrLength);
          }
          if(aggs[i] == COUNT_F) {
            int* count = (int*)(buf + attrs[i].offset);
            *count = 0;
            (*count)++;
          }
        }
      }
      prev = lt;
      // cout << "created t " << t << endl;
    }
    
    if(!firstTime) // 0 records
      set.push_back(t);

    it = set.begin();

    bIterOpen = true;
    return 0;
  }

  virtual RC GetNext(Tuple &t) {
    if(!bIterOpen)
      return RM_FNOTOPEN;

    if (it == set.end())
      return Eof();
    t = *it;
    // cout << "Agg::GetNext() tuple " << t << endl;

    it++;
    return 0;
  }

  virtual RC Close() {
    if(!bIterOpen)
      return RM_FNOTOPEN;

    RC rc = lhsIt->Close();
    if(rc != 0 ) return rc;
    set.clear();
    it = set.begin();

    bIterOpen = false;
    return 0;
  }
  
  virtual RC Eof() const { return QL_EOF; }

 private:
  TupleCmp cmp;
  TupleCmp* attrCmps;
  Iterator* lhsIt;
  DataAttrInfo* lattrs;
  AggFun* aggs;
  list<Tuple> set;
  list<Tuple>::const_iterator it;
};

#endif // AGG_H
