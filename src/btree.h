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
class Btree {
  typedef BtreeNode<T> TNode;
 public:
  
 private:
  
};

#endif // BTREE_H
