//
// File:        rm_error.cc
// Description: RM_PrintError function
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm_error.h"
#include "ix_error.h"
#include "sm_error.h"
#include "ql_error.h"
#include "pf.h"

using namespace std;


//
// Error table
//
const char *RM_WarnMsg[] = {
  (char*)"bad record size <= 0",
  (char*)"This rid has no record",
};

const char *RM_ErrorMsg[] = {
  (char*)"record size is too big for a page",
  (char*)"error is in the PF component",
  (char*)"record null",
  (char*)"record size mismatch",
  (char*)"attempt to open already open file handle",
  (char*)"bad parameters specified to RM open/create file handle",
  (char*)"file is not open",
  (char*)"Bad RID - invalid page num or slot num",
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
  else if ((-rc >= -START_RM_ERR) && -rc <= -RM_LASTERROR)
  {
   // Print error
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


//
// PrintError
//
// Desc: Send a message whether it is a RM or a PF return code to cerr
// In:   rc - return code for which a message is desired
//
void PrintErrorAll(RC rc)
{
  // Check the return code is within proper limits
  if ((rc >= START_RM_WARN && rc <= RM_LASTWARN)
      || (-rc >= -START_RM_ERR && -rc <= -RM_LASTERROR))
    RM_PrintError(rc);
  else if ((rc >= START_PF_WARN && rc <= PF_LASTWARN)
           || (-rc >= -START_PF_ERR && -rc <= -PF_LASTERROR))
    PF_PrintError(rc);
  else if ((rc >= START_IX_WARN && rc <= IX_LASTWARN)
           || (-rc >= -START_IX_ERR && -rc <= -IX_LASTERROR))
    IX_PrintError(rc);
  else if ((rc >= START_SM_WARN && rc <= SM_LASTWARN)
           || (-rc >= -START_SM_ERR && -rc <= -SM_LASTERROR))
    SM_PrintError(rc);
  else if ((rc >= START_QL_WARN && rc <= QL_LASTWARN)
           || (-rc >= -START_QL_ERR && -rc <= -QL_LASTERROR))
    QL_PrintError(rc);
  else if (rc == 0)
    cerr << "PrintError called with return code of 0\n";
  else
  {
   // Print error
    cerr << "rc was " << rc << endl;
    cerr << "Out of bounds for RM, IX, SM, QL and PF err/warn" << endl;
  }
}
