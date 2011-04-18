#ifndef BTREE_NODE_H
#define BTREE_NODE_H

#include "redbase.h"
#include "pf.h"
#include "rm_rid.h"
#include "ix_error.h"
#include <iosfwd>

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
  RC GetKey(int pos, void* &key) const;
  int SetKey(int pos, const void* newkey);

  // return 0 if insert was successful
  // return -1 if there is no space
  int Insert(const void* newkey, const RID& newrid);

// return 0 if remove was successful
// return -1 if key does not exist
// kpos is optional - will remove from that position if specified
// if kpos is specified newkey can be NULL
  int Remove(const void* newkey, int kpos = -1);
  
  // exact match
  int FindKey(const void* &key) const;
  RID FindAddr(const void* &key) const;
  // get rid for given position
  // return (-1, -1) if there was an error or pos was not found
  RID GetAddr(const int pos) const;


  // find a poistion instead of exact match
  int FindKeyPosition(const void* &key) const;
  RID FindAddrAtPosition(const void* &key) const;

  // split or merge this node with rhs node
  RC Split(BtreeNode* rhs);
  RC Merge(BtreeNode* rhs);

  // print
  void Print(ostream & os);


  int CmpKey(const void * k1, const void * k2) const;
  bool isSorted() const;
  RC LargestKey(void *& key) const;

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
  bool dups; // Are duplicate values allowed for keys ?
  bool isRoot;
  bool isLeaf;
};


#endif // BTREE_NODE_H
