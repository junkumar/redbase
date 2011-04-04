//
// File:        pf_test2.cc
// Description: Test PF component
// Authors:     Jason McHugh (mchughj@cs.stanford.edu)
//
// 1997: This file was created to utilize the statistics manager to ensure
// that the buffer manager was performing correctly.  Note that you must
// compile the pf layer with the -DPF_STATS flag.  If you don't then this
// test won't really test anything over and above the pf_test1.cc.
//

#include <cstdio>
#include <iostream>
#include <cstring>
#include <unistd.h>
#include "pf.h"
#include "pf_internal.h"
#include "pf_hashtable.h"

using namespace std;

// The PF_STATS indicates that we will be tracking statistics for the PF
// Layer.  The Manager is defined within pf_buffermgr.cc.  Here we must
// place the initializer and then the final call to printout the statistics
// once main has finished
#ifdef PF_STATS
#include "statistics.h"

// This is defined within pf_buffermgr.cc
extern StatisticsMgr *pStatisticsMgr;

// This method is defined within pf_statistics.cc.  It is called at the end
// to display the final statistics, or by the debugger to monitor progress.
extern void PF_Statistics();
#endif

//
// Defines
//
#define FILE1	"file1"

RC TestPF()
{
   PF_Manager pfm;
   PF_FileHandle fh;
   PF_PageHandle ph;
   RC rc;
   char *pData;
   PageNum pageNum;
   int i;

   cout << "Creating file: " << FILE1 << "\n";
   if ((rc = pfm.CreateFile(FILE1)))
      return (rc);

   cout << "Opening file: " << FILE1 << "\n";

   if ((rc = pfm.OpenFile(FILE1, fh)))
      return(rc);

   cout << "Allocating " << PF_BUFFER_SIZE << " pages.\n";

   for (i = 0; i < PF_BUFFER_SIZE; i++) {
      if ((rc = fh.AllocatePage(ph)) ||
            (rc = ph.GetData(pData)) ||
            (rc = ph.GetPageNum(pageNum)))
         return(rc);

      if (i != pageNum) {
         cout << "Page number incorrect: " << (int)pageNum << " " << i << "\n";
         exit(1);
      }
      // Put only the page number into the page
      memcpy(pData, (char *)&pageNum, sizeof(PageNum));
   }

   // Now ask for the same pages again
   cout << "Asking for the same pages again.\n";

   for (i = 0; i < PF_BUFFER_SIZE; i++) {
      if ((rc = fh.GetThisPage(i, ph)) ||
            (rc = ph.GetData(pData)) ||
            (rc = ph.GetPageNum(pageNum)))
         return(rc);
      if (i != pageNum) {
         cout << "Page number incorrect: " << (int)pageNum
            << " " << i << "\n";
         exit(1);
      }
   }

   // Now, if we have compiled with PF_STATS then we need to ensure that
   // PF_GETPAGE = PF_BUFFER_SIZE and PF_PAGEFOUND = PF_BUFFER_SIZE.
   // Also that PF_PAGENOTFOUND = PF_BUFFER_SIZE.
#ifdef PF_STATS
   cout << "Verifying the statistics for buffer manager: ";
   int *piGP = pStatisticsMgr->Get(PF_GETPAGE);
   int *piPF = pStatisticsMgr->Get(PF_PAGEFOUND);
   int *piPNF = pStatisticsMgr->Get(PF_PAGENOTFOUND);

   if (piGP && (*piGP != PF_BUFFER_SIZE)) {
      cout << "Number of GetPages is incorrect! (" << *piGP << ")\n";
      // No built in error code for this
      exit(1);
   }
   if (piPF && (*piPF != PF_BUFFER_SIZE)) {
      cout << "Number of pages found in the buffer is incorrect! (" <<
        *piPF << ")\n";
      // No built in error code for this
      exit(1);
   }
   if (piPNF!=NULL) {
      cout << "Number of pages not found in the buffer is incorrect! (" <<
        *piPNF << ")\n";
      // No built in error code for this
      exit(1);
   }
   cout << " Correct!\n";

   delete piGP;
   delete piPF;
   delete piPNF;

#endif                 // PF_STATS

   cout << "Unpinning pages.\n";
   for (i = 0; i < PF_BUFFER_SIZE; i++)
      // Must unpine the pages twice
      if ((rc = fh.UnpinPage(i)) ||
          (rc = fh.UnpinPage(i)))
         return(rc);

   // Confirm that the buffer manager has written the correct number of
   // pages.
#ifdef PF_STATS
   cout << "Verifying the write statistics for buffer manager: ";
   int *piWP = pStatisticsMgr->Get(PF_WRITEPAGE);
   int *piRP = pStatisticsMgr->Get(PF_READPAGE);

   if (piWP && (*piWP != PF_BUFFER_SIZE)) {
      cout << "Number of write pages is incorrect! (" << *piGP << ")\n";
      // No built in error code for this
      exit(1);
   }
   if (piRP!=NULL) {
      cout << "Number of pages read in is incorrect! (" <<
        *piPNF << ")\n";
      // No built in error code for this
      exit(1);
   }
   cout << " Correct!\n";

   delete piWP;
   delete piRP;

#endif          // PF_STATS

   // Goal here is to push out of the buffer manager the old pages by
   // asking for new ones.  At the end the LRU algorithm should ensure that
   // none of the original pages lie in memory
   cout << "Allocating an additional " << PF_BUFFER_SIZE << " pages to clear";
   cout << "out the buffer pool.\n";
   for (i = 0; i < PF_BUFFER_SIZE; i++) {
      if ((rc = fh.AllocatePage(ph)) ||
            (rc = ph.GetData(pData)) ||
            (rc = ph.GetPageNum(pageNum)))
         return(rc);
      if (i+PF_BUFFER_SIZE != pageNum) {
         cout << "Page number incorrect: " << (int)pageNum
            << " " << i << "\n";
         exit(1);
      }
      if ((rc = fh.UnpinPage(i+PF_BUFFER_SIZE)))
         return(rc);
   }

   // Now refetch the original pages
   cout << "Now asking for the original pages again.\n";
   for (i = 0; i < PF_BUFFER_SIZE; i++) {
      if ((rc = fh.GetThisPage(i, ph)) ||
            (rc = ph.GetData(pData)) ||
            (rc = ph.GetPageNum(pageNum)))
         return(rc);
      if (i!= pageNum) {
         cout << "Page number incorrect: " << (int)pageNum
            << " " << i << "\n";
         exit(1);
      }
      if ((rc = fh.UnpinPage(i)))
         return(rc);
   }

   // The previous refetch should have resulted in the buffer manager
   // going to disk for each of the pages, since the buffer should
   // not have had any of the pages.
#ifdef PF_STATS
   cout << "Verifying that pages were not found in buffer pool: ";
   piPNF = pStatisticsMgr->Get(PF_PAGENOTFOUND);

   if (piPNF && (*piPNF != PF_BUFFER_SIZE)) {
      cout << "Number of pages not found in the buffer is incorrect! (" <<
        *piPF << ")\n";
      // No built in error code for this
      exit(1);
   }
   cout << " Correct!\n";

   delete piPNF;
#endif          // PF_STATS

   // Now we will Flush the buffer manager to disk and count the number of
   // flushes and the total number of writes.
   cout << "Flushing the File handle to disk.\n";
   if ((rc = fh.FlushPages()))
      return (rc);

#ifdef PF_
   cout << "Testing flush to disk: ";
   int *piFP = pStatisticsMgr->Get(PF_FLUSHPAGES);
   piWP = pStatisticsMgr->Get(PF_WRITEPAGES);

   if (piFP && (*piFP != 1)) {
      cout << "Number of times Flush pages routine has been called " <<
         "is incorrect! (" << *piFP << ")\n";
      // No built in error code for this
      exit(1);
   }
   if (piWP && (*piWP != 2*PF_BUFFER_SIZE)) {
      cout << "Number of written pages is incorrect! (" << *piWP << ")\n";
      // No built in error code for this
      exit(1);
   }
   cout << " Correct!\n";

   delete piFP;
   delete piWP;
#endif


   cout << "Flushing the File handle to disk. (Again)\n";
   if ((rc = fh.FlushPages()))
      return (rc);

   // Here the idea is to ensure that the number of pages written has not
   // increased!  Since everything was already flushed.
#ifdef PF_STATS
   cout << "Testing number of pages written to disk: ";
   piWP = pStatisticsMgr->Get(PF_WRITEPAGE);

   // This number should not have increased since last time!
   if (piWP && (*piWP != 2*PF_BUFFER_SIZE)) {
      cout << "Number of written pages is incorrect! (" << *piWP << ")\n";
      // No built in error code for this
      exit(1);
   }
   cout << " Correct!\n";

   delete piWP;
#endif

   // Close the file
   if ((rc = pfm.CloseFile(fh)))
      return(rc);

   // If we are dealing with statistics then we might as well output the
   // final numbers
#ifdef PF_STATS
   PF_Statistics();
#endif

   // Return ok
   return (0);
}

int main()
{
   RC rc;

   // Write out initial starting message
   cerr.flush();
   cout.flush();
   cout << "********************\n";
   cout << "Starting PF layer test.\n";
   cout.flush();

   // If we are tracking the PF Layer statistics
#ifndef PF_STATS
   cout << " ** The PF layer was not compiled with the -DPF_STATS flag **\n";
   cout << " **    This test file is not very effective without it!    **\n";
#endif

   cout << "----------------------\n";

   // Delete files from last time
   unlink(FILE1);

   if ((rc = TestPF())) {
      PF_PrintError(rc);
      return (1);
   }

   // Write ending message and exit
   cout << "Ending PF layer test.\n";
   cout << "********************\n\n";

   return (0);
}

