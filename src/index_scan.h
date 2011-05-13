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
            const Condition& cond,
            RC& status,
            bool desc=false);

  virtual ~IndexScan();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();

  RC IsValid();
  virtual RC Eof() const { return IX_EOF; }
  virtual DataAttrInfo* GetAttr() const { return attrs; }
  virtual int GetAttrCount() const { return attrCount; }

 private:
  IX_IndexScan ifs;
  IX_Manager* pixm;
  RM_Manager* prmm;
  RM_FileHandle rmh;
  IX_IndexHandle ixh;
  // used for iterator interface
  bool bIterOpen;
  DataAttrInfo* attrs;
  int attrCount;
};

#endif // INDEXSCAN_H
