//
// ix.h
//
//   Index Manager Component Interface
//

#ifndef IX_INDEX_MANAGER_H
#define IX_INDEX_MANAGER_H

// Please do not include any other files than the ones below in this file.

#include "redbase.h"  // Please don't change these lines
#include "rm_rid.h"  // Please don't change these lines
#include "pf.h"
#include "ix_indexhandle.h"


//
// IX_Manager: provides IX index file management
//
class IX_Manager {
 public:
  IX_Manager(PF_Manager &pfm);
  ~IX_Manager();

  // Create a new Index
  RC CreateIndex(const char *fileName, int indexNo,
                 AttrType attrType, int attrLength,
                 int pageSize = PF_PAGE_SIZE);

  // Destroy and Index
  RC DestroyIndex(const char *fileName, int indexNo);

  // Open an Index
  RC OpenIndex(const char *fileName, int indexNo,
               IX_IndexHandle &indexHandle);

  // Close an Index
  RC CloseIndex(IX_IndexHandle &indexHandle);
 private:
  PF_Manager& pfm;
};

#endif // IX_INDEX_MANAGER_H
