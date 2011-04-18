#ifndef BTREE_NODE_H
#define BTREE_NODE_H

#include "redbase.h"
#include "pf.h"
#include "ix_index_handle.h"
#include "ix_error.h"

// Key has to be a single attribute of type attrType and length attrLength

class BtreeNode {
 public:
  // if newPage is false then the page ph is expected to contain an
  // existing btree node, otherwise a fresh node is assumed.
  BtreeNode(AttrType attrType, int attrLength,
            PF_PageHandle& ph, bool newPage = true,
            int pageSize = PF_PAGE_SIZE, bool duplicates = false, 
            bool leaf = false, bool root = false);
  ~BtreeNode();
  friend class BtreeNodeTest;
  RC IsValid() const;
  int GetMaxKeys() const;
  int GetNumKeys();
  int GetKey(int pos, void* &key) const;
  int SetKey(int pos, const void* newkey);
  int Insert(const void* newkey, const RID& newrid);
  int Remove(const void* newkey);
  int FindKey(const void* &key) const;
  int FindKeyPosition(const void* &key) const;
  int CmpKey(const void * k1, const void * k2) const;
  bool isSorted() const;
    
 private:
  // serialized
  char * keys; // should not be accessed directly as keys[] but with SetKey()
  RID * rids;
  int numKeys; // stored in keys[order] - no real page header
  // not serialized - common to all ix pages
  int attrLength;
  AttrType attrType;
  int order;
  // not serialized - convenience
  PF_PageHandle * pph; // where did this page come from ?
  bool dups; // Are duplicate values allowed for keys ?
  bool isRoot;
  bool isLeaf;
};


#endif // BTREE_NODE_H
