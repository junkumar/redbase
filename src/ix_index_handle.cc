#include "ix_index_handle.h"

IX_IndexHandle::IX_IndexHandle()
	:pfHandle(NULL), bFileOpen(false), bHdrChanged(false)
{
  root = NULL;
  path = NULL;
}

IX_IndexHandle::~IX_IndexHandle()
{
	if(pfHandle != NULL)
		delete pfHandle;
	if(root != NULL)
		delete root;
	if(path != NULL) {
    // path[0] is root
    for (int i = 1; i < hdr.height; i++) 
      if(path[i] != NULL) delete path[i];
    delete [] path;
  }
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

  // largest key in tree
  void * largest = node->LargestKey();
  if (largest == NULL ||
      node->CmpKey(pData, largest) > 0) {
    newLargest = true;
    prevKey = largest;
  }

  int result = node->Insert(pData, rid);
  
  if(newLargest) {
    for(int i=0; i < hdr.height-1; i++) {
      int pos = path[i]->FindKeyPosition((const void *&)prevKey);
      path[i]->SetKey(pos, pData);
    }
  }
  
  // no room in node - deal with overflow - non-root
  while(result == -1) 
  {
    cerr << "non root overflow" << endl;
    void * largestKey = node->LargestKey();
    
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
                            PF_PAGE_SIZE, hdr.dups,
                            leaf, true);
    hdr.numPages++;
    
    // split into new node
    node->Split(newNode);
    
    // go up to parent level and repeat
    level--;
    if(level < 0) break;

    BtreeNode * parent = path[level];
    // update old key - keep same addr
    int pos = parent->FindKey((const void*&)largestKey);
    cerr << "pos was " << pos << endl;
    cerr << "largestKey was " << *(int*)largestKey  << endl;
    cerr << "parent largestKey was " << *(int*)parent->LargestKey()  << endl;
    result = parent->SetKey(pos, node->LargestKey());
    // insert new key
    PageNum pp; ph.GetPageNum(pp);
    result = parent->Insert(newNode->LargestKey(), RID(pp,-1));
    
    // TODO - setup prev and next pointers for sibling nodes

    // iterate for parent node and split if required
    node = parent;
  }
  
  if(level >= 0) {
    // insertion done
    return 0;
  } else {
    cerr << "root split happened" << endl;
    // make new root node
    PF_PageHandle ph;
    PageNum p;
    RC rc = GetNewPage(p);
    if (rc != 0) return IX_PF;
    rc = pfHandle->GetThisPage(p, ph);
    if (rc != 0) return IX_PF;
  
    bool leaf = false;
    RID oldRID = root->GetPageRID();
    root = new BtreeNode(hdr.attrType, hdr.attrLength,
                         ph, true,
                         PF_PAGE_SIZE, hdr.dups,
                         leaf, true);
    root->Insert(node->LargestKey(), oldRID);
    root->Insert(node->LargestKey(), newNode->GetPageRID());

    hdr.numPages++;
    SetHeight(hdr.height++);
    return 0;
  }
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID& rid)
{
}

RC IX_IndexHandle::Open(PF_FileHandle * pfh, int pairSize, PageNum rootPage)
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
                       PF_PAGE_SIZE, hdr.dups,
                       leaf, true);
  path[0] = root;
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

  hdr.numPages++;
  assert(hdr.numPages > 1); // page 0 is this page in worst case
  bHdrChanged = true;
  return 0; // pageNum is set correctly
}

// return NULL if there is not root
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
    RID r = path[i-1]->FindAddrAtPosition(pData);
    bool leaf = (hdr.height-1 == i ? true : false );
    
    PF_PageHandle ph;
    pfHandle->GetThisPage(r.Page(), ph);

    // start with a fresh path
    delete path[i];

    path[i] = new BtreeNode(hdr.attrType, hdr.attrLength,
                            ph, false,
                            PF_PAGE_SIZE, hdr.dups,
                            leaf, false);
  }
  return path[hdr.height-1];
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
}

