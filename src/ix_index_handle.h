//
// ix_file_handle.h
//
//   Index Manager Component Interface
//

#ifndef IX_FILE_HANDLE_H
#define IX_FILE_HANDLE_H

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"
#include "ix_error.h"
#include "btree.h"
//
// IX_FileHdr: Header structure for files
//
struct IX_FileHdr {
  int firstFree;     // first free page in the linked list
  int numPages;      // # of pages in the file
  int pairSize;      // size of each (key, RID) pair in index
};


//
// IX_IndexHandle: IX Index File interface
//
class IX_IndexHandle {
 public:
  IX_IndexHandle();
  ~IX_IndexHandle();
  
  // Insert a new index entry
  RC InsertEntry(void *pData, const RID &rid);
  
  // Delete a new index entry
  RC DeleteEntry(void *pData, const RID &rid);
  
  // Force index files to disk
  RC ForcePages();
 private:
  IX_FileHdr hdr;
};

#endif // #IX_FILE_HANDLE_H
