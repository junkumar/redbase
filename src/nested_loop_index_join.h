//
// File:        nested_loop_index_join.h
//

#ifndef NESTEDLOOPINDEXJOIN_H
#define NESTEDLOOPINDEXJOIN_H

#include "nested_loop_join.h"
#include "index_scan.h"

using namespace std;

class NestedLoopIndexJoin: public NestedLoopJoin {
 public:
  NestedLoopIndexJoin(
                 Iterator*    lhsIt,      // access for left i/p to join -R
                 IndexScan*   rhsIt,      // access for right i/p to join -S
                 RC& status,
                 int nOutFilters = 0,
                 // join keys are specified
                 // as conditions. NULL implies cross product
                 const Condition outFilters[] = NULL  
                 //FldSpec  * proj_list,
                 // int        n_out_flds,
                 )
    :NestedLoopJoin(lhsIt, rhsIt, status, nOutFilters, outFilters)
    {}

  RC virtual ReopenIfIndexJoin() {
    // close
    // open scan with lhs.left's value
  }
};

#endif // NESTEDLOOPINDEXJOIN_H
