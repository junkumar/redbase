#include "ix_index_handle.h"

IX_IndexHandle::IX_IndexHandle()
	:pfHandle(NULL), bFileOpen(false), bHdrChanged(false)
{
}

IX_IndexHandle::~IX_IndexHandle()
{
	if(pfHandle != NULL)
		delete pfHandle;
}

RC IX_IndexHandle::InsertEntry(void *pData, const RID& rid)
{
}

RC IX_IndexHandle::DeleteEntry(void *pData, const RID& rid)
{
}

RC IX_IndexHandle::Open(PF_FileHandle * pfh, int pairSize)
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

	{ // testing
			 char * pData;
			 ph.GetData(pData);
			 IX_FileHdr hdr;
			 memcpy(&hdr, pData, sizeof(hdr));
			 // std::cerr << "IX_FileHandle::Open inner hdr.numPages" << hdr.numPages << std::endl;
	}

	this->GetFileHeader(ph); // write into hdr
	// std::cerr << "IX_FileHandle::Open hdr.numPages" << hdr.numPages << std::endl;

	
	bHdrChanged = true;
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
	assert(pfHandle != NULL);
	return pfHandle->ForcePages(ALL_PAGES);
}

RC IX_IndexHandle::IsValid ()
{
  bool ret = true;
  ret = ret && (pfHandle != NULL);

  return ret ? 0 : IX_BADIXPAGE;
}

RC IX_FileHandle::GetNewPage(PageNum& pageNum) 
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
