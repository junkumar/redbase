//
// File:        nested_loop_join.h
//

#ifndef NESTEDLOOPJOIN_H
#define NESTEDLOOPJOIN_H

#include "redbase.h"
#include "iterator.h"
#include "ix.h"
#include "sm.h"
#include "rm.h"
#include "ql_error.h"

using namespace std;

class NestedLoopJoin: public Iterator {
 public:
  NestedLoopJoin(const char * lJoinAttr,   // name of join key - left
                 const char * rJoinAttr,   // name of join key - right
                 Iterator *    lhsIt,      // access for left i/p to join -R
                 Iterator *    rhsIt,      // access for right i/p to join -S
               
                 //Cond **outFilter,   // Ptr to the output filter
                 //Cond **rightFilter, // Ptr to filter applied on right i
                 //FldSpec  * proj_list,
                 // int        n_out_flds,
                 RC   & status);

  virtual ~NestedLoopJoin();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();

  RC IsValid();
  virtual RC Eof() const { return QL_EOF; }

 private:
  int CmpKey(AttrType attrType, int attrLength, 
             const void* a,
             const void* b) const;

 private:
  DataAttrInfo lKey; // offset of join key in the left iterator
  DataAttrInfo rKey; // offset of join key in the right iterator
  Iterator* lhsIt;
  Iterator* rhsIt;
  Tuple left;
  Tuple right;
};

#endif // NESTEDLOOPJOIN_H
