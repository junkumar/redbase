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
                 int nOutFilters,
                 const Condition outFilters[]
                 )
    :NestedLoopJoin(lhsIt, rhsIt, status, nOutFilters, outFilters)
    {
      if(nOutFilters < 1 || outFilters == NULL) {
        status = QL_BADJOINKEY;
        return;
      }
      explain.str("");
      explain << "NestedLoopIndexJoin\n";
      if(nOFilters > 0) {
        explain << "   nJoinConds = " << nOutFilters << "\n";
        for (int i = 0; i < nOutFilters; i++)
          explain << "   joinConds[" << i << "]:" << outFilters[i] << "\n";
      }
    }

  RC virtual ReopenIfIndexJoin(const Tuple& t) {
    // assume that first condition is the join condition.
   
    IndexScan* rhsIxIt = dynamic_cast<IndexScan*>(rhsIt);
    if(rhsIxIt == NULL) return 0;

    void * newValue = NULL;
    t.Get(rhsIxIt->GetIndexAttr().c_str(), newValue);
    /* cerr << "NestedLoopIndexJoin::ReopenIfIndexJoin() newBuf " << t */
    /*      << " newVAlue " << (char*)newValue */
    /*      << endl; */

    RC rc = rhsIxIt->ReOpenScan(newValue);
    return rc;
  }
};

#endif // NESTEDLOOPINDEXJOIN_H
