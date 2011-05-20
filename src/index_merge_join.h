//
// File:        IndexMergeJoin.h
//

#ifndef INDEXMERGEJOIN_H
#define INDEXMERGEJOIN_H

#include "redbase.h"
#include "iterator.h"
#include "rm.h"

using namespace std;

class IndexMergeJoin: public Iterator {
 public:
  IndexMergeJoin(const char * lJoinAttr,   // name of join key - left
                 const char * rJoinAttr,   // name of join key - right
                 Iterator *    lhsIt,      // access for left i/p to join -R
                 Iterator *    rhsIt,      // access for right i/p to join -S
                 RC& status,
                 int nOutFilters = 0,
                 const Condition outFilters[] = NULL
                 //FldSpec  * proj_list,
                 // int        n_out_flds,
                 );

  virtual ~IndexMergeJoin();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();
  
  RC IsValid();
  virtual RC Eof() const { return QL_EOF; }

 private:
  DataAttrInfo lKey; // offset of join key in the left iterator
  DataAttrInfo rKey; // offset of join key in the right iterator
  Iterator* lhsIt;
  Iterator* rhsIt;
  Tuple left;
  Tuple right;
  int nOFilters;
  Condition* oFilters;
};

#endif // INDEXMERGEJOIN_H
