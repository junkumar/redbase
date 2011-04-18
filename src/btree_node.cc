#include "btree_node.h"
#include "pf.h"
#include <cstdlib>

BtreeNode::BtreeNode(AttrType attrType, int attrLength,
                     PF_PageHandle& ph, bool newPage,
                     int pageSize, bool duplicates,
                     bool leaf, bool root)
:keys(NULL), rids(NULL),
 attrLength(attrLength), attrType(attrType),
 pph(&ph),
 dups(duplicates), isRoot(root), isLeaf(leaf),
 numKeys(0)
{
  order = floor((pageSize) / (sizeof(RID) + attrLength)) - 1;
  // n + 1 pointers + n keys + 1 keyspace used for numKeys
  while( ((order+1) * (attrLength + sizeof(RID)))
         > (unsigned int) pageSize)
    order--;
  assert( ((order+1) * (attrLength + sizeof(RID))) 
          <= (unsigned int) pageSize);
  
  // Leaf Node - RID(i) points to the record associated with keys(i)
  // Intermediate Node - RID(i) points to the ix page associated with
  // keys < keys(i)
  char * pData = NULL;
  RC rc = ph.GetData(pData);
  if (rc != 0) {
    pph = NULL;
    return;
  }

  // keys = new char[attrLength*(order+1)];
  // rids = new RID[order+1];

  keys = pData;
  rids = (RID*) (pData + attrLength*(order+1));
  // if this is an existing page read number of keys from page
  if(!newPage)
    GetNumKeys();
  else
    // or start fresh with 0 keys
    numKeys = 0;
  assert(IsValid() == 0);
}


BtreeNode::~BtreeNode()
{
  // if (keys != NULL)
  //   delete [] keys;
  // if(rids != NULL)
  //   delete [] rids;
};


RC BtreeNode::IsValid() const
{
  if (order <= 0)
    return IX_SIZETOOBIG;

  bool ret = true;
  ret = ret && (keys != NULL);
  ret = ret && (rids != NULL);
  ret = ret && (numKeys >= 0);

  return ret ? 0 : IX_BADIXPAGE;
};


int BtreeNode::GetMaxKeys() const
{
  assert(IsValid() == 0);
  return order;
};

int BtreeNode::GetNumKeys() 
{
  assert(IsValid() == 0);
  // get from page and store in local var
  char * loc = keys + attrLength*order;
  int * pi = (int *) loc;  
  numKeys = *pi;
  return numKeys;
};

// return 0 if key is found at position
// return -1 if position is bad
int BtreeNode::GetKey(int pos, void* &key) const
{
  assert(IsValid() == 0);
  assert(pos >= 0 && pos < numKeys);
  if (pos >= 0 && pos < numKeys) 
    {
      key = keys + attrLength*pos;
      return 0;
    } 
  else 
    {
      return -1;
    }
}

int BtreeNode::SetKey(int pos, const void* newkey)
{
  assert(IsValid() == 0);
  assert(pos >= 0 && pos <= order);
  if (pos >= 0 && pos <= order) 
    {
      memcpy(keys + attrLength*pos,
             newkey,
             attrLength);
      return 0;
    } 
  else 
    {
      return -1;
    }
}

// return 0 if insert was successful
// return -1 if there is no space
int BtreeNode::Insert(const void* newkey, const RID & rid)
{
  assert(IsValid() == 0);
  if(numKeys >= order) return -1;
  int i = -1;
  for(i = numKeys-1; i >= 0; i--)
  {
    void *p;
    GetKey(i, p);    
    if (CmpKey(newkey, p) > 0)
      break; // go out and insert at i
    rids[i+1] = rids[i];
    SetKey(i + 1, p);
  }
  rids[i+1] = rid;
  SetKey(i+1, newkey);
  numKeys++;
  SetKey(order, &numKeys);
  return 0;
}

// return 0 if remove was successful
// return -1 if key does not exist
int BtreeNode::Remove(const void* newkey)
{
  assert(IsValid() == 0);
  int pos = FindKey(newkey);
  if (pos < 0)
    return pos;
  // shift all keys after this
  for(int i = pos; i < numKeys-1; i++)
  {
    void *p;
    GetKey(i+1, p);    
    SetKey(i, p);
    rids[i] = rids[i+1];
  }
  numKeys--;
  SetKey(order, &numKeys);
  return 0;
}


// return position if key will fit in a particular position
// return -1 if there was an error
int BtreeNode::FindKeyPosition(const void* &key) const
{
  assert(IsValid() == 0);
  for(int i = numKeys-1; i >=0; i--)
  {
    void* k;
    if(GetKey(i, k) != 0)
      return -1;
    if (CmpKey(key, k) >= 0)
      return i+1;
  }
  return 0; // key is smaller than anything currently
}

// return position if key already exists at position
// return -1 if there was an error or if key does not exist
int BtreeNode::FindKey(const void* &key) const
{
  assert(IsValid() == 0);

  for(int i = 0; i < numKeys; i++)
  {
    void* k;
    if(GetKey(i, k) != 0)
      return -1;
    if (CmpKey(key, k) == 0)
      return i;
  }
  return -1;
}


int BtreeNode::CmpKey(const void * a, const void * b) const
{
  if (attrType == STRING) {
    return memcmp(a, b, attrLength);
  } 

  if (attrType == FLOAT) {
    typedef float MyType;
    if ( *(MyType*)a >  *(MyType*)b ) return 1;
    if ( *(MyType*)a == *(MyType*)b ) return 0;
    if ( *(MyType*)a <  *(MyType*)b ) return -1;
  }

  if (attrType == INT) {
    typedef int MyType;
    if ( *(MyType*)a >  *(MyType*)b ) return 1;
    if ( *(MyType*)a == *(MyType*)b ) return 0;
    if ( *(MyType*)a <  *(MyType*)b ) return -1;
  }

}


bool BtreeNode::isSorted() const
{
  assert(IsValid() == 0);

  for(int i = 0; i < numKeys-2; i++)
  {
    void* k1;
    GetKey(i, k1);
    void* k2;
    GetKey(i+1, k2);
    if (CmpKey(k1, k2) > 0)
      return false;
  }
  return true;
}
