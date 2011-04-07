//
// File:        rm_record.cc
//

#include <cerrno>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

RM_FileScan::RM_FileScan() 
{
}

RM_FileScan::~RM_FileScan()
{
}

RC RM_FileScan::OpenScan(const RM_FileHandle &fileHandle,
                     AttrType   attrType,	     
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint) 
{
}

RC RM_FileScan::GetNextRec     (RM_Record &rec)
{
}

RC RM_FileScan::CloseScan()
{
}
