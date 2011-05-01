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
            int pageSize = PF_PAGE_SIZE);
  RC ResetBtreeNode(PF_PageHandle& ph, const BtreeNode& rhs);
  ~BtreeNode();
  int Destroy();

  friend class BtreeNodeTest;
  friend class IX_IndexHandle;
  RC IsValid() const;
  int GetMaxKeys() const;
  
  // structural setters/getters - affect PF_page composition
  int GetNumKeys();
  int SetNumKeys(int newNumKeys);
  int GetLeft();
  int SetLeft(PageNum p);
  int GetRight();
  int SetRight(PageNum p);


  RC GetKey(int pos, void* &key) const;
  int SetKey(int pos, const void* newkey);
  int CopyKey(int pos, void* toKey) const;


  // return 0 if insert was successful
  // return -1 if there is no space
  int Insert(const void* newkey, const RID& newrid);

// return 0 if remove was successful
// return -1 if key does not exist
// kpos is optional - will remove from that position if specified
// if kpos is specified newkey can be NULL
  int Remove(const void* newkey, int kpos = -1);
  
  // exact match functions

  // return position if key already exists at position
  // if there are dups - returns rightmost position unless an RID is
  // specified.
  // if optional RID is specified, will only return a position if both
  // key and RID match.
  // return -1 if there was an error or if key does not exist
  int FindKey(const void* &key, const RID& r = RID(-1,-1)) const;

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

  // get/set pageRID
  RID GetPageRID() const;
  void SetPageRID(const RID&);

  int CmpKey(const void * k1, const void * k2) const;
  bool isSorted() const;
  void* LargestKey() const;

 private:
  // serialized
  char * keys; // should not be accessed directly as keys[] but with SetKey()
  RID * rids;
  int numKeys;
  // not serialized - common to all ix pages
  int attrLength;
  AttrType attrType;
  int order;
  // not serialized - convenience
  RID pageRID;
};


#endif // BTREE_NODE_H
