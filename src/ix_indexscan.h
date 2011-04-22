//
// ix_indexscan.h
//
//   Index Manager Component Interface
//

#ifndef IX_INDEX_SCAN_H
#define IX_INDEX_SCAN_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"
#include "ix_indexhandle.h"
#include "predicate.h"

//
// IX_IndexScan: condition-based scan of index entries
//
class IX_IndexScan {
 public:
  IX_IndexScan();
  ~IX_IndexScan();

  // Open index scan
  RC OpenScan(const IX_IndexHandle &indexHandle,
              CompOp compOp,
              void *value,
              ClientHint  pinHint = NO_HINT);

  // Get the next matching entry return IX_EOF if no more matching
  // entries.
  RC GetNextEntry(RID &rid);

  RC GetNextEntry(void *& key, RID &rid);

  // Close index scan
  RC CloseScan();
 private:
  Predicate* pred;
  IX_IndexHandle* pixh;
  BtreeNode* currNode;
  int currPos;
  bool bOpen;
};


#endif // IX_INDEX_SCAN_H
