//
// File:        Indexscan.h
//

#ifndef INDEXSCAN_H
#define INDEXSCAN_H

#include "redbase.h"
#include "iterator.h"
#include "ix.h"
#include "sm.h"
#include "rm.h"

using namespace std;

class IndexScan: public Iterator {
 public:
  IndexScan(SM_Manager& smm,
            RM_Manager& rmm,
            IX_Manager& ixm,
            const char* relName,
            const char* indexAttrName,
            RC& status,
            const Condition& cond = NULLCONDITION,
            int nOutFilters = 0,
            const Condition outFilters[] = NULL,
            bool desc=false);

  virtual ~IndexScan();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();

  RC IsValid();
  virtual RC Eof() const { return IX_EOF; }

 private:
  IX_IndexScan ifs;
  IX_Manager* pixm;
  RM_Manager* prmm;
  SM_Manager* psmm;
  const char * relName;
  RM_FileHandle rmh;
  IX_IndexHandle ixh;
  int nOFilters;
  Condition* oFilters;
};

#endif // INDEXSCAN_H
