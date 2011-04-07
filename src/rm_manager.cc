//
// File:        rm_manager.cc
// Description: RM_Manager class implementation
//

#include <cstdio>
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
   if(recordSize >= PF_PAGE_SIZE - sizeof(RID))
      return RM_SIZETOOBIG;

   int RC = pfm.CreateFile(fileName);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   
   PF_FileHandle pfh;
   RC = pfm.OpenFile(fileName, pfh);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   
   PF_PageHandle headerPage;
   RC = pfh.GetFirstPage(headerPage);
   if (RC < 0)
   {
      PF_PrintError(RC);
      return RM_PF;
   }
   //TODO - header page init
   //TODO - remove PF_PrintError or make it #define optional
   
   RC = pfm.CloseFile(pfh);
   // Return ok
   return (RC);
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
   return pfm.DestroyFile(fileName); 
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
RC RM_Manager::OpenFile (const char *fileName, RM_FileHandle &fileHandle)
{
   PF_FileHandle pfh;
   RC rc = pfm.OpenFile(fileName, pfh);
   if (rc != 0)
   {
      PF_PrintError(rc);
      return RM_PF;
   }
   RM_FileHandle rmh(&pfh);
   // TODO - put values into rmh
   // copy header into filehandle
   fileHandle = rmh;
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
RC RM_Manager::CloseFile(RM_FileHandle &fileHandle)
{
   PF_FileHandle pfh;
   RC rc = fileHandle.getPF_FileHandle(pfh);
   if (rc < 0) {
      return rc;
   }
   rc = pfm.CloseFile(pfh);
   return rc;
}


