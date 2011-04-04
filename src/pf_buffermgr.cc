//
// File:        pf_buffermgr.cc
// Description: PF_BufferMgr class implementation
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//              Jason McHugh (mchughj@cs.stanford.edu)
//
// 1997: If PF_LOG is defined then a log file with the sequence of calls
//       to the buffer manager is maintained.
//       If PF_STATS is defined then a Statistics manager now tracks some
//       relevant stats.  This differs from PF_LOG in that it can give a
//       summary of the calls.  See statistics.h for interface and
//       pf_test2.cc for a demo.
// 1998: The statistics manager is now instantiated in this file and is
//       created and destroyed by the buffer manager.
//

#include <cstdio>
#include <unistd.h>
#include <iostream>
#include "pf_buffermgr.h"

using namespace std;

// The switch PF_STATS indicates that the user wishes to have statistics
// tracked for the PF layer
#ifdef PF_STATS
#include "statistics.h"   // For StatisticsMgr interface

// Global variable for the statistics manager
StatisticsMgr *pStatisticsMgr;
#endif

#ifdef PF_LOG

//
// WriteLog
//
// This is a self contained unit that will create a new log file and send
// psMessage to the log file.  Notice that I do not close the file fLog at
// any time.  Hopefully if all goes well this will be done when the program
// exits.
//
void WriteLog(const char *psMessage)
{
   static FILE *fLog = NULL;

   // The first time through we have to create a new Log file
   if (fLog == NULL) {
      // This is the first time so I need to create a new log file.
      // The log file will be named "PF_LOG.x" where x is the next
      // available sequential number
      int iLogNum = -1;
      int bFound = FALSE;
      char psFileName[10];

      while (iLogNum < 999 && bFound==FALSE) {
         iLogNum++;
         sprintf (psFileName, "PF_LOG.%d", iLogNum);
         fLog = fopen(psFileName,"r");
         if (fLog==NULL) {
            bFound = TRUE;
            fLog = fopen(psFileName,"w");
         } else
            delete fLog;
      }

      if (!bFound) {
         cerr << "Cannot create a new log file!\n";
         exit(1);
      }
   }
   // Now we have the log file open and ready for writing
   fprintf (fLog, psMessage);
}
#endif


//
// PF_BufferMgr
//
// Desc: Constructor - called by PF_Manager::PF_Manager
//       The buffer manager manages the page buffer.  When asked for a page,
//       it checks if it is in the buffer.  If so, it pins the page (pages
//       can be pinned multiple times).  If not, it reads it from the file
//       and pins it.  If the buffer is full and a new page needs to be
//       inserted, an unpinned page is replaced according to an LRU
// In:   numPages - the number of pages in the buffer
//
// Note: The constructor will initialize the global pStatisticsMgr.  We
//       make it global so that other components may use it and to allow
//       easy access.
//
// Aut2003
// numPages changed to _numPages for to eliminate CC warnings

PF_BufferMgr::PF_BufferMgr(int _numPages) : hashTable(PF_HASH_TBL_SIZE)
{
   // Initialize local variables
   this->numPages = _numPages;
   pageSize = PF_PAGE_SIZE + sizeof(PF_PageHdr);

#ifdef PF_STATS
   // Initialize the global variable for the statistics manager
   pStatisticsMgr = new StatisticsMgr();
#endif

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Creating buffer manager. %d pages of size %d.\n",
         numPages, PF_PAGE_SIZE+sizeof(PF_PageHdr));
   WriteLog(psMessage);
#endif

   // Allocate memory for buffer page description table
   bufTable = new PF_BufPageDesc[numPages];

   // Initialize the buffer table and allocate memory for buffer pages.
   // Initially, the free list contains all pages
   for (int i = 0; i < numPages; i++) {
      if ((bufTable[i].pData = new char[pageSize]) == NULL) {
         cerr << "Not enough memory for buffer\n";
         exit(1);
      }

      memset ((void *)bufTable[i].pData, 0, pageSize);

      bufTable[i].prev = i - 1;
      bufTable[i].next = i + 1;
   }
   bufTable[0].prev = bufTable[numPages - 1].next = INVALID_SLOT;
   free = 0;
   first = last = INVALID_SLOT;

#ifdef PF_LOG
   WriteLog("Succesfully created the buffer manager.\n");
#endif
}

//
// ~PF_BufferMgr
//
// Desc: Destructor - called by PF_Manager::~PF_Manager
//
PF_BufferMgr::~PF_BufferMgr()
{
   // Free up buffer pages and tables
   for (int i = 0; i < this->numPages; i++)
      delete [] bufTable[i].pData;

   delete [] bufTable;

#ifdef PF_STATS
   // Destroy the global statistics manager
   delete pStatisticsMgr;
#endif

#ifdef PF_LOG
   WriteLog("Destroyed the buffer manager.\n");
#endif
}

//
// GetPage
//
// Desc: Get a pointer to a page pinned in the buffer.  If the page is
//       already in the buffer, (re)pin the page and return a pointer
//       to it.  If the page is not in the buffer, read it from the file,
//       pin it, and return a pointer to it.  If the buffer is full,
//       replace an unpinned page.
// In:   fd - OS file descriptor of the file to read
//       pageNum - number of the page to read
//       bMultiplePins - if FALSE, it is an error to ask for a page that is
//                       already pinned in the buffer.
// Out:  ppBuffer - set *ppBuffer to point to the page in the buffer
// Ret:  PF return code
//
RC PF_BufferMgr::GetPage(int fd, PageNum pageNum, char **ppBuffer,
      int bMultiplePins)
{
   RC  rc;     // return code
   int slot;   // buffer slot where page is located

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Looking for (%d,%d).\n", fd, pageNum);
   WriteLog(psMessage);
#endif


#ifdef PF_STATS
   pStatisticsMgr->Register(PF_GETPAGE, STAT_ADDONE);
#endif

   // Search for page in buffer
   if ((rc = hashTable.Find(fd, pageNum, slot)) &&
         (rc != PF_HASHNOTFOUND))
      return (rc);                // unexpected error

   // If page not in buffer...
   if (rc == PF_HASHNOTFOUND) {

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_PAGENOTFOUND, STAT_ADDONE);
#endif

      // Allocate an empty page, this will also promote the newly allocated
      // page to the MRU slot
      if ((rc = InternalAlloc(slot)))
         return (rc);

      // read the page, insert it into the hash table,
      // and initialize the page description entry
      if ((rc = ReadPage(fd, pageNum, bufTable[slot].pData)) ||
            (rc = hashTable.Insert(fd, pageNum, slot)) ||
            (rc = InitPageDesc(fd, pageNum, slot))) {

         // Put the slot back on the free list before returning the error
         Unlink(slot);
         InsertFree(slot);
         return (rc);
      }
#ifdef PF_LOG
   WriteLog("Page not found in buffer. Loaded.\n");
#endif
   }
   else {   // Page is in the buffer...

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_PAGEFOUND, STAT_ADDONE);
#endif

      // Error if we don't want to get a pinned page
      if (!bMultiplePins && bufTable[slot].pinCount > 0)
         return (PF_PAGEPINNED);

      // Page is alredy in memory, just increment pin count
      bufTable[slot].pinCount++;
#ifdef PF_LOG
      sprintf (psMessage, "Page found in buffer.  %d pin count.\n",
            bufTable[slot].pinCount);
      WriteLog(psMessage);
#endif

      // Make this page the most recently used page
      if ((rc = Unlink(slot)) ||
            (rc = LinkHead (slot)))
         return (rc);
   }

   // Point ppBuffer to page
   *ppBuffer = bufTable[slot].pData;

   // Return ok
   return (0);
}

//
// AllocatePage
//
// Desc: Allocate a new page in the buffer and return a pointer to it.
// In:   fd - OS file descriptor of the file associated with the new page
//       pageNum - number of the new page
// Out:  ppBuffer - set *ppBuffer to point to the page in the buffer
// Ret:  PF return code
//
RC PF_BufferMgr::AllocatePage(int fd, PageNum pageNum, char **ppBuffer)
{
   RC  rc;     // return code
   int slot;   // buffer slot where page is located

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Allocating a page for (%d,%d)....", fd, pageNum);
   WriteLog(psMessage);
#endif

   // If page is already in buffer, return an error
   if (!(rc = hashTable.Find(fd, pageNum, slot)))
      return (PF_PAGEINBUF);
   else if (rc != PF_HASHNOTFOUND)
      return (rc);              // unexpected error

   // Allocate an empty page
   if ((rc = InternalAlloc(slot)))
      return (rc);

   // Insert the page into the hash table,
   // and initialize the page description entry
   if ((rc = hashTable.Insert(fd, pageNum, slot)) ||
         (rc = InitPageDesc(fd, pageNum, slot))) {

      // Put the slot back on the free list before returning the error
      Unlink(slot);
      InsertFree(slot);
      return (rc);
   }

#ifdef PF_LOG
   WriteLog("Succesfully allocated page.\n");
#endif

   // Point ppBuffer to page
   *ppBuffer = bufTable[slot].pData;

   // Return ok
   return (0);
}

//
// MarkDirty
//
// Desc: Mark a page dirty so that when it is discarded from the buffer
//       it will be written back to the file.
// In:   fd - OS file descriptor of the file associated with the page
//       pageNum - number of the page to mark dirty
// Ret:  PF return code
//
RC PF_BufferMgr::MarkDirty(int fd, PageNum pageNum)
{
   RC  rc;       // return code
   int slot;     // buffer slot where page is located

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Marking dirty (%d,%d).\n", fd, pageNum);
   WriteLog(psMessage);
#endif

   // The page must be found and pinned in the buffer
   if ((rc = hashTable.Find(fd, pageNum, slot))){
      if ((rc == PF_HASHNOTFOUND))
         return (PF_PAGENOTINBUF);
      else
         return (rc);              // unexpected error
   }

   if (bufTable[slot].pinCount == 0)
      return (PF_PAGEUNPINNED);

   // Mark this page dirty
   bufTable[slot].bDirty = TRUE;

   // Make this page the most recently used page
   if ((rc = Unlink(slot)) ||
         (rc = LinkHead (slot)))
      return (rc);

   // Return ok
   return (0);
}

//
// UnpinPage
//
// Desc: Unpin a page so that it can be discarded from the buffer.
// In:   fd - OS file descriptor of the file associated with the page
//       pageNum - number of the page to unpin
// Ret:  PF return code
//
RC PF_BufferMgr::UnpinPage(int fd, PageNum pageNum)
{
   RC  rc;       // return code
   int slot;     // buffer slot where page is located

   // The page must be found and pinned in the buffer
   if ((rc = hashTable.Find(fd, pageNum, slot))){
      if ((rc == PF_HASHNOTFOUND))
         return (PF_PAGENOTINBUF);
      else
         return (rc);              // unexpected error
   }

   if (bufTable[slot].pinCount == 0)
      return (PF_PAGEUNPINNED);

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Unpinning (%d,%d). %d Pin count\n",
         fd, pageNum, bufTable[slot].pinCount-1);
   WriteLog(psMessage);
#endif

   // If unpinning the last pin, make it the most recently used page
   if (--(bufTable[slot].pinCount) == 0) {
      if ((rc = Unlink(slot)) ||
            (rc = LinkHead (slot)))
         return (rc);
   }

   // Return ok
   return (0);
}

//
// FlushPages
//
// Desc: Release all pages for this file and put them onto the free list
//       Returns a warning if any of the file's pages are pinned.
//       A linear search of the buffer is performed.
//       A better method is not needed because # of buffers are small.
// In:   fd - file descriptor
// Ret:  PF_PAGEPINNED or other PF return code
//
RC PF_BufferMgr::FlushPages(int fd)
{
   RC rc, rcWarn = 0;  // return codes

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Flushing all pages for (%d).\n", fd);
   WriteLog(psMessage);
#endif

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_FLUSHPAGES, STAT_ADDONE);
#endif

   // Do a linear scan of the buffer to find pages belonging to the file
   int slot = first;
   while (slot != INVALID_SLOT) {

      int next = bufTable[slot].next;

      // If the page belongs to the passed-in file descriptor
      if (bufTable[slot].fd == fd) {

#ifdef PF_LOG
 sprintf (psMessage, "Page (%d) is in buffer manager.\n", bufTable[slot].pageNum);
 WriteLog(psMessage);
#endif
         // Ensure the page is not pinned
         if (bufTable[slot].pinCount) {
            rcWarn = PF_PAGEPINNED;
         }
         else {
            // Write the page if dirty
            if (bufTable[slot].bDirty) {
#ifdef PF_LOG
 sprintf (psMessage, "Page (%d) is dirty\n",bufTable[slot].pageNum);
 WriteLog(psMessage);
#endif
               if ((rc = WritePage(fd, bufTable[slot].pageNum, bufTable[slot].pData)))
                  return (rc);
               bufTable[slot].bDirty = FALSE;
            }

            // Remove page from the hash table and add the slot to the free list
            if ((rc = hashTable.Delete(fd, bufTable[slot].pageNum)) ||
                  (rc = Unlink(slot)) ||
                  (rc = InsertFree(slot)))
               return (rc);
         }
      }
      slot = next;
   }

#ifdef PF_LOG
   WriteLog("All necessary pages flushed.\n");
#endif

   // Return warning or ok
   return (rcWarn);
}

//
// ForcePages
//
// Desc: If a page is dirty then force the page from the buffer pool
//       onto disk.  The page will not be forced out of the buffer pool.
// In:   The page number, a default value of ALL_PAGES will be used if
//       the client doesn't provide a value.  This will force all pages.
// Ret:  Standard PF errors
//
//
RC PF_BufferMgr::ForcePages(int fd, PageNum pageNum)
{
   RC rc;  // return codes

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Forcing page %d for (%d).\n", pageNum, fd);
   WriteLog(psMessage);
#endif

   // Do a linear scan of the buffer to find the page for the file
   int slot = first;
   while (slot != INVALID_SLOT) {

      int next = bufTable[slot].next;

      // If the page belongs to the passed-in file descriptor
      if (bufTable[slot].fd == fd &&
            (pageNum==ALL_PAGES || bufTable[slot].pageNum == pageNum)) {

#ifdef PF_LOG
 sprintf (psMessage, "Page (%d) is in buffer pool.\n", bufTable[slot].pageNum);
 WriteLog(psMessage);
#endif
         // I don't care if the page is pinned or not, just write it if
         // it is dirty.
         if (bufTable[slot].bDirty) {
#ifdef PF_LOG
sprintf (psMessage, "Page (%d) is dirty\n",bufTable[slot].pageNum);
WriteLog(psMessage);
#endif
            if ((rc = WritePage(fd, bufTable[slot].pageNum, bufTable[slot].pData)))
               return (rc);
            bufTable[slot].bDirty = FALSE;
         }
      }
      slot = next;
   }

   return 0;
}


//
// PrintBuffer
//
// Desc: Display all of the pages within the buffer.
//       This routine will be called via the system command.
// In:   Nothing
// Out:  Nothing
// Ret:  Always returns 0
//
RC PF_BufferMgr::PrintBuffer()
{
   cout << "Buffer contains " << numPages << " pages of size "
      << pageSize <<".\n";
   cout << "Contents in order from most recently used to "
      << "least recently used.\n";

   int slot, next;
   slot = first;
   while (slot != INVALID_SLOT) {
      next = bufTable[slot].next;
      cout << slot << " :: \n";
      cout << "  fd = " << bufTable[slot].fd << "\n";
      cout << "  pageNum = " << bufTable[slot].pageNum << "\n";
      cout << "  bDirty = " << bufTable[slot].bDirty << "\n";
      cout << "  pinCount = " << bufTable[slot].pinCount << "\n";
      slot = next;
   }

   if (first==INVALID_SLOT)
      cout << "Buffer is empty!\n";
   else
      cout << "All remaining slots are free.\n";

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
// Ret:  Will return an error if a page is pinned and the Clear routine
//       is called.
RC PF_BufferMgr::ClearBuffer()
{
   RC rc;

   int slot, next;
   slot = first;
   while (slot != INVALID_SLOT) {
      next = bufTable[slot].next;
      if (bufTable[slot].pinCount == 0)
         if ((rc = hashTable.Delete(bufTable[slot].fd,
               bufTable[slot].pageNum)) ||
            (rc = Unlink(slot)) ||
            (rc = InsertFree(slot)))
         return (rc);
      slot = next;
   }

   return 0;
}

//
// ResizeBuffer
//
// Desc: Resizes the buffer manager to the size passed in.
//       This routine will be called via the system command.
// In:   The new buffer size
// Out:  Nothing
// Ret:  0 for success or,
//       Some other PF error (probably PF_NOBUF)
//
// Notes: This method attempts to copy all the old pages which I am
// unable to kick out of the old buffer manager into the new buffer
// manager.  This obviously cannot always be successfull!
//
RC PF_BufferMgr::ResizeBuffer(int iNewSize)
{
   int i;
   RC rc;

   // First try and clear out the old buffer!
   ClearBuffer();

   // Allocate memory for a new buffer table
   PF_BufPageDesc *pNewBufTable = new PF_BufPageDesc[iNewSize];

   // Initialize the new buffer table and allocate memory for buffer
   // pages.  Initially, the free list contains all pages
   for (i = 0; i < iNewSize; i++) {
      if ((pNewBufTable[i].pData = new char[pageSize]) == NULL) {
         cerr << "Not enough memory for buffer\n";
         exit(1);
      }

      memset ((void *)pNewBufTable[i].pData, 0, pageSize);

      pNewBufTable[i].prev = i - 1;
      pNewBufTable[i].next = i + 1;
   }
   pNewBufTable[0].prev = pNewBufTable[iNewSize - 1].next = INVALID_SLOT;

   // Now we must remember the old first and last slots and (of course)
   // the buffer table itself.  Then we use insert methods to insert
   // each of the entries into the new buffertable
   int oldFirst = first;
   PF_BufPageDesc *pOldBufTable = bufTable;

   // Setup the new number of pages,  first, last and free
   numPages = iNewSize;
   first = last = INVALID_SLOT;
   free = 0;

   // Setup the new buffer table
   bufTable = pNewBufTable;

   // We must first remove from the hashtable any possible entries
   int slot, next, newSlot;
   slot = oldFirst;
   while (slot != INVALID_SLOT) {
      next = pOldBufTable[slot].next;

      // Must remove the entry from the hashtable from the
      if ((rc=hashTable.Delete(pOldBufTable[slot].fd, pOldBufTable[slot].pageNum)))
         return (rc);
      slot = next;
   }

   // Now we traverse through the old buffer table and copy any old
   // entries into the new one
   slot = oldFirst;
   while (slot != INVALID_SLOT) {

      next = pOldBufTable[slot].next;
      // Allocate a new slot for the old page
      if ((rc = InternalAlloc(newSlot)))
         return (rc);

      // Insert the page into the hash table,
      // and initialize the page description entry
      if ((rc = hashTable.Insert(pOldBufTable[slot].fd,
            pOldBufTable[slot].pageNum, newSlot)) ||
            (rc = InitPageDesc(pOldBufTable[slot].fd,
            pOldBufTable[slot].pageNum, newSlot)))
         return (rc);

      // Put the slot back on the free list before returning the error
      Unlink(newSlot);
      InsertFree(newSlot);

      slot = next;
   }

   // Finally, delete the old buffer table
   delete [] pOldBufTable;

   return 0;
}


//
// InsertFree
//
// Desc: Internal.  Insert a slot at the head of the free list
// In:   slot - slot number to insert
// Ret:  PF return code
//
RC PF_BufferMgr::InsertFree(int slot)
{
   bufTable[slot].next = free;
   free = slot;

   // Return ok
   return (0);
}

//
// LinkHead
//
// Desc: Internal.  Insert a slot at the head of the used list, making
//       it the most-recently used slot.
// In:   slot - slot number to insert
// Ret:  PF return code
//
RC PF_BufferMgr::LinkHead(int slot)
{
   // Set next and prev pointers of slot entry
   bufTable[slot].next = first;
   bufTable[slot].prev = INVALID_SLOT;

   // If list isn't empty, point old first back to slot
   if (first != INVALID_SLOT)
      bufTable[first].prev = slot;

   first = slot;

   // if list was empty, set last to slot
   if (last == INVALID_SLOT)
      last = first;

   // Return ok
   return (0);
}

//
// Unlink
//
// Desc: Internal.  Unlink the slot from the used list.  Assume that
//       slot is valid.  Set prev and next pointers to INVALID_SLOT.
//       The caller is responsible to either place the unlinked page into
//       the free list or the used list.
// In:   slot - slot number to unlink
// Ret:  PF return code
//
RC PF_BufferMgr::Unlink(int slot)
{
   // If slot is at head of list, set first to next element
   if (first == slot)
      first = bufTable[slot].next;

   // If slot is at end of list, set last to previous element
   if (last == slot)
      last = bufTable[slot].prev;

   // If slot not at end of list, point next back to previous
   if (bufTable[slot].next != INVALID_SLOT)
      bufTable[bufTable[slot].next].prev = bufTable[slot].prev;

   // If slot not at head of list, point prev forward to next
   if (bufTable[slot].prev != INVALID_SLOT)
      bufTable[bufTable[slot].prev].next = bufTable[slot].next;

   // Set next and prev pointers of slot entry
   bufTable[slot].prev = bufTable[slot].next = INVALID_SLOT;

   // Return ok
   return (0);
}

//
// InternalAlloc
//
// Desc: Internal.  Allocate a buffer slot.  The slot is inserted at the
//       head of the used list.  Here's how it chooses which slot to use:
//       If there is something on the free list, then use it.
//       Otherwise, choose a victim to replace.  If a victim cannot be
//       chosen (because all the pages are pinned), then return an error.
// Out:  slot - set to newly-allocated slot
// Ret:  PF_NOBUF if all pages are pinned, other PF return code otherwise
//
RC PF_BufferMgr::InternalAlloc(int &slot)
{
   RC  rc;       // return code

   // If the free list is not empty, choose a slot from the free list
   if (free != INVALID_SLOT) {
      slot = free;
      free = bufTable[slot].next;
   }
   else {

      // Choose the least-recently used page that is unpinned
      for (slot = last; slot != INVALID_SLOT; slot = bufTable[slot].prev) {
         if (bufTable[slot].pinCount == 0)
            break;
      }

      // Return error if all buffers were pinned
      if (slot == INVALID_SLOT)
         return (PF_NOBUF);

      // Write out the page if it is dirty
      if (bufTable[slot].bDirty) {
         if ((rc = WritePage(bufTable[slot].fd, bufTable[slot].pageNum,
               bufTable[slot].pData)))
            return (rc);

         bufTable[slot].bDirty = FALSE;
      }

      // Remove page from the hash table and slot from the used buffer list
      if ((rc = hashTable.Delete(bufTable[slot].fd, bufTable[slot].pageNum)) ||
            (rc = Unlink(slot)))
         return (rc);
   }

   // Link slot at the head of the used list
   if ((rc = LinkHead(slot)))
      return (rc);

   // Return ok
   return (0);
}

//
// ReadPage
//
// Desc: Read a page from disk
//
// In:   fd - OS file descriptor
//       pageNum - number of page to read
//       dest - pointer to buffer in which to read page
// Out:  dest - buffer contains page contents
// Ret:  PF return code
//
RC PF_BufferMgr::ReadPage(int fd, PageNum pageNum, char *dest)
{

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Reading (%d,%d).\n", fd, pageNum);
   WriteLog(psMessage);
#endif

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_READPAGE, STAT_ADDONE);
#endif

   // seek to the appropriate place (cast to long for PC's)
   long offset = pageNum * (long)pageSize + PF_FILE_HDR_SIZE;
   if (lseek(fd, offset, L_SET) < 0)
      return (PF_UNIX);

   // Read the data
   int numBytes = read(fd, dest, pageSize);
   if (numBytes < 0)
      return (PF_UNIX);
   else if (numBytes != pageSize)
      return (PF_INCOMPLETEREAD);
   else
      return (0);
}

//
// WritePage
//
// Desc: Write a page to disk
//
// In:   fd - OS file descriptor
//       pageNum - number of page to write
//       dest - pointer to buffer containing page contents
// Ret:  PF return code
//
RC PF_BufferMgr::WritePage(int fd, PageNum pageNum, char *source)
{

#ifdef PF_LOG
   char psMessage[100];
   sprintf (psMessage, "Writing (%d,%d).\n", fd, pageNum);
   WriteLog(psMessage);
#endif

#ifdef PF_STATS
   pStatisticsMgr->Register(PF_WRITEPAGE, STAT_ADDONE);
#endif

   // seek to the appropriate place (cast to long for PC's)
   long offset = pageNum * (long)pageSize + PF_FILE_HDR_SIZE;
   if (lseek(fd, offset, L_SET) < 0)
      return (PF_UNIX);

   // Read the data
   int numBytes = write(fd, source, pageSize);
   if (numBytes < 0)
      return (PF_UNIX);
   else if (numBytes != pageSize)
      return (PF_INCOMPLETEWRITE);
   else
      return (0);
}

//
// InitPageDesc
//
// Desc: Internal.  Initialize PF_BufPageDesc to a newly-pinned page
//       for a newly pinned page
// In:   fd - file descriptor
//       pageNum - page number
// Ret:  PF return code
//
RC PF_BufferMgr::InitPageDesc(int fd, PageNum pageNum, int slot)
{
   // set the slot to refer to a newly-pinned page
   bufTable[slot].fd       = fd;
   bufTable[slot].pageNum  = pageNum;
   bufTable[slot].bDirty   = FALSE;
   bufTable[slot].pinCount = 1;

   // Return ok
   return (0);
}

//------------------------------------------------------------------------------
// Methods for manipulating raw memory buffers
//------------------------------------------------------------------------------

#define MEMORY_FD -1

//
// GetBlockSize
//
// Return the size of the block that can be allocated.  This is simply
// just the size of the page since a block will take up a page in the
// buffer pool.
//
RC PF_BufferMgr::GetBlockSize(int &length) const
{
   length = pageSize;
   return OK_RC;
}


//
// AllocateBlock
//
// Allocates a page in the buffer pool that is not associated with a
// particular file and returns the pointer to the data area back to the
// user.
//
RC PF_BufferMgr::AllocateBlock(char *&buffer)
{
   RC rc = OK_RC;

   // Get an empty slot from the buffer pool
   int slot;
   if ((rc = InternalAlloc(slot)) != OK_RC)
      return rc;

   // Create artificial page number (just needs to be unique for hash table)
   PageNum pageNum = PageNum(bufTable[slot].pData);

   // Insert the page into the hash table, and initialize the page description entry
   if ((rc = hashTable.Insert(MEMORY_FD, pageNum, slot) != OK_RC) ||
         (rc = InitPageDesc(MEMORY_FD, pageNum, slot)) != OK_RC) {
      // Put the slot back on the free list before returning the error
      Unlink(slot);
      InsertFree(slot);
      return rc;
   }

   // Return pointer to buffer
   buffer = bufTable[slot].pData;

   // Return success code
   return OK_RC;
}

//
// DisposeBlock
//
// Free the block of memory from the buffer pool.
//
RC PF_BufferMgr::DisposeBlock(char* buffer)
{
   return UnpinPage(MEMORY_FD, PageNum(buffer));
}
