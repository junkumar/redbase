#include "ix_index_handle.h"

IX_IndexHandle::IX_IndexHandle()
	:pfHandle(NULL), bFileOpen(false), bHdrChanged(false)
{
  root = NULL;
  path = NULL;
  pathP = NULL;
  treeLargest = NULL;
}

IX_IndexHandle::~IX_IndexHandle()
{
	if(pfHandle != NULL)
		delete pfHandle;
	if(root != NULL)
		delete root;
	if(pathP != NULL)
		delete [] pathP;
	if(path != NULL) {
    // path[0] is root
    for (int i = 1; i < hdr.height; i++) 
      if(path[i] != NULL) delete path[i];
    delete [] path;
  }
	if(treeLargest != NULL)
		delete [] (char*) treeLargest;
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID& rid)
{
  assert(IsValid() == 0);
  bool newLargest = false;
  void * prevKey = NULL;
  int level = hdr.height - 1;
  BtreeNode* node = FindLeaf(pData);
  BtreeNode* newNode = NULL;
  assert(node != NULL);

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
        assert("largest key should be everywhere\n");
        return IX_PF;
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

    // make new  node
    PF_PageHandle ph;
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0) return IX_PF;
    rc = pfHandle->GetThisPage(p, ph);
    if (rc != 0) return IX_PF;
  
    bool leaf = true;
    newNode = new BtreeNode(hdr.attrType, hdr.attrLength,
                            ph, true,
                            hdr.pageSize, hdr.dups,
                            leaf, true);
    // split into new node
    rc = node->Split(newNode);
    if (rc != 0) return IX_PF;
    

    BtreeNode * nodeInsertedInto = NULL;

    // put the new entry into one of the 2 now
    if(node->CmpKey(pData, node->LargestKey()) > 0) {
      newNode->Insert(failedKey, failedRid);
      nodeInsertedInto = newNode;
    }
    else { // <=
      node->Insert(failedKey, failedRid);
      nodeInsertedInto = node;
    }

    // go up to parent level and repeat
    level--;
    if(level < 0) break;
    // pos at which parent stores a pointer to this node
    int posAtParent = pathP[level];
    // cerr << "posAtParent was " << posAtParent << endl ;
    // cerr << "level was " << level << endl ;


    BtreeNode * parent = path[level];
    // update old key - keep same addr
    RID oldrid = parent->GetAddr(posAtParent);
    parent->Remove(NULL, posAtParent);
    result = parent->Insert((const void*)node->LargestKey(), oldrid);
    delete [] charPtr;

    // insert new key
    PageNum pp; ph.GetPageNum(pp);
    result = parent->Insert(newNode->LargestKey(), RID(pp,-1));
    
    // TODO - setup prev and next pointers for sibling nodes

    // iterate for parent node and split if required
    node = parent;
    failedKey = nodeInsertedInto->LargestKey();
    failedRid = RID(pp,-1);
  }  
  if(level >= 0) {
    // insertion done
    return 0;
  } else {
    // cerr << "root split happened" << endl;
    // make new root node
    PF_PageHandle ph;
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0) return IX_PF;
    rc = pfHandle->GetThisPage(p, ph);
    if (rc != 0) return IX_PF;
  
    bool leaf = false;
    root = new BtreeNode(hdr.attrType, hdr.attrLength,
                         ph, true,
                         hdr.pageSize, hdr.dups,
                         leaf, true);
    root->Insert(node->LargestKey(), node->GetPageRID());
    root->Insert(newNode->LargestKey(), newNode->GetPageRID());

    SetHeight(++hdr.height);
    return 0;
  }
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID& rid)
{
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
	pfHandle->GetThisPage(0, ph);
  // Needs to be called everytime GetThisPage is called.
	pfHandle->UnpinPage(0); 

	this->GetFileHeader(ph); // write into hdr member
	// std::cerr << "IX_FileHandle::Open hdr.numPages" << hdr.numPages << std::endl;

	PF_PageHandle rootph;

  bool newPage = true;
  if (hdr.height > 0) {
    SetHeight(hdr.height); // do all other init
    newPage = false;

    RC rc = pfHandle->GetThisPage(hdr.rootPage, rootph); 
    if (rc != 0) return rc;
  } else {
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0) return rc;
    rc = pfHandle->GetThisPage(p, rootph);
    if (rc != 0) return rc;
    hdr.rootPage = p;
    SetHeight(1); // do all other init
  } 

  bool leaf = (hdr.height == 1 ? true : false);
  root = new BtreeNode(hdr.attrType, hdr.attrLength,
                       rootph, newPage,
                       hdr.pageSize, hdr.dups,
                       leaf, true);
  path[0] = root;
  treeLargest = (void*) new char[hdr.attrLength];
  hdr.order = root->GetMaxKeys();
	bHdrChanged = true;
  assert(IsValid() == 0);
	return 0;
}

// get header from the first page of a newly opened file
RC IX_IndexHandle::GetFileHeader(PF_PageHandle ph)
{
	char * buf;
	RC rc = ph.GetData(buf);
	memcpy(&hdr, buf, sizeof(hdr));
	return rc;
}

// persist header into the first page of a file for later
RC IX_IndexHandle::SetFileHeader(PF_PageHandle ph) const 
{
	char * buf;
	RC rc = ph.GetData(buf);
	memcpy(buf, &hdr, sizeof(hdr));
	return rc;
}

// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk.  Default value forces all pages.
RC IX_IndexHandle::ForcePages ()
{
  assert(IsValid() == 0);
	return pfHandle->ForcePages(ALL_PAGES);
}

RC IX_IndexHandle::IsValid () const
{
  bool ret = true;
  ret = ret && (pfHandle != NULL);
  assert(ret);
  if(hdr.height > 0) {
    ret = ret && (hdr.rootPage > 0); // 0 is for file header
    assert(ret);
    ret = ret && (hdr.numPages >= hdr.height + 1);
    assert(ret);
    ret = ret && (root != NULL);
    assert(ret);
    ret = ret && (path != NULL);
    assert(ret);
  }
  return ret ? 0 : IX_BADIXPAGE;
}

RC IX_IndexHandle::GetNewPage(PageNum& pageNum) 
{
	assert(IsValid() == 0);
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

// return NULL if there is no root
// otherwise return a pointer to the leaf node that is leftmost OR
// smallest in value
// also populates the path member variable with the path
BtreeNode* IX_IndexHandle::FindSmallestLeaf()
{
  assert(IsValid() == 0);
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
    delete path[i];
    path[i] = FetchNode(r);
    pathP[i-1] = 0;
  }
  return path[hdr.height-1];
}

// return NULL if there is no root
// otherwise return a pointer to the leaf node where key might go
// also populates the path member variable with the path
BtreeNode* IX_IndexHandle::FindLeaf(const void *pData)
{
  assert(IsValid() == 0);
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
    delete path[i];
    path[i] = FetchNode(r);
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
  assert(IsValid() == 0);
  if(r.Page() < 0) return NULL;
  PF_PageHandle ph;
  RC rc = pfHandle->GetThisPage(r.Page(), ph);
  if(rc!=0) return NULL;

  //TODO remove isleaf & isroot
  return new BtreeNode(hdr.attrType, hdr.attrLength,
                       ph, false,
                       hdr.pageSize, hdr.dups,
                       false, false);
}

// Search an index entry
// return -ve if error
// 0 if found
// IX_KEYNOTFOUND if not found
// rid is populated if found
RC IX_IndexHandle::Search(void *pData, RID &rid)
{
  assert(IsValid() == 0);
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
  delete [] path;
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
  // level -1 signals first call to recursive function - root
  // os << "Print called with level " << level << endl;
  BtreeNode * node = NULL;
  if(level == -1) {
    level = hdr.height;
    node = root;
  }
  else {
    if(level < 1) {
      os << endl;
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

}
