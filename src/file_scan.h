//
// File:        FileScan.h
//

#ifndef FILESCAN_H
#define FILESCAN_H

#include "redbase.h"
#include "iterator.h"
#include "rm.h"

using namespace std;

class FileScan: public Iterator {
 public:
  FileScan(SM_Manager& smm,
           RM_Manager& rmm,
           const char* relName,
           RC& status,
           const Condition& cond = NULLCONDITION,
           int nOutFilters = 0,
           const Condition outFilters[] = NULL);

  virtual ~FileScan();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();
  virtual string Explain();

  RC IsValid();
  virtual RC Eof() const { return RM_EOF; }

 private:
  RM_FileScan rfs;
  RM_Manager* prmm;
  SM_Manager* psmm;
  const char * relName;
  RM_FileHandle rmh;
  int nOFilters;
  Condition* oFilters;
};

#endif // FILESCAN_H
