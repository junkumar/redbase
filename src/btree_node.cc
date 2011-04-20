#include "btree_node.h"
#include "pf.h"
#include <cstdlib>

BtreeNode::BtreeNode(AttrType attrType, int attrLength,
                     PF_PageHandle& ph, bool newPage,
                     int pageSize, bool duplicates,
                     bool leaf, bool root)
:keys(NULL), rids(NULL),
 attrLength(attrLength), attrType(attrType),
 dups(duplicates), isRoot(root), isLeaf(leaf)
{

  order = floor(
    (pageSize + sizeof(numKeys) + 2*sizeof(PageNum)) / 
    (sizeof(RID) + attrLength));
  // n + 1 pointers + n keys + 1 keyspace used for numKeys
  while( ((order) * (attrLength + sizeof(RID)))
         > ((unsigned int) pageSize - sizeof(numKeys) - 2*sizeof(PageNum) ))
    order--;
  assert( ((order) * (attrLength + sizeof(RID))) 
          <= (unsigned int) pageSize - sizeof(numKeys) - 2*sizeof(PageNum) );

  // Leaf Node - RID(i) points to the record associated with keys(i)
  // Intermediate Node - RID(i) points to the ix page associated with
  // keys <= keys(i)
  char * pData = NULL;
  RC rc = ph.GetData(pData);
  if (rc != 0) {
    // bad page - call IsValid after construction to check
    return;
  }

  PageNum p; ph.GetPageNum(p);
  SetPageRID(RID(p, -1));

  keys = pData;
  rids = (RID*) (pData + attrLength*(order));
  // if this is an existing page read number of keys from page
  if(!newPage) {
    numKeys = 0; //needs init value >=0
    GetNumKeys();
    GetLeft();
    GetRight();
  }
  else
    // or start fresh with 0 keys
    numKeys = 0;

  assert(IsValid() == 0);
  //Layout
    // n * keys - takes up n * attrLength
    // n * RIds - takes up n * sizeof(RID)
    // numKeys - takes up sizeof(int)
    // left - takes up sizeof(PageNum)
    // right - takes up sizeof(PageNum)
}

int BtreeNode::GetNumKeys() 
{
  assert(IsValid() == 0);
  // get from page and store in local var
  void * loc = (char*)rids + sizeof(RID)*order;
  int * pi = (int *) loc;
  numKeys = *pi;
  return numKeys;
};

// sets number of keys
// returns -1 on error
int BtreeNode::SetNumKeys(int newNumKeys)
{
  assert(IsValid() == 0);
  memcpy((char*)rids + sizeof(RID)*order,
         &newNumKeys,
         sizeof(int));
  numKeys = newNumKeys; // conv variable
  return 0;
}

PageNum BtreeNode::GetLeft() 
{
  assert(IsValid() == 0);
  void * loc = (char*)rids + sizeof(RID)*order + sizeof(int);
  return *((PageNum*) loc);
};

int BtreeNode::SetLeft(PageNum p)
{
  assert(IsValid() == 0);
  memcpy((char*)rids + sizeof(RID)*order + sizeof(int),
         &p,
         sizeof(PageNum));
  return 0;
}

PageNum BtreeNode::GetRight() 
{
  assert(IsValid() == 0);
  void * loc = (char*)rids + sizeof(RID)*order + sizeof(int) + sizeof(PageNum);
  return *((PageNum*) loc);
};

int BtreeNode::SetRight(PageNum p)
{
  assert(IsValid() == 0);
  memcpy((char*)rids + sizeof(RID)*order + sizeof(int)  + sizeof(PageNum),
         &p,
         sizeof(PageNum));
  return 0;
}



// get/set pageRID
RID BtreeNode::GetPageRID() const
{
  return pageRID;
}
void BtreeNode::SetPageRID(const RID& r)
{
  pageRID = r;
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
  assert(ret);
  ret = ret && (rids != NULL);
  assert(ret);
  ret = ret && (numKeys >= 0);
  assert(ret);
  ret = ret && (numKeys <= order);
  if(!ret)
    cerr << "order was " << order << " numkeys was  " << numKeys << endl;
  return ret ? 0 : IX_BADIXPAGE;
};


int BtreeNode::GetMaxKeys() const
{
  assert(IsValid() == 0);
  return order;
};

// populate NULL if there are no keys
// other populate largest key
void* BtreeNode::LargestKey() const
{
  assert(IsValid() == 0);
  void * key = NULL;
  if (numKeys > 0) {
    GetKey(numKeys-1, key);
    return key;
  } else {
    assert("Largest Key called when numKey <= 0");
    return NULL;
  }
};


// return 0 if key is found at position
// return -1 if position is bad
RC BtreeNode::GetKey(int pos, void* &key) const
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

// copy key at location pos to the pointer provided
// must be already allocated
int BtreeNode::CopyKey(int pos, void* toKey) const
{
  assert(IsValid() == 0);
  assert(pos >= 0 && pos < order);
  if(toKey == NULL)
    return -1;
  if (pos >= 0 && pos < order) 
    {
      memcpy(toKey,
             keys + attrLength*pos,
             attrLength);
      return 0;
    } 
  else 
    {
      return -1;
    }
}

// set key at location pos with a copy of whatever pointer provided
// points to
// returns -1 on error
int BtreeNode::SetKey(int pos, const void* newkey)
{
  assert(IsValid() == 0);
  assert(pos >= 0 && pos < order);
  if (pos >= 0 && pos < order) 
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
// return -1 if there is no space - overflow
int BtreeNode::Insert(const void* newkey, const RID & rid)
{
  assert(IsValid() == 0);
  if(numKeys >= order) return -1;
  int i = -1;
  void *prevKey = NULL;
  void *currKey = NULL;
  for(i = numKeys-1; i >= 0; i--)
  {
    prevKey = currKey;
    GetKey(i, currKey);    
    if (CmpKey(newkey, currKey) >= 0)
      break; // go out and insert at i
    rids[i+1] = rids[i];
    SetKey(i + 1, currKey);
  }
  // handle case where keys are equal
  // can result only from a split an a newer page in a split will
  // always have the higher pageNum
  // if (prevKey != NULL && CmpKey(prevKey, currKey)) {
  // TODO - not needed for now - trying >=
  // }
  // inserting at i


  rids[i+1] = rid;
  SetKey(i+1, newkey);

  assert(isSorted());
//  numKeys++;
  SetNumKeys(GetNumKeys()+1);
  return 0;
}

// return 0 if remove was successful
// return -1 if key does not exist
// kpos is optional - will remove from that position if specified
// if kpos is specified newkey can be NULL
int BtreeNode::Remove(const void* newkey, int kpos)
{
  assert(IsValid() == 0);
  int pos = -1;
  if (kpos != -1) {
    if (kpos < 0 || kpos >= numKeys)
      return -1;
    pos = kpos;
  } 
  else {
    pos = FindKey(newkey);
    if (pos < 0)
      return pos;
    // shift all keys after this
  }
  for(int i = pos; i < numKeys-1; i++)
  {
    void *p;
    GetKey(i+1, p);    
    SetKey(i, p);
    rids[i] = rids[i+1];
  }
  SetNumKeys(GetNumKeys()-1);
  return 0;
}

// return position if key will fit in a particular position
// return (-1, -1) if there was an error
RID BtreeNode::FindAddrAtPosition(const void* &key) const
{
  assert(IsValid() == 0);
  int pos = FindKeyPosition(key);
  if (pos == -1 || pos >= numKeys) return RID(-1,-1);
  return rids[pos];
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

// get rid for given position
// return (-1, -1) if there was an error or pos was not found
RID BtreeNode::GetAddr(const int pos) const
{
  assert(IsValid() == 0);
  if(pos < 0 || pos > numKeys)
    return RID(-1, -1);
  return rids[pos];
}

// return rid for exact key match
// return (-1, -1) if there was an error or key was not found
RID BtreeNode::FindAddr(const void* &key) const
{
  assert(IsValid() == 0);
  int pos = FindKey(key);
  if (pos == -1) return RID(-1,-1);
  return rids[pos];
}


// return position if key already exists at position
// return -1 if there was an error or if key does not exist
int BtreeNode::FindKey(const void* &key) const
{
  assert(IsValid() == 0);

  for(int i = numKeys-1; i >= 0; i--)
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

  assert("should never get here - bad attrtype");
  return 0; // to satisfy gcc warnings
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

// return -1 on error, 0 on success
// split or merge this node with rhs node
RC BtreeNode::Split(BtreeNode* rhs)
{
  assert(IsValid() == 0);
  assert(rhs->IsValid() == 0);

  // if (numKeys < order)
  //   return -1; // splitting too early - TODO

  // this node is full
  // shift higher keys to rhs
  int firstMovedPos = (numKeys+1)/2;
  int moveCount = (numKeys - firstMovedPos + 1);
  // ensure that rhs wont overflow
  if( (rhs->GetNumKeys() + moveCount)
      > rhs->GetMaxKeys())
    return -1;

  for (int pos = firstMovedPos; pos < numKeys; pos++) {
    RID r = rids[pos];
    void * k = NULL; this->GetKey(pos, k);
    RC rc = rhs->Insert(k, r);
    if(rc != 0) return rc;
  }

  // TODO use range remove - faster
  for (int i = 0; i < moveCount; i++) {
    RC rc = this->Remove(NULL, firstMovedPos);
    if(rc != 0) return rc;
  }

  assert(isSorted());
  assert(rhs->isSorted());

  assert(IsValid() == 0);
  assert(rhs->IsValid() == 0);
  return 0;
}

// return -1 on error, 0 on success
// merge this node with rhs node and put everything in this node
RC BtreeNode::Merge(BtreeNode* rhs) {
  assert(IsValid() == 0);
  assert(rhs->IsValid() == 0);

  if (numKeys + rhs->GetNumKeys() > order)
    return -1; // overflow will result from merge

  for (int pos = 0; pos < rhs->GetNumKeys(); pos++) {
    void * k = NULL; rhs->GetKey(pos, k);
    RID r = rhs->GetAddr(pos);
    RC rc = this->Insert(k, r);
    if(rc != 0) return rc;
  }

  int moveCount = rhs->GetNumKeys();
  for (int i = 0; i < moveCount; i++) {
    RC rc = rhs->Remove(NULL, 0);
    if(rc != 0) return rc;
  }

  assert(IsValid() == 0);
  assert(rhs->IsValid() == 0);
  return 0;
}

void BtreeNode::Print(ostream & os) {
  os << pageRID.Page() << "{";
  for (int pos = 0; pos < GetNumKeys(); pos++) {
    void * k = NULL; GetKey(pos, k);
    os << "(";
    if( attrType == INT )
      os << *((int*)k);
    if( attrType == FLOAT )
      os << *((float*)k);
    if( attrType == STRING )
      os << *((char*)k);
      
    os << "," 
       << GetAddr(pos) << "), ";
  }
  os << "}\n";
}
