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
           const Condition& cond,
           RC& status);
  virtual ~FileScan();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();

  RC IsValid();
  virtual RC Eof() const { return RM_EOF; }
  virtual DataAttrInfo* GetAttr() const { return attrs; }

 private:
  RM_FileScan rfs;
  RM_Manager* prmm;
  RM_FileHandle rmh;
  // used for iterator interface
  bool bIterOpen;
  DataAttrInfo* attrs;
  int attrCount;
};

#endif // FILESCAN_H
