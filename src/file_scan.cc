//
// File:        rm_filescan.cc
//

#include "file_scan.h"
#include "rm.h"
#include "sm.h"

using namespace std;

FileScan::FileScan(SM_Manager& smm,
                   RM_Manager& rmm,
                   const char* relName, 
                   const Condition& cond,
                   RC& status)
  :rfs(RM_FileScan()), prmm(&rmm), rmh(RM_FileHandle())
{
  attrCount = -1;
  attrs = NULL;
  RC rc = smm.GetFromTable(relName, attrCount, attrs);
  if (rc != 0) { 
    status = rc;
    return;
  }
  
  assert(cond.rhsValue.data == NULL || cond.bRhsIsAttr == FALSE); // has to be a value
  
  DataAttrInfo condAttr;
  RID r;
  smm.GetAttrFromCat(relName, cond.lhsAttr.attrName, condAttr, r);


  rc = prmm->OpenFile(relName, rmh);
  if (rc != 0) { 
    status = rc;
    return;
  }

  rc = rfs.OpenScan(rmh, 
                    condAttr.attrType,
                    condAttr.attrLength,
                    condAttr.offset,
                    cond.op,
                    cond.rhsValue.data,
                    NO_HINT);
  if (rc != 0) { 
    status = rc;
    return;
  }

  status = 0;
}

RC FileScan::IsValid()
{
  return (attrCount != -1 && attrs != NULL) ? 0 : SM_BADTABLE;
}

FileScan::~FileScan()
{
  rfs.CloseScan();
  prmm->CloseFile(rmh);
  delete [] attrs;
}


// iterator interface
// acts as a (re)open after OpenScan has been called.
RC FileScan::Open()
{
  if(bIterOpen)
    return RM_HANDLEOPEN;
  if(!rfs.IsOpen())
    return RM_FNOTOPEN;

  bIterOpen = true;
  return 0;
}

// iterator interface
RC FileScan::Close()
{
  if(!bIterOpen)
    return RM_FNOTOPEN;
  if(!rfs.IsOpen())
    return RM_FNOTOPEN;

  bIterOpen = false;
  rfs.resetState();
  return 0;
}

// iterator interface
RC FileScan::GetNext(Tuple &t)
{
  if(!bIterOpen)
    return RM_FNOTOPEN;
  if(!rfs.IsOpen())
    return RM_FNOTOPEN;
  
  RM_Record rec;
  RC rc = rfs.GetNextRec(rec);
  if (rc == 0) {
    char * buf;
    rec.GetData(buf);
    t.Set(buf);
  }
  return rc;
}
