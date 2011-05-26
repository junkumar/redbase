//
// File:        nested_block_join.h
//

#ifndef NESTEDBLOCKJOIN_H
#define NESTEDBLOCKJOIN_H

#include "nested_loop_join.h"
#include "file_scan.h"
#include <vector>
#include <cmath>

using namespace std;

class NestedBlockJoin: public NestedLoopJoin {
 public:
  NestedBlockJoin(
                  FileScan*    lhsIt,      // access for left i/p to join -R
                  Iterator*    rhsIt,      // access for right i/p to join -S
                  RC& status,
                  int nOutFilters = 0,
                  const Condition outFilters[] = NULL,
                  int blockPages = 40
                  )
    :NestedLoopJoin(lhsIt, rhsIt, status, nOutFilters, outFilters),
    blockSize(blockPages-3) // leave room for 3 other pages
    {
      justOpen = true;
      // outer params
      nPages = lhsIt->GetNumPages();
      pageSize = lhsIt->GetNumSlotsPerPage();

      // cerr << "nPages, pageSize " << nPages << ", " << pageSize << endl;
       
      double p = nPages;
      p = ceil(p/blockSize);
      int nBlocks = p;
      for(int j = 0; j < nBlocks; j++) {
        // RM page 0 is header - so things start at 1
        blocks.push_back(1 + j*blockSize);
        // cerr << j << "th block - start page " <<  blocks[j] << endl;
      }
      blockIt = blocks.begin();
      left_rc = 0;
      right_rc = 0;

      explain.str("");
      explain << "NestedBlockJoin\n";
      if(nOFilters > 0) {
        explain << "   nJoinConds = " << nOutFilters << "\n";
        for (int i = 0; i < nOutFilters; i++)
          explain << "   joinConds[" << i << "]:" << outFilters[i] << "\n";
      }
    }
  
  virtual RC Open() { justOpen = true; return this->NestedLoopJoin::Open(); }


  virtual RC GetNext(Tuple &t) {
    RC invalid = IsValid(); if(invalid) return invalid;

    // cout << "GetNext [[" << left << " - " << right << endl;

    if(!bIterOpen)
      return QL_FNOTOPEN;

    bool joined = false;
    
    if(justOpen) {
      right_rc = rhsIt->GetNext(right);
      // cout << "justOpen " << left << " - " << right << endl;
    }
    justOpen = false;

    while(blockIt != blocks.end()) {
      while(right_rc != rhsIt->Eof()) {
        vector<int>::iterator next = blockIt;
        next++;
        int nextp = -1;
        if(next != blocks.end())
          nextp = *next;
        while(left_rc != lhsIt->Eof() && left.GetRid().Page() != nextp) {
          // nrecs++;
          // lhsIt++;
          // cout << left << " - " << right << endl;
          EvalJoin(t, joined, &left, &right);

          left_rc = lhsIt->GetNext(left);

          if(joined)
            return 0;

          if(left_rc == lhsIt->Eof())
            break;

          // cout << "[" << *lhsIt << ", " << *rhsIt << "]" << endl;
        }
        // nrecs = 0;
        // rhsIt++;
        right_rc = rhsIt->GetNext(right);
        // cerr << "new right is " << right << endl;
        // advance to right block start
        lhsIt->Close();
        lhsIt->Open();
        FileScan* lfs = dynamic_cast<FileScan*>(lhsIt);
        assert(lfs != NULL);
        lfs->GotoPage(*blockIt);
        left_rc = lhsIt->GetNext(left);
      }
      blockIt++;
      if(blockIt == blocks.end())
        break;
      // cout << "block " << *blockIt << endl;

      // advance to right block start
      lhsIt->Close();
      lhsIt->Open();
      FileScan* lfs = dynamic_cast<FileScan*>(lhsIt);
      assert(lfs != NULL);
      lfs->GotoPage(*blockIt);
      left_rc = lhsIt->GetNext(left);

      rhsIt->Close();
      rhsIt->Open();
      right_rc = rhsIt->GetNext(right);
    }
    // eof
    return QL_EOF;
  }

 protected:
  RC left_rc;
  RC right_rc;
  bool justOpen;
  int blockSize;
  int nPages; // num pages used by outer
  int pageSize; // num slots per page for outer
  vector<int> blocks;
  vector<int>::iterator blockIt;
};

#endif // NESTEDBLOCKJOIN_H
