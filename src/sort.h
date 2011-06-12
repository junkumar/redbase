//
// File:        Sort.h
//

#ifndef SORT_H
#define SORT_H

#include "redbase.h"
#include "iterator.h"
#include "sm.h"
#include "ql_error.h"
#include <set>
#include "predicate.h"

using namespace std;

// Single key, single pass sort operator. Uses memory directly.
class Sort: public Iterator {
 public:
   Sort(Iterator *    lhsIt,
        AttrType     sortKeyType,
        int    sortKeyLength,
        int     sortKeyOffset,
        RC& status,
        bool desc_=false) 
     :cmp(sortKeyType, sortKeyLength, sortKeyOffset, (desc == true ? GT_OP : LT_OP)),
    lhsIt(lhsIt), set(cmp)
    {
      if(lhsIt == NULL || 
         sortKeyLength < 1 || 
         sortKeyOffset < 0 ||
         sortKeyOffset > lhsIt->TupleLength()-1 ) {
        status = SM_NOSUCHTABLE;
        return;
      }

      bSorted = true;
      this->desc = desc_;
      attrCount = lhsIt->GetAttrCount();
      attrs = new DataAttrInfo[attrCount];
      DataAttrInfo* cattrs = lhsIt->GetAttr();
      for (int i = 0; i < attrCount; i++) {
        attrs[i] = cattrs[i];
      }

      it = set.begin();

      string attr;
      for (int i = 0; i < attrCount; i++) {
        if(attrs[i].offset == sortKeyOffset) {
          attr = string(attrs[i].attrName);
          sortRel = string(attrs[i].relName);
          sortAttr = string(attrs[i].attrName);
        }
      }
      // wrong offset - no such key
      if(attr == string()) {
        status = SM_NOSUCHTABLE;
        return;
      }

      explain << "Sort\n";
      explain << "   SortKey=" << attr
              << " " << (desc == true ? "DESC" : "ASC")
              << endl;

      status = 0;
    }
    
  string Explain() {
    stringstream dyn;
    dyn << indent << explain.str();
    lhsIt->SetIndent(indent + "-----");
    dyn << lhsIt->Explain();
    return dyn.str();
  }

  virtual ~Sort() {}

  virtual RC Open() {
    if(bIterOpen)
      return RM_HANDLEOPEN;

    RC rc = lhsIt->Open();
    if(rc != 0 ) return rc;

    Tuple t = lhsIt->GetTuple();
    while(lhsIt->GetNext(t) != lhsIt->Eof()) {
      set.insert(t);
    }
    
    it = set.begin();
    rit = set.rbegin();

    bIterOpen = true;
    return 0;
  }

  virtual RC GetNext(Tuple &t) {
    if(!bIterOpen)
      return RM_FNOTOPEN;

    if (it == set.end() || rit == set.rend())
      return Eof();
    if(!desc)
      t = *it;
    else
      t = *rit;
    it++;
    rit++;
    return 0;
  }

  virtual RC Close() {
    if(!bIterOpen)
      return RM_FNOTOPEN;

    RC rc = lhsIt->Close();
    if(rc != 0 ) return rc;
    set.clear();
    it = set.begin();
    rit = set.rbegin();

    bIterOpen = false;
    return 0;
  }
  
  virtual RC Eof() const { return QL_EOF; }

 private:
  TupleCmp cmp;
  Iterator* lhsIt;
  multiset<Tuple, TupleCmp> set;
  multiset<Tuple>::const_iterator it;
  multiset<Tuple>::const_reverse_iterator rit;
};

#endif // SORT_H
