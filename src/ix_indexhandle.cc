#include "ix_indexhandle.h"
#include "rm_error.h"

IX_IndexHandle::IX_IndexHandle()
  :bFileOpen(false), pfHandle(NULL), bHdrChanged(false)
{
  root = NULL;
  path = NULL;
  pathP = NULL;
  treeLargest = NULL;
  hdr.height = 0;
}

IX_IndexHandle::~IX_IndexHandle()
{
  if(root != NULL && pfHandle != NULL) {
    // unpin old root page 
    pfHandle->UnpinPage(hdr.rootPage);
    delete root;
    root = NULL;
  }
  if(pathP != NULL) {
    delete [] pathP;
    pathP = NULL;
  }
  if(path != NULL) {
    // path[0] is root
    for (int i = 1; i < hdr.height; i++) 
      if(path[i] != NULL) {
        if(pfHandle != NULL)
          pfHandle->UnpinPage(path[i]->GetPageRID().Page());
        // delete path[i]; - better leak than crash
      }
    delete [] path;
    path = NULL;
  }
  if(pfHandle != NULL) {
    delete pfHandle;
    pfHandle = NULL;
  }
  if(treeLargest != NULL) {
    delete [] (char*) treeLargest;
    treeLargest = NULL;
  }
}

// 0 indicates success
RC IX_IndexHandle::InsertEntry(void *pData, const RID& rid)
{
  RC invalid = IsValid(); if(invalid) return invalid;
  if(pData == NULL) return IX_BADKEY;

  bool newLargest = false;
  void * prevKey = NULL;
  int level = hdr.height - 1;
  BtreeNode* node = FindLeaf(pData);
  BtreeNode* newNode = NULL;
  assert(node != NULL);

  int pos2 = node->FindKey((const void*&)pData, rid);
  if((pos2 != -1))
    return IX_ENTRYEXISTS;

  // largest key in tree is in every intermediate root from root
  // onwards !
  if (node->GetNumKeys() == 0 ||
      node->CmpKey(pData, treeLargest) > 0) {
    newLargest = true;
    prevKey = treeLargest;
  }

  int result = node->Insert(pData, rid);

  if(newLargest) {
    for(int i=0; i < hdr.height-1; i++) {
      int pos = path[i]->FindKey((const void *&)prevKey);
      if(pos != -1)
        path[i]->SetKey(pos, pData);
      else {
        // assert(false); //largest key should be everywhere
        // return IX_BADKEY;
        cerr << "Fishy intermediate node ?" << endl;
      }
    }
    // copy from pData into new treeLargest
    memcpy(treeLargest,
           pData,
           hdr.attrLength);
    // cerr << "new treeLargest " << *(int*)treeLargest << endl;
  }
  
  
  // no room in node - deal with overflow - non-root
  void * failedKey = pData;
  RID failedRid = rid;
  while(result == -1) 
  {
    // cerr << "non root overflow" << endl;

    char * charPtr = new char[hdr.attrLength];
    void * oldLargest = charPtr;

    if(node->LargestKey() == NULL)
      oldLargest = NULL;
    else
      node->CopyKey(node->GetNumKeys()-1, oldLargest);

    // cerr << "nro largestKey was " << *(int*)oldLargest  << endl;
    // cerr << "nro numKeys was " << node->GetNumKeys()  << endl;
    delete [] charPtr;

    // make new  node
    PF_PageHandle ph;
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0) return rc;
    rc = GetThisPage(p, ph);
    if (rc != 0) return rc;
  
    // rc = pfHandle->MarkDirty(p);
    // if(rc!=0) return NULL;

    newNode = new BtreeNode(hdr.attrType, hdr.attrLength,
                            ph, true,
                            hdr.pageSize);
    // split into new node
    rc = node->Split(newNode);
    if (rc != 0) return IX_PF;
    // split adjustment
    BtreeNode * currRight = FetchNode(newNode->GetRight());
    if(currRight != NULL) {
      currRight->SetLeft(newNode->GetPageRID().Page());
      delete currRight;
    }

    BtreeNode * nodeInsertedInto = NULL;

    // put the new entry into one of the 2 now.
    // In the comparison,
    // > takes care of normal cases
    // = is a hack for dups - this results in affinity for preserving
    // RID ordering for children - more balanced tree when all keys
    // are the same.
    if(node->CmpKey(pData, node->LargestKey()) >= 0) {
      newNode->Insert(failedKey, failedRid);
      nodeInsertedInto = newNode;
    }
    else { // <
      node->Insert(failedKey, failedRid);
      nodeInsertedInto = node;
    }

    // go up to parent level and repeat
    level--;
    if(level < 0) break; // root !
    // pos at which parent stores a pointer to this node
    int posAtParent = pathP[level];
    // cerr << "posAtParent was " << posAtParent << endl ;
    // cerr << "level was " << level << endl ;


    BtreeNode * parent = path[level];
    // update old key - keep same addr
    parent->Remove(NULL, posAtParent);
    result = parent->Insert((const void*)node->LargestKey(),
                            node->GetPageRID());
    // this result should always be good - we removed first before
    // inserting to prevent overflow.

    // insert new key
    result = parent->Insert(newNode->LargestKey(), newNode->GetPageRID());
    
    // iterate for parent node and split if required
    node = parent;
    failedKey = newNode->LargestKey(); // failure cannot be in node -
                                       // something was removed first.
    failedRid = newNode->GetPageRID();

    delete newNode;
    newNode = NULL;
  } // while

  if(level >= 0) {
    // insertion done
    return 0;
  } else {
    // cerr << "root split happened" << endl;

    // unpin old root page 
    RC rc = pfHandle->UnpinPage(hdr.rootPage);
    if (rc < 0) 
      return rc;

    // make new root node
    PF_PageHandle ph;
    PageNum p;
    rc = GetNewPage(p);
    if (rc != 0) return IX_PF;
    rc = GetThisPage(p, ph);
    if (rc != 0) return IX_PF;

    // rc = pfHandle->MarkDirty(p);
    // if(rc!=0) return NULL;

    root = new BtreeNode(hdr.attrType, hdr.attrLength,
                         ph, true,
                         hdr.pageSize);
    root->Insert(node->LargestKey(), node->GetPageRID());
    root->Insert(newNode->LargestKey(), newNode->GetPageRID());

    // pin root page - should always be valid
    hdr.rootPage = root->GetPageRID().Page();
    PF_PageHandle rootph;
    rc = pfHandle->GetThisPage(hdr.rootPage, rootph);
    if (rc != 0) return rc;

    if(newNode != NULL) {
      delete newNode;
      newNode = NULL;
    }

    SetHeight(hdr.height+1);
    return 0;
  }
}

// return NULL if key, rid is not found
BtreeNode* IX_IndexHandle::DupScanLeftFind(BtreeNode* right, void *pData, const RID& rid)
{

  BtreeNode* currNode = FetchNode(right->GetLeft());
  int currPos = -1;

  for( BtreeNode* j = currNode;
       j != NULL;
       j = FetchNode(j->GetLeft()))
  {
    currNode = j;
    int i = currNode->GetNumKeys()-1;

    for (; i >= 0; i--) 
    {
      currPos = i; // save Node in object state for later.
      char* key = NULL;
      int ret = currNode->GetKey(i, (void*&)key);
      if(ret == -1) 
        break;
      if(currNode->CmpKey(pData, key) < 0)
        return NULL;
      if(currNode->CmpKey(pData, key) > 0)
        return NULL; // should never happen
      if(currNode->CmpKey(pData, key) == 0) {
        if(currNode->GetAddr(currPos) == rid)
          return currNode;
      }
    }
  }
  return NULL;
}


// 0 indicates success
// Implements lazy deletion - underflow is defined as 0 keys for all
// non-root nodes.
RC IX_IndexHandle::DeleteEntry(void *pData, const RID& rid)
{
  if(pData == NULL)
    // bad input to method
    return IX_BADKEY;
  RC invalid = IsValid(); if(invalid) return invalid;

  bool nodeLargest = false;

  BtreeNode* node = FindLeaf(pData);
  assert(node != NULL);

  int pos = node->FindKey((const void*&)pData, rid);
  if(pos == -1) {
    
    // make sure that there are no dups (keys) left of this node that might
    // have this rid.
    int p = node->FindKey((const void*&)pData, RID(-1,-1));
    if(p != -1) {
      BtreeNode * other = DupScanLeftFind(node, pData, rid);
      if(other != NULL) {
        int pos = other->FindKey((const void*&)pData, rid);
        other->Remove(pData, pos); // ignore result - not dealing with
                                   // underflow here 
        return 0;
      }
    }

    // key/rid does not exist - error
    return IX_NOSUCHENTRY;
  }

  else if(pos == node->GetNumKeys()-1)
    nodeLargest = true;

  // Handle special case of key being largest and rightmost in
  // node. Means it is in parent and potentially whole path (every
  // intermediate node)
  if (nodeLargest) {
    // cerr << "node largest" << endl;
    // void * leftKey = NULL;
    // node->GetKey(node->GetNumKeys()-2, leftKey);
    // cerr << " left key " << *(int*)leftKey << endl;
    // if(node->CmpKey(pData, leftKey) != 0) {
    // replace this key with leftkey in every intermediate node
    // where it appears
    for(int i = hdr.height-2; i >= 0; i--) {
      int pos = path[i]->FindKey((const void *&)pData);
      if(pos != -1) {
        
        void * leftKey = NULL;
        leftKey = path[i+1]->LargestKey();
        if(node->CmpKey(pData, leftKey) == 0) {
          int pos = path[i+1]->GetNumKeys()-2;
          if(pos < 0) {
            continue; //underflow will happen
          }
          path[i+1]->GetKey(path[i+1]->GetNumKeys()-2, leftKey);
        }
        
        // if level is lower than leaf-1 then make sure that this is
        // the largest key
        if((i == hdr.height-2) || // leaf's parent level
           (pos == path[i]->GetNumKeys()-1) // largest at
           // intermediate node too 
          )
          path[i]->SetKey(pos, leftKey);
      }
    }
  }

  int result = node->Remove(pData, pos); // pos is the param that counts

  int level = hdr.height - 1; // leaf level
  while (result == -1) {

    // go up to parent level and repeat
    level--;
    if(level < 0) break; // root !

    // pos at which parent stores a pointer to this node
    int posAtParent = pathP[level];
    // cerr << "posAtParent was " << posAtParent << endl ;
    // cerr << "level was " << level << endl ;


    BtreeNode * parent = path[level];
    result = parent->Remove(NULL, posAtParent);
    // root is considered underflow even it is left with a single key
    if(level == 0 && parent->GetNumKeys() == 1 && result == 0)
      result = -1;

    // adjust L/R pointers because a node is going to be destroyed
    BtreeNode* left = FetchNode(node->GetLeft());
    BtreeNode* right = FetchNode(node->GetRight());
    if(left != NULL) {
      if(right != NULL)
        left->SetRight(right->GetPageRID().Page());
      else
        left->SetRight(-1);
    }
    if(right != NULL) {
      if(left != NULL)
        right->SetLeft(left->GetPageRID().Page());
      else
        right->SetLeft(-1);
    }
    if(right != NULL)
      delete right;
    if(left != NULL)
      delete left;

    node->Destroy();
    RC rc = DisposePage(node->GetPageRID().Page());
    if (rc < 0)
      return IX_PF;

    node = parent;
  } // end of while result == -1
  

  if(level >= 0) {
    // deletion done
    return 0;
  } else {
    // cerr << "root underflow" << endl;
    
    if(hdr.height == 1) { // root is also leaf
      return 0; //leave empty leaf-root around.
    }

    // new root is only child
    root = FetchNode(root->GetAddr(0));
    // pin root page - should always be valid
    hdr.rootPage = root->GetPageRID().Page();
    PF_PageHandle rootph;
    RC rc = pfHandle->GetThisPage(hdr.rootPage, rootph);
    if (rc != 0) return rc;

    node->Destroy();
    rc = DisposePage(node->GetPageRID().Page());
    if (rc < 0)
      return IX_PF;

    SetHeight(hdr.height-1); // do all other init
    return 0;
  }
}

RC IX_IndexHandle::Pin(PageNum p) {
  PF_PageHandle ph;
  RC rc = pfHandle->GetThisPage(p, ph); 
  return rc;
}

RC IX_IndexHandle::UnPin(PageNum p) {
  RC rc = pfHandle->UnpinPage(p); 
  return rc;
}

//Unpinning version that will unpin after every call correctly
RC IX_IndexHandle::GetThisPage(PageNum p, PF_PageHandle& ph) const {
  RC rc = pfHandle->GetThisPage(p, ph); 
  if (rc != 0) return rc;
  // Needs to be called everytime GetThisPage is called.
  rc = pfHandle->MarkDirty(p);
  if(rc!=0) return NULL;

  rc = pfHandle->UnpinPage(p);
  if (rc != 0) return rc;
  return 0;
}

RC IX_IndexHandle::Open(PF_FileHandle * pfh, int pairSize, 
                        PageNum rootPage, int pageSize)
{
  if(bFileOpen || pfHandle != NULL) {
    return IX_HANDLEOPEN; 
  }
  if(pfh == NULL ||
     pairSize <= 0) {
    return IX_BADOPEN;
  }
  bFileOpen = true;
  pfHandle = new PF_FileHandle;
  *pfHandle = *pfh ;

  PF_PageHandle ph;
  GetThisPage(0, ph);

  this->GetFileHeader(ph); // write into hdr member
  // std::cerr << "IX_FileHandle::Open hdr.numPages" << hdr.numPages << std::endl;

  PF_PageHandle rootph;

  bool newPage = true;
  if (hdr.height > 0) {
    SetHeight(hdr.height); // do all other init
    newPage = false;

    RC rc = GetThisPage(hdr.rootPage, rootph); 
    if (rc != 0) return rc;

  } else {
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0) return rc;
    rc = GetThisPage(p, rootph);
    if (rc != 0) return rc;
    hdr.rootPage = p;
    SetHeight(1); // do all other init
  } 

  // pin root page - should always be valid
  RC rc = pfHandle->GetThisPage(hdr.rootPage, rootph);
  if (rc != 0) return rc;

  // rc = pfHandle->MarkDirty(hdr.rootPage);
  // if(rc!=0) return NULL;

  root = new BtreeNode(hdr.attrType, hdr.attrLength,
                       rootph, newPage,
                       hdr.pageSize);
  path[0] = root;
  hdr.order = root->GetMaxKeys();
  bHdrChanged = true;
  RC invalid = IsValid(); if(invalid) return invalid;
  treeLargest = (void*) new char[hdr.attrLength];
  if(!newPage) {
    BtreeNode * node = FindLargestLeaf();
    // set treeLargest
    if(node->GetNumKeys() > 0)
      node->CopyKey(node->GetNumKeys()-1, treeLargest);
  }
  return 0;
}

// get header from the first page of a newly opened file
RC IX_IndexHandle::GetFileHeader(PF_PageHandle ph)
{
  char * buf;
  RC rc = ph.GetData(buf);
  if (rc != 0)
    return rc;
  memcpy(&hdr, buf, sizeof(hdr));
  return 0;
}

// persist header into the first page of a file for later
RC IX_IndexHandle::SetFileHeader(PF_PageHandle ph) const 
{
  char * buf;
  RC rc = ph.GetData(buf);
  if (rc != 0)
    return rc;
  memcpy(buf, &hdr, sizeof(hdr));
  return 0;
}

// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk.  Default value forces all pages.
RC IX_IndexHandle::ForcePages ()
{
  RC invalid = IsValid(); if(invalid) return invalid;
  return pfHandle->ForcePages(ALL_PAGES);
}

// Users will call - RC invalid = IsValid(); if(invalid) return invalid; 
RC IX_IndexHandle::IsValid () const
{
  bool ret = true;
  ret = ret && (pfHandle != NULL);
  if(hdr.height > 0) {
    ret = ret && (hdr.rootPage > 0); // 0 is for file header
    ret = ret && (hdr.numPages >= hdr.height + 1);
    ret = ret && (root != NULL);
    ret = ret && (path != NULL);
  }
  return ret ? 0 : IX_BADIXPAGE;
}

RC IX_IndexHandle::GetNewPage(PageNum& pageNum) 
{
  RC invalid = IsValid(); if(invalid) return invalid;
  PF_PageHandle ph;

  RC rc;
  if ((rc = pfHandle->AllocatePage(ph)) ||
      (rc = ph.GetPageNum(pageNum)))
    return(rc);
  
  // the default behavior of the buffer pool is to pin pages
  // let us make sure that we unpin explicitly after setting
  // things up
  if (
//  (rc = pfHandle->MarkDirty(pageNum)) ||
    (rc = pfHandle->UnpinPage(pageNum)))
    return rc;
  // cerr << "GetNewPage called to get page " << pageNum << endl;
  hdr.numPages++;
  assert(hdr.numPages > 1); // page 0 is this page in worst case
  bHdrChanged = true;
  return 0; // pageNum is set correctly
}

RC IX_IndexHandle::DisposePage(const PageNum& pageNum) 
{
  RC invalid = IsValid(); if(invalid) return invalid;

  RC rc;
  if ((rc = pfHandle->DisposePage(pageNum)))
    return(rc);
  
  hdr.numPages--;
  assert(hdr.numPages > 0); // page 0 is this page in worst case
  bHdrChanged = true;
  return 0; // pageNum is set correctly
}

// return NULL if there is no root
// otherwise return a pointer to the leaf node that is leftmost OR
// smallest in value
// also populates the path member variable with the path
BtreeNode* IX_IndexHandle::FindSmallestLeaf()
{
  RC invalid = IsValid(); if(invalid) return NULL;
  if (root == NULL) return NULL;
  RID addr;
  if(hdr.height == 1) {
    path[0] = root;
    return root;
  }

  for (int i = 1; i < hdr.height; i++) 
  {
    RID r = path[i-1]->GetAddr(0);
    if(r.Page() == -1) {
      // no such position or other error
      // no entries in node ?
      assert("should not run into empty node");
      return NULL;
    }
    // start with a fresh path
    if(path[i] != NULL) {
      RC rc = pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      if(rc < 0) {
        PrintErrorAll(rc);
      }
      if (rc < 0) return NULL;
      delete path[i];
      path[i] = NULL;
    }
    path[i] = FetchNode(r);
    PF_PageHandle dummy;
    // pin path pages
    RC rc = pfHandle->GetThisPage(path[i]->GetPageRID().Page(), dummy);
    if (rc != 0) return NULL;
    pathP[i-1] = 0; // dummy
  }
  return path[hdr.height-1];
}

// return NULL if there is no root
// otherwise return a pointer to the leaf node that is rightmost AND
// largest in value
// also populates the path member variable with the path
BtreeNode* IX_IndexHandle::FindLargestLeaf()
{
  RC invalid = IsValid(); if(invalid) return NULL;
  if (root == NULL) return NULL;
  RID addr;
  if(hdr.height == 1) {
    path[0] = root;
    return root;
  }

  for (int i = 1; i < hdr.height; i++) 
  {
    RID r = path[i-1]->GetAddr(path[i-1]->GetNumKeys() - 1);
    if(r.Page() == -1) {
      // no such position or other error
      // no entries in node ?
      assert("should not run into empty node");
      return NULL;
    }
    // start with a fresh path
    if(path[i] != NULL) {
      RC rc = pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      if (rc < 0) return NULL;
      delete path[i];
      path[i] = NULL;
    }
    path[i] = FetchNode(r);
    PF_PageHandle dummy;
    // pin path pages
    RC rc = pfHandle->GetThisPage(path[i]->GetPageRID().Page(), dummy);
    if (rc != 0) return NULL;
    pathP[i-1] = 0; // dummy
  }
  return path[hdr.height-1];
}

// return NULL if there is no root
// otherwise return a pointer to the leaf node where key might go
// also populates the path[] member variable with the path
// if there are dups (keys) along the path, the rightmost path will be
// chosen
// TODO - add a random parameter to this which will be used during
// inserts - this is prevent pure rightwards growth when inserting a
// dup value continuously.
BtreeNode* IX_IndexHandle::FindLeaf(const void *pData)
{
  RC invalid = IsValid(); if(invalid) return NULL;
  if (root == NULL) return NULL;
  RID addr;
  if(hdr.height == 1) {
    path[0] = root;
    return root;
  }

  for (int i = 1; i < hdr.height; i++) 
  {
    // cerr << "i was " << i << endl;
    // cerr << "pData was " << *(int*)pData << endl;

    RID r = path[i-1]->FindAddrAtPosition(pData);
    int pos = path[i-1]->FindKeyPosition(pData);
    if(r.Page() == -1) {
      // pData is bigger than any other key - return address of node
      // that largest key points to.
      const void * p = path[i-1]->LargestKey();
      // cerr << "p was " << *(int*)p << endl;
      r = path[i-1]->FindAddr((const void*&)(p));
      // cerr << "r was " << r << endl;
      pos = path[i-1]->FindKey((const void*&)(p));
    }
    // start with a fresh path
    if(path[i] != NULL) {
      RC rc = pfHandle->UnpinPage(path[i]->GetPageRID().Page());
      // if (rc != 0) return NULL;
      delete path[i];
      path[i] = NULL;
    }
    path[i] = FetchNode(r.Page());
    PF_PageHandle dummy;
    // pin path pages
    
    RC rc = pfHandle->GetThisPage(path[i]->GetPageRID().Page(), dummy);
    if (rc != 0) return NULL;

    pathP[i-1] = pos;
  }



  return path[hdr.height-1];
}

// Get the BtreeNode at the PageNum specified within Btree
// SlotNum in RID does not matter anyway
BtreeNode* IX_IndexHandle::FetchNode(PageNum p) const
{
  return FetchNode(RID(p,-1));
}

// Get the BtreeNode at the RID specified within Btree
BtreeNode* IX_IndexHandle::FetchNode(RID r) const
{
  RC invalid = IsValid(); if(invalid) return NULL;
  if(r.Page() < 0) return NULL;
  PF_PageHandle ph;
  RC rc = GetThisPage(r.Page(), ph);
  if(rc != 0) { 
    PrintErrorAll(rc);
  }
  if(rc!=0) return NULL;

  // rc = pfHandle->MarkDirty(r.Page());
  // if(rc!=0) return NULL;

  return new BtreeNode(hdr.attrType, hdr.attrLength,
                       ph, false,
                       hdr.pageSize);
}

// Search an index entry
// return -ve if error
// 0 if found
// IX_KEYNOTFOUND if not found
// rid is populated if found
RC IX_IndexHandle::Search(void *pData, RID &rid)
{
  RC invalid = IsValid(); if(invalid) return invalid;
  if(pData == NULL)
    return IX_BADKEY;
  BtreeNode * leaf = FindLeaf(pData);
  if(leaf == NULL)
    return IX_BADKEY;
  rid = leaf->FindAddr((const void*&)pData);
  if(rid == RID(-1, -1))
    return IX_KEYNOTFOUND;
  return 0;
}

// get/set height
int IX_IndexHandle::GetHeight() const
{
  return hdr.height;
}
void IX_IndexHandle::SetHeight(const int& h)
{
  for(int i = 1;i < hdr.height; i++)
    if (path != NULL && path[i] != NULL) {
      delete path[i];
      path[i] = NULL;
    }
  if(path != NULL) delete [] path;
  if(pathP != NULL) delete [] pathP;

  hdr.height = h;
  path = new BtreeNode* [hdr.height];
  for(int i=1;i < hdr.height; i++)
    path[i] = NULL;
  path[0] = root;

  pathP = new int [hdr.height-1]; // leaves don't point
  for(int i=0;i < hdr.height-1; i++)
    pathP[i] = -1;
}

BtreeNode* IX_IndexHandle::GetRoot() const
{
  return root;
}

void IX_IndexHandle::Print(ostream & os, int level, RID r) const {
  RC invalid = IsValid(); if(invalid) assert(invalid);
  // level -1 signals first call to recursive function - root
  // os << "Print called with level " << level << endl;
  BtreeNode * node = NULL;
  if(level == -1) {
    level = hdr.height;
    node = FetchNode(root->GetPageRID());
  }
  else {
    if(level < 1) {
      return;
    }
    else
    {
      node = FetchNode(r);
    }
  }

  node->Print(os);
  if (level >= 2) // non leaf
  {
    for(int i = 0; i < node->GetNumKeys(); i++)
    {
      RID newr = node->GetAddr(i);
      Print(os, level-1, newr);
    }
  } 
  // else level == 1 - recursion ends
  if(level == 1 && node->GetRight() == -1)
    os << endl; //blank after rightmost leaf
  if(node!=NULL) delete node;
}

// hack for indexscan::OpOptimize
// FindLeaf() does not really return rightmost node that has a key. This happens
// when there are duplicates that span multiple btree nodes.
// The strict rightmost guarantee is mainly required for
// IX_IndexScan::OpOptimize()
// In terms of usage this method is a drop-in replacement for FindLeaf() and will
// call FindLeaf() in turn.
BtreeNode* IX_IndexHandle::FindLeafForceRight(const void* pData)
{
  FindLeaf(pData);
  //see if duplicates for this value exist in the next node to the right and
  //return that node if true.
  // have a right node ?
  if(path[hdr.height-1]->GetRight() != -1) {
    // cerr << "bingo: dups to the right at leaf " << *(int*)pData << " withR\n";

    // at last position in node ?
    int pos = path[hdr.height-1]->FindKey(pData);
    if ( pos != -1 &&
         pos == path[hdr.height-1]->GetNumKeys() - 1) {

      // cerr << "bingo: dups to the right at leaf " << *(int*)pData << " lastPos\n";
      // cerr << "bingo: right page at " << path[hdr.height-1]->GetRight() << "\n";

      BtreeNode* r = FetchNode(path[hdr.height-1]->GetRight());
      if( r != NULL) {
        // cerr << "bingo: dups to the right at leaf " << *(int*)pData << " nonNUllR\n";

        void* k = NULL;
        r->GetKey(0, k);
        if(r->CmpKey(k, pData) == 0) {
          // cerr << "bingo: dups to the right at leaf " << *(int*)pData << "\n";

          RC rc = pfHandle->UnpinPage(path[hdr.height-1]->GetPageRID().Page());
          if(rc < 0) {
            PrintErrorAll(rc);
          }
          if (rc < 0) return NULL;
          delete path[hdr.height-1];
          path[hdr.height-1] = FetchNode(r->GetPageRID());
          pathP[hdr.height-2]++;
        }
      }
    }
  }
  return path[hdr.height-1];
}
