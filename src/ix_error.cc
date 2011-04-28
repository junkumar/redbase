//
// File:        ix_error.cc
// Description: IX_PrintError function
//

#include "ix_error.h"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include "ix.h"
#include "pf.h"

using namespace std;

//
// Error table
//
const char *IX_WarnMsg[] = {
  (char*)"key was not found in btree",
  (char*)"key attribute size is too small(<=0) or too big for a page",
  (char*)"key,rid already exists in index",
  (char*)"key,rid combination does not exist in index",
};

const char *IX_ErrorMsg[] = {
  (char*)"entry is too big",
  (char*)"error is in the PF component",
  (char*)"Index page with btree node is no longer valid",
  (char*)"Index file creation failed",
  (char*)"attempt to open already open file handle",
  (char*)"bad parameters specified to IX open file handle",
  (char*)"file is not open",
  (char*)"Bad RID - invalid page num or slot num",
  (char*)"Bad Key - null or invalid",
  (char*)"end of file",
};

//
// IX_PrintError
//
// Desc: Send a message corresponding to a IX return code to cerr
// In:   rc - return code for which a message is desired
//
void IX_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_IX_WARN && rc <= IX_LASTWARN)
    // Print warning
    cerr << "IX warning: " << IX_WarnMsg[rc - START_IX_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if ((-rc >= -START_IX_ERR) && -rc <= -IX_LASTERROR)
  {
   // Print error
    cerr << "IX error: " << IX_ErrorMsg[-rc + START_IX_ERR] << "\n";
  }
  else if (rc == 0)
    cerr << "IX_PrintError called with return code of 0\n";
  else
  {
   // Print error
    cerr << "rc was " << rc << endl;
    cerr << "START_IX_ERR was " << START_IX_ERR << endl;
    cerr << "IX_LASTERROR was " << IX_LASTERROR << endl;
    cerr << "IX error: " << rc << " is out of bounds\n";
  }
}
