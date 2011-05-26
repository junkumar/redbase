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

  // It is assumed that IX component clients will not close the
  // corresponding open index while a scan is underway.
  RC OpenScan(const IX_IndexHandle &indexHandle,
              CompOp compOp,
              void *value,
              ClientHint  pinHint = NO_HINT,
              bool desc = false);

  // Get the next matching entry return IX_EOF if no more matching
  // entries.
  RC GetNextEntry(RID &rid);

  // also passes back the key scanned and number of scanned items so
  // far (whether the predicate matched or not.
  RC GetNextEntry(void *& key, RID &rid, int& numScanned);

  // Close index scan
  RC CloseScan();

  // for iterator to reset state for another open/close
  RC ResetState();
  
  bool IsOpen() const { return (bOpen && pred != NULL && pixh != NULL); }
  bool IsDesc() const { return desc; }
 private:
  RC OpOptimize(); // Optimizes based on value of c, value and resets state
  RC EarlyExitOptimize(void* now);
 private:
  Predicate* pred;
  IX_IndexHandle* pixh;
  BtreeNode* currNode;
  int currPos;
  void* currKey; // saved to check for delete on scan
  RID currRid; // saved to check for delete on scan
  bool bOpen;
  bool desc; // Is scan order ascending(def) or descending ?
  bool eof; // early EOF set by btree analysis - set by OpOpt
  bool foundOne; // flag that is set by getNext if it is ever successful
  BtreeNode* lastNode; // last node setup by OpOpt
  CompOp c; // save Op for OpOpt
  void* value; // save Op for OpOpt
};


#endif // IX_INDEX_SCAN_H
