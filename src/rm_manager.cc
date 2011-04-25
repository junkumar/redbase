//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
//

#include <cstdio>
#include <cstring>
#include <cassert>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "rm.h"

//
// RM_Manager
//
// Desc: Constructor - intended to be called once at begin of program
//       Handles creation, deletion, opening and closing of files.
//
RM_Manager::RM_Manager(PF_Manager & pfm):
  pfm(pfm)
{
  
}

//
// ~RM_Manager
//
// Desc: Destructor - intended to be called once at end of program
//       All files are expected to be closed when this method is called.
//
RM_Manager::~RM_Manager()
{

}

//
// CreateFile
//
// Desc: Create a new RM table/file named fileName
// with recordSize as the fixed size of records.
// In:   fileName - name of file to create
// In:   recordSize
// Ret:  RM return code
//
RC RM_Manager::CreateFile (const char *fileName, int recordSize)
{
   if(recordSize >= PF_PAGE_SIZE - (int)sizeof(RM_PageHdr))
      return RM_SIZETOOBIG;

   if(recordSize <= 0)
      return RM_BADRECSIZE;

   int RC = pfm.CreateFile(fileName);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RC;
   }
   
   PF_FileHandle pfh;
   RC = pfm.OpenFile(fileName, pfh);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RC;
   }
   
   PF_PageHandle headerPage;
   char * pData;
   
   RC = pfh.AllocatePage(headerPage);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   RC = headerPage.GetData(pData);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   RM_FileHdr hdr;
   hdr.firstFree = RM_PAGE_LIST_END;
   hdr.numPages = 1; // hdr page
   hdr.extRecordSize = recordSize;

   memcpy(pData, &hdr, sizeof(hdr));
   //TODO - remove PF_PrintError or make it #define optional
   PageNum headerPageNum;
   headerPage.GetPageNum(headerPageNum);
   assert(headerPageNum == 0);
   RC = pfh.MarkDirty(headerPageNum);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   RC = pfh.UnpinPage(headerPageNum);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }  
   RC = pfm.CloseFile(pfh);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   return (0);
}

//
// DestroyFile
//
// Desc: Delete a RM file named fileName (fileName must exist and not be open)
// In:   fileName - name of file to delete
// Ret:  RM return code
//
RC RM_Manager::DestroyFile (const char *fileName)
{
   RC RC = pfm.DestroyFile(fileName); 
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   return 0;
}

//
// OpenFile
//
// In:   fileName - name of file to open
// Out:  fileHandle - refer to the open file
//                    this function modifies local var's in fileHandle
//       to point to the file data in the file table, and to point to the
//       buffer manager object
// Ret:  PF_FILEOPEN or other RM return code
//
RC RM_Manager::OpenFile (const char *fileName, RM_FileHandle &rmh)
{
   PF_FileHandle pfh;
   RC rc = pfm.OpenFile(fileName, pfh);
   if (rc < 0)
   {
      PF_PrintError(rc);
      return RM_PF;
   }
   // header page is at 0
   PF_PageHandle ph;
   char * pData;
   if ((rc = pfh.GetThisPage(0, ph)) ||
       (rc = ph.GetData(pData)))
      return(rc);
   RM_FileHdr hdr;
   memcpy(&hdr, pData, sizeof(hdr));
   rc = rmh.Open(&pfh, hdr.extRecordSize);
   if (rc < 0)
   {
      RM_PrintError(rc);
      return rc;
   }
   rc = pfh.UnpinPage(0);
   if (rc < 0)
   {
      PF_PrintError(rc);
      return rc;
   }

   return 0;
}

//
// CloseFile
//
// Desc: Close file associated with fileHandle
//       The file should have been opened with OpenFile().
// In:   fileHandle - handle of file to close
// Out:  fileHandle - no longer refers to an open file
//                    this function modifies local var's in fileHandle
// Ret:  RM return code
//
RC RM_Manager::CloseFile(RM_FileHandle &rfileHandle)
{
   if(!rfileHandle.bFileOpen || rfileHandle.pfHandle == NULL)
      return RM_FNOTOPEN;

   if(rfileHandle.hdrChanged())
   {
      // write header to disk
     PF_PageHandle ph;
     rfileHandle.pfHandle->GetThisPage(0, ph);
     rfileHandle.SetFileHeader(ph); // write hdr into file

     RC rc = rfileHandle.pfHandle->MarkDirty(0);
     if (rc < 0)
     {
       PF_PrintError(rc);
       return rc;
     }

     rc = rfileHandle.pfHandle->UnpinPage(0);
     if (rc < 0)
     {
       PF_PrintError(rc);
       return rc;
     }

     rc = rfileHandle.ForcePages();
     if (rc < 0)
     {
       RM_PrintError(rc);
       return rc;
     }
   }
      
   //PF_FileHandle pfh;
   // RC rc = rfileHandle.GetPF_FileHandle(pfh);
   // if (rc < 0) {
   //     RM_PrintError(rc);
   //     return rc;
   // }
   RC rc2 = pfm.CloseFile(*rfileHandle.pfHandle);
   if (rc2 < 0) {
      PF_PrintError(rc2);
      return rc2;
   }
   // TODO - is there a cleaner way than reaching into innards like this ?
   delete rfileHandle.pfHandle;
   rfileHandle.pfHandle = NULL;
   rfileHandle.bFileOpen = false;
   return 0;
}
