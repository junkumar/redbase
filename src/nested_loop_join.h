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

class NestedLoopJoin: public virtual Iterator {
 public:
  NestedLoopJoin(
                 Iterator *    lhsIt,      // access for left i/p to join -R
                 Iterator *    rhsIt,      // access for right i/p to join -S
                 RC& status,
                 int nOutFilters = 0,
                 // join keys are specified
                 // as conditions. NULL implies cross product
                 const Condition outFilters[] = NULL  
                 );

  virtual ~NestedLoopJoin();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();
  virtual string Explain();

  RC IsValid();
  virtual RC Eof() const { return QL_EOF; }

  // only used by derived class NLIJ
  // RC virtual ReopenIfIndexJoin(const char* newValue) { return 0; }
  RC virtual ReopenIfIndexJoin(const Tuple& t) { return 0; }
  
 protected:
  void virtual EvalJoin(Tuple &t, bool& joined, Tuple* l, Tuple* r);

 protected:
  Iterator* lhsIt;
  Iterator* rhsIt;
  Tuple left;
  Tuple right;
  int nOFilters;
  Condition* oFilters; // join keys
  DataAttrInfo* lKeys; // attrinfo of join key in the left iterator
  DataAttrInfo* rKeys; // attrinfo of join key in the right iterator
};

#endif // NESTEDLOOPJOIN_H
