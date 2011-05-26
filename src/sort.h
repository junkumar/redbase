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

class TupleCmp {
 public:
  TupleCmp(AttrType     sortKeyType,
           int    sortKeyLength,
           int     sortKeyOffset) 
    :p(sortKeyType, sortKeyLength, sortKeyOffset, LT_OP, NULL, NO_HINT),
    sortKeyOffset(sortKeyOffset)
    {}
  
  bool operator() (const Tuple& lhs, const Tuple& rhs) const 
  {
      void * b = NULL;
      rhs.Get(sortKeyOffset, b);
      const char * abuf;
      lhs.GetData(abuf);
      return p.eval(abuf, (char*)b, LT_OP);
  }
 private:
  Predicate p;
  int sortKeyOffset; 
};

// Single key, single pass sort operator. Uses memory directly.
class Sort: public SortedIterator {
 public:
   Sort(Iterator *    lhsIt,
        AttrType     sortKeyType,
        int    sortKeyLength,
        int     sortKeyOffset,
       RC& status) 
     :cmp(sortKeyType, sortKeyLength, sortKeyOffset),
    set(cmp), lhsIt(lhsIt)
    {
      if(lhsIt == NULL) {
        status = SM_NOSUCHTABLE;
        return;
      }

      attrCount = lhsIt->GetAttrCount();
      attrs = new DataAttrInfo[attrCount];
      DataAttrInfo* cattrs = lhsIt->GetAttr();
      for (int i = 0; i < attrCount; i++) {
        attrs[i] = cattrs[i];
      }

      it = set.begin();

      string attr;
      for (int i = 0; i < attrCount; i++) {
        if(attrs[i].offset == sortKeyOffset)
          attr = string(attrs[i].attrName);
      }

      explain << "Sort\n";
      explain << "   SortKey=" << attr
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
    bIterOpen = true;
    return 0;
  }

  virtual RC GetNext(Tuple &t) {
    if(!bIterOpen)
      return RM_FNOTOPEN;

    if (it == set.end())
      return Eof();
    t = *it;
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
  Iterator* lhsIt;
  TupleCmp cmp;
  multiset<Tuple, TupleCmp> set;
  multiset<Tuple>::const_iterator it;
};

#endif // SORT_H
