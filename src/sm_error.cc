//
// File:        sm_error.cc
// Description: SM_PrintError function
//

#include "sm_error.h"
#include <cerrno>
#include <cstdio>
#include <iostream>
#include "sm.h"

using namespace std;

//
// Error table
//
const char *SM_WarnMsg[] = {
  (char*)"key was not found in btree",
  (char*)"key attribute size is too small(<=0) or too big for a page",
  (char*)"key,rid already exists in index",
  (char*)"SM_NOSUCHENTRY relName/attrName do not exist in DB",
};

const char *SM_ErrorMsg[] = {
  (char*)"DB is already open",
  (char*)"Bad DB name - no such DB.",
  (char*)"DB is not open",
  (char*)"Bad Table name - no such table",
  (char*)"bad open",
  (char*)"file is not open",
  (char*)"Bad Attribute name specified for this relation",
  (char*)"Bad Table/Relation",
  (char*)"SM_INDEXEXISTS Index already exists.",
  (char*)"SM_TYPEMISMATCH Types do not match.",
  (char*)"SM_BADOP Bad operator in condition.",
  (char*)"SM_AMBGATTR Attribute ambiguous - more than 1 relation contains same attr.",
  (char*)"SM_BADPARAM Bad parameters used with set on commandline",
  (char*)"SM_BADAGGFUN Bad Aggregate function - not supported",
};

//
// SM_PrintError
//
// Desc: Send a message corresponding to a SM return code to cerr
// In:   rc - return code for which a message is desired
//
void SM_PrintError(RC rc)
{
  // Check the return code is within proper limits
  if (rc >= START_SM_WARN && rc <= SM_LASTWARN)
    // Print warning
    cerr << "SM warning: " << SM_WarnMsg[rc - START_SM_WARN] << "\n";
  // Error codes are negative, so invert everything
  else if ((-rc >= -START_SM_ERR) && -rc <= -SM_LASTERROR)
  {
   // Print error
    cerr << "SM error: " << SM_ErrorMsg[-rc + START_SM_ERR] << "\n";
  }
  else if (rc == 0)
    cerr << "SM_PrintError called with return code of 0\n";
  else
  {
   // Print error
    cerr << "rc was " << rc << endl;
    cerr << "START_SM_ERR was " << START_SM_ERR << endl;
    cerr << "SM_LASTERROR was " << SM_LASTERROR << endl;
    cerr << "SM error: " << rc << " is out of bounds\n";
  }
}
