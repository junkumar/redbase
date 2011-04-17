#ifndef BTREE_H
#define BTREE_H

#include "redbase.h"
#include "pf.h"
#include "ix_index_handle.h"
#include "ix_error.h"

// T must be a simple type where sizeof(T) is the real size
// and it cannot be serialized directly. A default constructor
// T() must be available.
template <class T>
class BtreeNode {
 public:
  BtreeNode(int pageSize = PF_PAGE_SIZE, bool duplicates = true);
  ~BtreeNode();
 protected:
  /* friend class Btree<T>; */
  /* friend class BtreeNodeTest; */
  /* friend class BtreeTest; */
  RC IsValid();
  int GetNumKeys();
 private:
  // serialized
  T * keys;
  RID * rids;
  // not serialized
  PageNum address; // where did this page come from ?
  int order;
  bool dups; // Are duplicate values allowed for keys ?
};

template <class T>
BtreeNode<T>::BtreeNode(int pageSize, bool duplicates) 
  :dups(duplicates), keys(NULL), rids(NULL), address(-1)
{
  //TODO page header ?
  order = floor((pageSize) / (sizeof(RID) + sizeof(T)));
  keys = new T[order];
  rids = new RID[order];
}

template <class T>
BtreeNode<T>::~BtreeNode()
{
  if (keys != NULL)
    delete [] keys;
  if(rids != NULL)
    delete [] rids;
};

template <class T>
RC BtreeNode<T>::IsValid()
{
  if (order <= 0)
    return IX_SIZETOOBIG;

  bool ret = true;
  ret = ret && (keys == NULL);
  ret = ret && (rids == NULL);

  return ret ? 0 : IX_BADIXPAGE;
};

template <class T>
int BtreeNode<T>::GetNumKeys()
{
  assert(IsValid() == 0);
  return order;
};

template <class T>
class Btree {
 public:
};

#endif // BTREE_H
