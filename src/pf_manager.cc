//
// File:        pf_manager.cc
// Description: PF_Manager class implementation
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
#include <unistd.h>
#include <fcntl.h>
#include <sys/stat.h>
#include <sys/types.h>
#include "pf_internal.h"
#include "pf_buffermgr.h"

//
// PF_Manager
//
// Desc: Constructor - intended to be called once at begin of program
//       Handles creation, deletion, opening and closing of files.
//       It is associated with a PF_BufferMgr that manages the page
//       buffer and executes the page replacement policies.
//
PF_Manager::PF_Manager()
{
   // Create Buffer Manager
   pBufferMgr = new PF_BufferMgr(PF_BUFFER_SIZE);
}

//
// ~PF_Manager
//
// Desc: Destructor - intended to be called once at end of program
//       Destroys the buffer manager.
//       All files are expected to be closed when this method is called.
//
PF_Manager::~PF_Manager()
{
   // Destroy the buffer manager objects
   delete pBufferMgr;
}

//
// CreateFile
//
// Desc: Create a new PF file named fileName
// In:   fileName - name of file to create
// Ret:  PF return code
//
RC PF_Manager::CreateFile (const char *fileName)
{
   int fd;		// unix file descriptor
   int numBytes;		// return code form write syscall

   // Create file for exclusive use
   if ((fd = open(fileName,
#ifdef PC
         O_BINARY |
#endif
         O_CREAT | O_EXCL | O_WRONLY,
         CREATION_MASK)) < 0)
      return (PF_UNIX);

   // Initialize the file header: must reserve FileHdrSize bytes in memory
   // though the actual size of FileHdr is smaller
   char hdrBuf[PF_FILE_HDR_SIZE];

   // So that Purify doesn't complain
   memset(hdrBuf, 0, PF_FILE_HDR_SIZE);

   PF_FileHdr *hdr = (PF_FileHdr*)hdrBuf;
   hdr->firstFree = PF_PAGE_LIST_END;
   hdr->numPages = 0;

   // Write header to file
   if((numBytes = write(fd, hdrBuf, PF_FILE_HDR_SIZE))
         != PF_FILE_HDR_SIZE) {

      // Error while writing: close and remove file
      close(fd);
      unlink(fileName);

      // Return an error
      if(numBytes < 0)
         return (PF_UNIX);
      else
         return (PF_HDRWRITE);
   }

   // Close file
   if(close(fd) < 0)
      return (PF_UNIX);

   // Return ok
   return (0);
}

//
// DestroyFile
//
// Desc: Delete a PF file named fileName (fileName must exist and not be open)
// In:   fileName - name of file to delete
// Ret:  PF return code
//
RC PF_Manager::DestroyFile (const char *fileName)
{
   // Remove the file
   if (unlink(fileName) < 0)
      return (PF_UNIX);

   // Return ok
   return (0);
}

//
// OpenFile
//
// Desc: Open the paged file whose name is "fileName".  It is possible to open
//       a file more than once, however, it will be treated as 2 separate files
//       (different file descriptors; different buffers).  Thus, opening a file
//       more than once for writing may corrupt the file, and can, in certain
//       circumstances, crash the PF layer. Note that even if only one instance
//       of a file is for writing, problems may occur because some writes may
//       not be seen by a reader of another instance of the file.
// In:   fileName - name of file to open
// Out:  fileHandle - refer to the open file
//                    this function modifies local var's in fileHandle
//       to point to the file data in the file table, and to point to the
//       buffer manager object
// Ret:  PF_FILEOPEN or other PF return code
//
RC PF_Manager::OpenFile (const char *fileName, PF_FileHandle &fileHandle)
{
   int rc;                   // return code

   // Ensure file is not already open
   if (fileHandle.bFileOpen)
      return (PF_FILEOPEN);

   // Open the file
   if ((fileHandle.unixfd = open(fileName,
#ifdef PC
         O_BINARY |
#endif
         O_RDWR)) < 0)
      return (PF_UNIX);

   // Read the file header
   {
      int numBytes = read(fileHandle.unixfd, (char *)&fileHandle.hdr,
            sizeof(PF_FileHdr));
      if (numBytes != sizeof(PF_FileHdr)) {
         rc = (numBytes < 0) ? PF_UNIX : PF_HDRREAD;
         goto err;
      }
   }

   // Set file header to be not changed
   fileHandle.bHdrChanged = FALSE;

   // Set local variables in file handle object to refer to open file
   fileHandle.pBufferMgr = pBufferMgr;
   fileHandle.bFileOpen = TRUE;

   // Return ok
   return 0;

err:
   // Close file
   close(fileHandle.unixfd);
   fileHandle.bFileOpen = FALSE;

   // Return error
   return (rc);
}

//
// CloseFile
//
// Desc: Close file associated with fileHandle
//       The file should have been opened with OpenFile().
//       Also, flush all pages for the file from the page buffer
//       It is an error to close a file with pages still fixed in the buffer.
// In:   fileHandle - handle of file to close
// Out:  fileHandle - no longer refers to an open file
//                    this function modifies local var's in fileHandle
// Ret:  PF return code
//
RC PF_Manager::CloseFile(PF_FileHandle &fileHandle)
{
   RC rc;

   // Ensure fileHandle refers to open file
   if (!fileHandle.bFileOpen)
      return (PF_CLOSEDFILE);

   // Flush all buffers for this file and write out the header
   if ((rc = fileHandle.FlushPages()))
      return (rc);

   // Close the file
   if (close(fileHandle.unixfd) < 0)
      return (PF_UNIX);
   fileHandle.bFileOpen = FALSE;

   // Reset the buffer manager pointer in the file handle
   fileHandle.pBufferMgr = NULL;

   // Return ok
   return 0;
}

//
// ClearBuffer
//
// Desc: Remove all entries from the buffer manager.
//       This routine will be called via the system command and is only
//       really useful if the user wants to run some performance
//       comparison starting with an clean buffer.
// In:   Nothing
// Out:  Nothing
// Ret:  Returns the result of PF_BufferMgr::ClearBuffer
//       It is a code: 0 for success, something else for a PF error.
//
RC PF_Manager::ClearBuffer()
{
   return pBufferMgr->ClearBuffer();
}

//
// PrintBuffer
//
// Desc: Display all of the pages within the buffer.
//       This routine will be called via the system command.
// In:   Nothing
// Out:  Nothing
// Ret:  Returns the result of PF_BufferMgr::PrintBuffer
//       It is a code: 0 for success, something else for a PF error.
//
RC PF_Manager::PrintBuffer()
{
   return pBufferMgr->PrintBuffer();
}

//
// ResizeBuffer
//
// Desc: Resizes the buffer manager to the size passed in.
//       This routine will be called via the system command.
// In:   The new buffer size
// Out:  Nothing
// Ret:  Returns the result of PF_BufferMgr::ResizeBuffer
//       It is a code: 0 for success, PF_TOOSMALL when iNewSize
//       would be too small.
//
RC PF_Manager::ResizeBuffer(int iNewSize)
{
   return pBufferMgr->ResizeBuffer(iNewSize);
}

//------------------------------------------------------------------------------
// Three Methods for manipulating raw memory buffers.  These memory
// locations are handled by the buffer manager, but are not
// associated with a particular file.  These should be used if you
// want memory that is bounded by the size of the buffer pool.
//
// The PF_Manager just passes the calls down to the Buffer manager.
//------------------------------------------------------------------------------

RC PF_Manager::GetBlockSize(int &length) const
{
   return pBufferMgr->GetBlockSize(length);
}

RC PF_Manager::AllocateBlock(char *&buffer)
{
   return pBufferMgr->AllocateBlock(buffer);
}

RC PF_Manager::DisposeBlock(char *buffer)
{
   return pBufferMgr->DisposeBlock(buffer);
}
