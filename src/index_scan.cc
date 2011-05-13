//
// File:        index_scan.cc
//

#include "index_scan.h"
#include "rm.h"
#include "sm.h"

using namespace std;

IndexScan::IndexScan(SM_Manager& smm,
                     RM_Manager& rmm,
                     IX_Manager& ixm,
                     const char* relName,
                     const char* indexAttrName, 
                     const Condition& cond,
                     RC& status,
                     bool desc)
  :ifs(IX_IndexScan()), prmm(&rmm), pixm(&ixm),
   rmh(RM_FileHandle()), ixh(IX_IndexHandle())
{
  if(relName == NULL || indexAttrName == NULL) {
    status = SM_NOSUCHTABLE;
    return;
  }

  attrCount = -1;
  attrs = NULL;
  RC rc = smm.GetFromTable(relName, attrCount, attrs);
  if (rc != 0) { 
    status = rc;
    return;
  }

  int indexNo = -2;
  assert(attrCount > 0);
  for(int i = 0; i < attrCount; i++) {
    if(strcmp(attrs[i].attrName, indexAttrName) == 0) {
      indexNo = attrs[i].indexNo;
    }
  }

  // has to be a value
  assert(cond.rhsValue.data == NULL || cond.bRhsIsAttr == FALSE); 
  // only conditions
  // on index key can be pushed down.
  assert(strcmp(cond.lhsAttr.attrName, indexAttrName) == 0);
  assert(strcmp(cond.lhsAttr.relName, relName) == 0);

  rc = prmm->OpenFile(relName, rmh);
  if (rc != 0) { 
    status = rc;
    return;
  }

  rc = pixm->OpenIndex(relName, indexNo, ixh);
  if (rc != 0) {
    status = rc;
    return;
  }

  rc = ifs.OpenScan(ixh, 
                    cond.op,
                    cond.rhsValue.data,
                    NO_HINT,
                    desc);
  if (rc != 0) { 
    status = rc;
    return;
  }

  status = 0;
}

RC IndexScan::IsValid()
{
  return (attrCount != -1 && attrs != NULL) ? 0 : SM_BADTABLE;
}

IndexScan::~IndexScan()
{
  ifs.CloseScan();
  pixm->CloseIndex(ixh);
  prmm->CloseFile(rmh);
  delete [] attrs;
}


// iterator interface
// acts as a (re)open after OpenScan has been called.
RC IndexScan::Open()
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(bIterOpen)
    return IX_HANDLEOPEN;
  if(!ifs.IsOpen())
    return IX_FNOTOPEN;

  bIterOpen = true;
  return 0;
}

// iterator interface
RC IndexScan::Close()
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(!bIterOpen)
    return IX_FNOTOPEN;
  if(!ifs.IsOpen())
    return IX_FNOTOPEN;

  bIterOpen = false;
  ifs.ResetState();
  return 0;
}

// iterator interface
RC IndexScan::GetNext(Tuple &t)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(!bIterOpen)
    return IX_FNOTOPEN;
  if(!ifs.IsOpen())
    return IX_FNOTOPEN;
  
  RID rid;
  RC rc = ifs.GetNextEntry(rid);
  if (rc == 0) {
    RM_Record rec;
    rc = rmh.GetRec(rid, rec);
    if (rc != 0 ) return rc;
    char * buf;
    rc = rec.GetData(buf);
    if (rc != 0 ) return rc;
    t.Set(buf);
  }
  return rc;
}
