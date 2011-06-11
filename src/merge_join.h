//
// File:        MergeJoin.h
//

#ifndef MERGEJOIN_H
#define MERGEJOIN_H

#include "nested_loop_join.h"
#include "redbase.h"
#include "iterator.h"
#include "rm.h"
#include <vector>

using namespace std;

// Merge two sorted iterators - equijoin+ only
class MergeJoin: public NestedLoopJoin {
 public:
  MergeJoin(Iterator*    lhsIt,      // access for left i/p to join -R
            Iterator*    rhsIt,      // access for right i/p to join -S
            RC& status,
            int nJoinConds,
            int equiCond,        // the join condition that can be used as
            // the equality consideration.
            const Condition joinConds_[]);

  virtual ~MergeJoin() {
    delete pcurrTuple;
    delete potherTuple;
  }

  virtual RC Open() {
    firstOpen = true;
    return this->NestedLoopJoin::Open();
  }

  virtual RC IsValid()
  {
    if((curr == lhsIt && other == rhsIt) ||
       (curr == rhsIt && other == lhsIt)) {
      return this->NestedLoopJoin::IsValid();
    }
    return SM_BADTABLE;
  }

  virtual RC GetNext(Tuple &t);

 private:
  bool firstOpen;
  Iterator* curr;
  Iterator* other;
  Condition equi;
  int equiPos; // position in oFilters of equality condition used as primary
               // for the merge-join.
  bool sameValueCross;
  vector<Tuple> cpvec; // store cross product of all tuples when both sides are equal
  vector<Tuple>::iterator cpit;
  bool lastCpit; // flag to track if the last tuple of a cross product was returned
                 // in the last call to GetNext
  Tuple* pcurrTuple;
  Tuple* potherTuple;
};

#endif // MERGEJOIN_H
