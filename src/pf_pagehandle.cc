//
// File:        pf_pagehandle.cc
// Description: PF_PageHandle class implementation
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//

#include "pf_internal.h"

//
// Defines
//
#define INVALID_PAGE   (-1)

//
// PF_PageHandle
//
// Desc: Default constructor for a page handle object
//       A page handle object provides access to the contents of a page
//       and the page's page number.  The page handle object is constructed
//       here but it must be passed to one of the PF_FileHandle methods to
//       have it refer to a pinned page before it can be used to access the
//       contents of a page.  Remember to call PF_FileHandle::UnpinPage()
//       to unpin the page when you are finished accessing it.
//
PF_PageHandle::PF_PageHandle()
{
  pageNum = INVALID_PAGE;
  pPageData = NULL;
}

//
// ~PF_PageHandle
//
// Desc: Destroy the page handle object.
//       If the page handle object refers to a pinned page, the page will
//       NOT be unpinned.
//
PF_PageHandle::~PF_PageHandle()
{
  // Don't need to do anything
}

//
// PF_PageHandle
//
// Desc: Copy constructor
//       If the incoming page handle object refers to a pinned page,
//       the page will NOT be pinned again.
// In:   pageHandle - page handle object from which to construct this object
//
PF_PageHandle::PF_PageHandle(const PF_PageHandle &pageHandle)
{
  // Just copy the local variables since there is no local memory
  // allocation involved
  this->pageNum = pageHandle.pageNum;
  this->pPageData = pageHandle.pPageData;
}

//
// operator=
//
// Desc: overload = operator
//       If the page handle object on the rhs refers to a pinned page,
//       the page will NOT be pinned again.
// In:   pageHandle - page handle object to set this object equal to
// Ret:  reference to *this
//
PF_PageHandle& PF_PageHandle::operator= (const PF_PageHandle &pageHandle)
{
  // Check for self-assignment
  if (this != &pageHandle) {

    // Just copy the pointers since there is no local memory
    // allocation involved
    this->pageNum = pageHandle.pageNum;
    this->pPageData = pageHandle.pPageData;
  }

  // Return a reference to this
  return (*this);
}

//
// GetData
//
// Desc: Access the contents of a page.  The page handle object must refer
//       to a pinned page.
// Out:  pData - Set pData to point to the page contents
// Ret:  PF return code
//
RC PF_PageHandle::GetData(char *&pData) const
{
  // Page must refer to a pinned page
  if (pPageData == NULL)
    return (PF_PAGEUNPINNED);

  // Set pData to point to page contents (after PF header)
  pData = pPageData;

  // Return ok
  return (0);
}

//
// GetPageNum
//
// Desc: Access the page number.  The page handle object must refer to
//       a pinned page.
// Out:  pageNum - contains the page number
// Ret:  PF return code
//
RC PF_PageHandle::GetPageNum(PageNum &_pageNum) const
{

  // Page must refer to a pinned page
  if (pPageData == NULL)
    return (PF_PAGEUNPINNED);

  // Set page number
  _pageNum = this->pageNum;

  // Return ok
  return (0);
}
