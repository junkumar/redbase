//
// redbase.cc
//
// Author: Jason McHugh (mchughj@cs.stanford.edu)
//
// This shell is provided for the student.

#include <iostream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "redbase.h"
#include "rm.h"
#include "sm.h"
#include "ql.h"
#include "parser.h"

using namespace std;

static void PrintErrorExit(RC rc) {
  PrintErrorAll(rc);
  exit(rc);
}

//
// main
//
int main(int argc, char *argv[])
{
  RC rc;

  // Look for 2 arguments.  The first is always the name of the program
  // that was executed, and the second should be the name of the
  // database.
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " dbname\n";
    exit(1);
  }
  char *dbname = argv[1];

  // initialize RedBase components
  PF_Manager pfm;
  RM_Manager rmm(pfm);
  IX_Manager ixm(pfm);
  SM_Manager smm(ixm, rmm);
  QL_Manager qlm(smm, ixm, rmm);
  // open the database
  if ((rc = smm.OpenDb(dbname)))
    PrintErrorExit(rc);
  // call the parser
  RC parseRC = RBparse(pfm, smm, qlm);

  // close the database
  if ((rc = smm.CloseDb()))
    PrintErrorExit(rc);
  
  if(parseRC != 0)
    PrintErrorExit(parseRC);

  cout << "Bye.\n";
  return 0;
}
