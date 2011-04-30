//
// dbdestroy.cc
//
//
// This shell is provided for the student.

#include <iostream>
#include <sstream>
#include <cstdio>
#include <cstring>
#include <unistd.h>
#include "rm.h"
#include "sm.h"
#include "redbase.h"

using namespace std;

//
// main
//
int main(int argc, char *argv[])
{
  RC rc;

  // Look for 2 arguments. The first is always the name of the program
  // that was executed, and the second should be the name of the
  // database.
  if (argc != 2) {
    cerr << "Usage: " << argv[0] << " <dbname> \n";
    exit(1);
  }
  
  // The database name is the second argument
  string dbname(argv[1]);
  
  // Create a subdirectory for the database
  stringstream command;
  command << "rm -r " << dbname;
  rc = system (command.str().c_str());
  if(rc != 0) {
    cerr << argv[0] << " rmdir error for " << dbname << "\n";
    exit(rc);
  }

  return(0);
}
