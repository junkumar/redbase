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
  virtual string Explain();

  RC IsValid();
  virtual RC Eof() const { return IX_EOF; }

  // will close if already open
  // made available for NLIJ to use
  // only value is new, rest of the index attr condition is the same
  virtual RC ReOpenScan(void* newData);
  virtual string GetIndexAttr() const { return attrName; }
  virtual string GetIndexRel() const { return relName; }
  virtual bool IsDesc() const { return ifs.IsDesc(); }

 private:
  IX_IndexScan ifs;
  IX_Manager* pixm;
  RM_Manager* prmm;
  SM_Manager* psmm;
  string relName;
  string attrName;
  RM_FileHandle rmh;
  IX_IndexHandle ixh;
  int nOFilters;
  Condition* oFilters;
  // options used to open scan
  CompOp c;
};

#endif // INDEXSCAN_H
