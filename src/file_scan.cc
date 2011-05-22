//
// File:        rm_filescan.cc
//

#include "file_scan.h"
#include "rm.h"
#include "sm.h"
#include "predicate.h"

using namespace std;

// if cond.rhsValue.data is null rest of the cond is not checked
FileScan::FileScan(SM_Manager& smm,
                   RM_Manager& rmm,
                   const char* relName_,
                   RC& status,
                   const Condition& cond,
                   int nOutFilters,
                   const Condition outFilters[]
  ):rfs(RM_FileScan()), prmm(&rmm), psmm(&smm), rmh(RM_FileHandle()),
    relName(relName_), nOFilters(nOutFilters)
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
  if(cond.rhsValue.data != NULL) {
    smm.GetAttrFromCat(relName, cond.lhsAttr.attrName, condAttr, r);
  }

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
  
  oFilters = new Condition[nOFilters];
  for(int i = 0; i < nOFilters; i++) {
    oFilters[i] = outFilters[i]; // shallow copy
  }

  explain << "FileScan\n";
  explain << "   relName = " << relName << "\n";
  if(cond.rhsValue.data != NULL)
    explain << "   ScanCond = " << cond << "\n";
  if(nOFilters > 0) {
    explain << "   nFilters = " << nOFilters << "\n";
    for (int i = 0; i < nOutFilters; i++)
      explain << "   filters[" << i << "]:" << oFilters[i] << "\n";
  }

  status = 0;
}

string FileScan::Explain()  
{
  return indent + explain.str();
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
  delete [] oFilters;
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
  RID recrid;
  bool found = false;
  RC rc;

  while(!found) {
    rc = rfs.GetNextRec(rec);
    if (rc != 0) return rc;

    char * buf;
    rec.GetData(buf);
    rec.GetRid(recrid);

    bool recordIn = true;
    for (int i = 0; i < nOFilters; i++) {
      Condition cond = oFilters[i];
      DataAttrInfo condAttr;
      RID r;  
      rc = psmm->GetAttrFromCat(relName, cond.lhsAttr.attrName, condAttr, r);
      if (rc != 0) return rc;

      Predicate p(condAttr.attrType,
                  condAttr.attrLength,
                  condAttr.offset,
                  cond.op,
                  cond.rhsValue.data,
                  NO_HINT);
        
      char * rhs = (char*)cond.rhsValue.data;
      if(cond.bRhsIsAttr == TRUE) {
        DataAttrInfo rhsAttr;
        RID r;  
        rc = psmm->GetAttrFromCat(relName, cond.lhsAttr.attrName, rhsAttr, r);
        if (rc != 0) return rc;
        rhs = (buf + rhsAttr.offset);
      }       
   
      if(!p.eval(buf, rhs, cond.op)) {
        recordIn = false;
        break;
      }
    } // for

    if(recordIn) {
      t.Set(buf);
      t.SetRid(recrid);
      found = true;
    }
  } // while

  return rc;
}

