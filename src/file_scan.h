//
// File:        FileScan.h
//

#ifndef FILESCAN_H
#define FILESCAN_H

#include "redbase.h"
#include "iterator.h"
#include "rm.h"
#include "filter_eval.h"
#include "sm.h"

using namespace std;

// FileScan borrows the outFilters array via shallow copy. It precomputes
// attribute metadata once and does not perform per-tuple catalog lookups.
// Caller manages the lifetime of any external resources; FileScan does not
// own the database (SM/RM) managers passed in.
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
  virtual int GetNumPages() const { return psmm->GetNumPages(relName); }
  virtual int GetNumSlotsPerPage() const { return rfs.GetNumSlotsPerPage(); }
  virtual int GetNumRecords() const { return psmm->GetNumRecords(relName); }
  virtual RC GotoPage(PageNum p) { return rfs.GotoPage(p); }

 private:
  RM_FileScan rfs;
  RM_Manager* prmm;
  SM_Manager* psmm;
  const char * relName;
  RM_FileHandle rmh;
  int nOFilters;
  Condition* oFilters;
  FilterEvaluator filter;
};

#endif // FILESCAN_H
