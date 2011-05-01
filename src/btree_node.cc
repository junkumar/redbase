#include "btree_node.h"
#include "pf.h"
#include <cstdlib>

BtreeNode::BtreeNode(AttrType attrType, int attrLength,
                     PF_PageHandle& ph, bool newPage,
                     int pageSize)
:keys(NULL), rids(NULL),
 attrLength(attrLength), attrType(attrType)
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
  else {
    // or start fresh with 0 keys
    SetNumKeys(0);
    SetLeft(-1); // node starts with no left or right 
    SetRight(-1);
  }
  assert(IsValid() == 0);
  //Layout
    // n * keys - takes up n * attrLength
    // n * RIds - takes up n * sizeof(RID)
    // numKeys - takes up sizeof(int)
    // left - takes up sizeof(PageNum)
    // right - takes up sizeof(PageNum)
}

BtreeNode::~BtreeNode()
{
  // cerr << "Destructor for BtreeNode - page id " << pageRID << endl;
};

RC BtreeNode::ResetBtreeNode(PF_PageHandle& ph, const BtreeNode& rhs)
{
  order = (rhs.order);
  attrLength = (rhs.attrLength);
  attrType = (rhs.attrType);
  numKeys = (rhs.numKeys);
  
  char * pData = NULL;
  RC rc = ph.GetData(pData);
  if(rc != 0 ) return rc;

  PageNum p; rc = ph.GetPageNum(p);
  if(rc != 0 ) return rc;
  SetPageRID(RID(p, -1));

  keys = pData;
  rids = (RID*) (pData + attrLength*(order));

  GetNumKeys();
  GetLeft();
  GetRight();

  assert(IsValid() == 0);
  return 0;
};


// Only works if node is empty
// ret -1 if node is not empty
// 0 on success
// Object can no longer be used after this point.
int BtreeNode::Destroy()
{
  assert(IsValid() == 0);
  if(numKeys != 0)
    return -1;

  keys = NULL;
  rids = NULL;
  return 0;
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
  memcpy((char*)rids + sizeof(RID)*order,
         &newNumKeys,
         sizeof(int));
  numKeys = newNumKeys; // conv variable
  assert(IsValid() == 0);
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



RC BtreeNode::IsValid() const
{
  if (order <= 0)
    return IX_INVALIDSIZE;

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
  // assert(newkey != (keys + attrLength*pos));
  if(newkey == (keys + attrLength*pos))
    return 0; // TODO - should never happen

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
// return -2 if key does not exist
// return -1 if key is the last one (lazy deletion) - underflow
// kpos is optional - will remove from that position if specified
// if kpos is specified newkey can be NULL
int BtreeNode::Remove(const void* newkey, int kpos)
{
  assert(IsValid() == 0);
  int pos = -1;
  if (kpos != -1) {
    if (kpos < 0 || kpos >= numKeys)
      return -2;
    pos = kpos;
  } 
  else {
    pos = FindKey(newkey);
    if (pos < 0)
      return -2;
    // shift all keys after this pos
  }
  for(int i = pos; i < numKeys-1; i++)
  {
    void *p;
    GetKey(i+1, p);    
    SetKey(i, p);
    rids[i] = rids[i+1];
  }
  SetNumKeys(GetNumKeys()-1);
  if(numKeys == 0) return -1;
  return 0;
}

// inexact
// return position if key will fit in a particular position
// return (-1, -1) if there was an error
// if there are dups - this will return rightmost position
RID BtreeNode::FindAddrAtPosition(const void* &key) const
{
  assert(IsValid() == 0);
  int pos = FindKeyPosition(key);
  if (pos == -1 || pos >= numKeys) return RID(-1,-1);
  return rids[pos];
}

// inexact
// return position if key will fit in a particular position
// return -1 if there was an error
// if there are dups - this will return rightmost position
int BtreeNode::FindKeyPosition(const void* &key) const
{
  assert(IsValid() == 0);
  for(int i = numKeys-1; i >=0; i--)
  {
    void* k;
    if(GetKey(i, k) != 0)
      return -1;
// added == condition so that FindLeaf can return exact match and not
// the position to the right upon matches. this affects where inserts
// will happen during dups.
    if (CmpKey(key, k) == 0) 
      return i;
    if (CmpKey(key, k) > 0)
      return i+1;
   }
  return 0; // key is smaller than anything currently
}

// exact
// get rid for given position
// return (-1, -1) if there was an error or pos was not found
RID BtreeNode::GetAddr(const int pos) const
{
  assert(IsValid() == 0);
  if(pos < 0 || pos > numKeys)
    return RID(-1, -1);
  return rids[pos];
}

// exact
// return rid for exact key match
// return (-1, -1) if there was an error or key was not found
RID BtreeNode::FindAddr(const void* &key) const
{
  assert(IsValid() == 0);
  int pos = FindKey(key);
  if (pos == -1) return RID(-1,-1);
  return rids[pos];
}


// exact
// return position if key already exists at position
// if there are dups - returns rightmost position unless an RID is
// specified.
// if RID is specified, will only return a position if both key and
// RID match.
// return -1 if there was an error or if key does not exist
int BtreeNode::FindKey(const void* &key, const RID& r) const
{
  assert(IsValid() == 0);

  for(int i = numKeys-1; i >= 0; i--)
  {
    void* k;
    if(GetKey(i, k) != 0)
      return -1;
    if (CmpKey(key, k) == 0) {
      if(r == RID(-1,-1))
        return i;
      else { // match RID as well
        if (rids[i] == r)
          return i;
      }
    }
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
// split this node with rhs node
RC BtreeNode::Split(BtreeNode* rhs)
{
  assert(IsValid() == 0);
  assert(rhs->IsValid() == 0);

  // if (numKeys < order)
  //   return -1; // splitting too early - TODO

  // this node is full
  // shift higher keys to rhs
  int firstMovedPos = (numKeys+1)/2;
  int moveCount = (numKeys - firstMovedPos);
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
    if(rc < -1) { 
      return rc;
    }
  }

  // other side will have to be set up on the outside
  rhs->SetRight(this->GetRight());

  this->SetRight(rhs->GetPageRID().Page());
  rhs->SetLeft(this->GetPageRID().Page());
  
  

  assert(isSorted());
  assert(rhs->isSorted());

  assert(IsValid() == 0);
  assert(rhs->IsValid() == 0);
  return 0;
}

// return -1 on error, 0 on success
// merge this node with other node and put everything in this node
// "other" node has to be a neighbour
RC BtreeNode::Merge(BtreeNode* other) {
  assert(IsValid() == 0);
  assert(other->IsValid() == 0);

  if (numKeys + other->GetNumKeys() > order)
    return -1; // overflow will result from merge

  for (int pos = 0; pos < other->GetNumKeys(); pos++) {
    void * k = NULL; other->GetKey(pos, k);
    RID r = other->GetAddr(pos);
    RC rc = this->Insert(k, r);
    if(rc != 0) return rc;
  }

  int moveCount = other->GetNumKeys();
  for (int i = 0; i < moveCount; i++) {
    RC rc = other->Remove(NULL, 0);
    if(rc < -1) return rc;
  }

  if(this->GetPageRID().Page() == other->GetLeft())
    this->SetRight(other->GetRight());
  else
    this->SetLeft(other->GetLeft());

  assert(IsValid() == 0);
  assert(other->IsValid() == 0);
  return 0;
}

void BtreeNode::Print(ostream & os) {
  os << GetLeft() << "<--"
     << pageRID.Page() << "{";
  for (int pos = 0; pos < GetNumKeys(); pos++) {
    void * k = NULL; GetKey(pos, k);
    os << "(";
    if( attrType == INT )
      os << *((int*)k);
    if( attrType == FLOAT )
      os << *((float*)k);
    if( attrType == STRING ) {
      for(int i=0; i < attrLength; i++)
        os << ((char*)k)[i];
    }
    os << "," 
       << GetAddr(pos) << "), ";
  }
  if(numKeys > 0)
    os << "\b\b";
  os << "}" 
     << "-->" << GetRight() << "\n";
}
