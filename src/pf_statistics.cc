//
// pf_statistics.cc
//

// This file contains the procedure to display all the statistics for the
// PF layer.

// Code written by Andre Bergholz, who was the TA for 2000

//
// This file only makes sense when the PF Statistics layer is defined
//
#ifdef PF_STATS

#include <iostream>
#include "pf.h"
#include "statistics.h"

using namespace std;

// This is defined within pf_buffermgr.cc
extern StatisticsMgr *pStatisticsMgr;

void PF_Statistics()
{
   // First get all the statistics, must remember to delete memory returned
   int *piGP = pStatisticsMgr->Get(PF_GETPAGE);
   int *piPF = pStatisticsMgr->Get(PF_PAGEFOUND);
   int *piPNF = pStatisticsMgr->Get(PF_PAGENOTFOUND);
   int *piRP = pStatisticsMgr->Get(PF_READPAGE);
   int *piWP = pStatisticsMgr->Get(PF_WRITEPAGE);
   int *piFP = pStatisticsMgr->Get(PF_FLUSHPAGES);

   cout << "PF Layer Statistics\n";
   cout << "-------------------\n";

   cout << "Total number of calls to GetPage Routine: ";
   if (piGP) cout << *piGP; else cout << "None";
   cout << "\n  Number found: ";
   if (piPF) cout << *piPF; else cout << "None";
   cout << "\n  Number not found: ";
   if (piPNF) cout << *piPNF; else cout << "None";
   cout << "\n-------------------\n";

   cout << "Number of read requests: ";
   if (piRP) cout << *piRP; else cout << "None";
   cout << "\nNumber of write requests: ";
   if (piWP) cout << *piWP; else cout << "None";
   cout << "\n-------------------\n";
   cout << "Number of flushes: ";
   if (piFP) cout << *piFP; else cout << "None";
   cout << "\n-------------------\n";

   // Must delete the memory returned from StatisticsMgr::Get
   delete piGP;
   delete piPF;
   delete piPNF;
   delete piRP;
   delete piWP;
   delete piFP;
}

#endif
