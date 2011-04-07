//
// File:        rm_error.cc
// Description: RM_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

//
// Error table
//
static char *RM_WarnMsg[] = {
  (char*)"record is too big for a page",
  (char*)"error is in the PF component",
  (char*)"end of file",
};

static char *RM_ErrorMsg[] = {
  (char*)"record size is too big for a page",
  (char*)"error is in the PF component",
  (char*)"record null",
  (char*)"record size mismatch",
  (char*)"end of file",
};

//
// RM_PrintError
//
// Desc: Send a message corresponding to a RM return code to cerr
// In:   rc - return code for which a message is desired
//
void RM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_RM_WARN && rc <= RM_LASTWARN)
    // Print warning
    cerr << "RM warning: " << RM_WarnMsg[rc - START_RM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if ((-rc >= -START_RM_ERR) && -rc < -RM_LASTERROR)
	{
   // Print error
		cerr << "rc was " << rc << endl;
		cerr << "START_RM_ERR was " << START_RM_ERR << endl;
		cerr << "RM_LASTERROR was " << RM_LASTERROR << endl;

    cerr << "RM error: " << RM_ErrorMsg[-rc + START_RM_ERR] << "\n";
	}
  else if (rc == 0)
    cerr << "RM_PrintError called with return code of 0\n";
  else
	{
   // Print error
		cerr << "rc was " << rc << endl;
		cerr << "START_RM_ERR was " << START_RM_ERR << endl;
		cerr << "RM_LASTERROR was " << RM_LASTERROR << endl;
    cerr << "RM error: " << rc << " is out of bounds\n";
	}
}
