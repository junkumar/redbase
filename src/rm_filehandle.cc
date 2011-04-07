//
// File:        rm_filehandle.cc
// Description: RM_Filehandle
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include <cassert>
#include "rm.h"

using namespace std;

// You will probably then want to copy the file header information into a private
// variable in the file handle that refers to the open file instance. By copying
// this information, you will subsequently be able to find out details such as the
// record size and the number of pages in the file by looking in the file handle
// instead of reading the header page again (or keeping the information on every
// page).

RM_FileHandle::RM_FileHandle() 
	:bFileOpen(false), bHdrChanged(false), pfHandle(NULL)
{
	memset(&hdr, 0, RM_FILE_HDR_SIZE);
  // TODO real hdr
}

RM_FileHandle::RM_FileHandle(PF_FileHandle* pfh) 
	:bFileOpen(true), bHdrChanged(false), pfHandle(pfh)
{	
	memset(&hdr, 0, RM_FILE_HDR_SIZE);
	hdr.firstFree = RM_PAGE_LIST_END;
	hdr.numPages = 0;
  // TODO real hdr
}

RC RM_FileHandle::getPF_FileHandle(PF_FileHandle &pfh) 
{
	assert(pfHandle != NULL);
	pfh = *pfHandle;
}

RM_FileHandle::~RM_FileHandle()
{
	assert(pfHandle != NULL);	
}

// Given a RID, return the record
RC RM_FileHandle::GetRec     (const RID &rid, RM_Record &rec) const 
{
	assert(pfHandle != NULL);
}


RC RM_FileHandle::InsertRec  (const char *pData, RID &rid)
{
	assert(pfHandle != NULL);
}

RC RM_FileHandle::DeleteRec  (const RID &rid)
{
	assert(pfHandle != NULL);
}
RC RM_FileHandle::UpdateRec  (const RM_Record &rec)
{
	assert(pfHandle != NULL);
}
// Forces a page (along with any contents stored in this class)
// from the buffer pool to disk.  Default value forces all pages.
RC RM_FileHandle::ForcePages (PageNum pageNum)
{
	assert(pfHandle != NULL);
}
