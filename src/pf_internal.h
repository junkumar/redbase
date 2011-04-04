//
// File:        pf_internal.h
// Description: Declarations internal to the paged file component
// Authors:     Hugo Rivero (rivero@cs.stanford.edu)
//              Dallan Quass (quass@cs.stanford.edu)
//              Jason McHugh (mchughj@cs.stanford.edu)
//

#ifndef PF_INTERNAL_H
#define PF_INTERNAL_H

#include <cstdlib>
#include <cstring>
#include "pf.h"

//
// Constants and defines
//
const int PF_BUFFER_SIZE = 40;     // Number of pages in the buffer
const int PF_HASH_TBL_SIZE = 20;   // Size of hash table

#define CREATION_MASK      0600    // r/w privileges to owner only
#define PF_PAGE_LIST_END  -1       // end of list of free pages
#define PF_PAGE_USED      -2       // page is being used

// L_SET is used to indicate the "whence" argument of the lseek call
// defined in "/usr/include/unistd.h".  A value of 0 indicates to
// move to the absolute location specified.
#ifndef L_SET
#define L_SET              0
#endif

//
// PF_PageHdr: Header structure for pages
//
struct PF_PageHdr {
    int nextFree;       // nextFree can be any of these values:
                        //  - the number of the next free page
                        //  - PF_PAGE_LIST_END if this is last free page
                        //  - PF_PAGE_USED if the page is not free
};

// Justify the file header to the length of one page
const int PF_FILE_HDR_SIZE = PF_PAGE_SIZE + sizeof(PF_PageHdr);

#endif
