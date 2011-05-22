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
                     const char* relName_,
                     const char* indexAttrName, 
                     RC& status,
                     const Condition& cond,
                     int nOutFilters,
                     const Condition outFilters[],
                     bool desc)
  :ifs(IX_IndexScan()), prmm(&rmm), pixm(&ixm), psmm(&smm),
   rmh(RM_FileHandle()), ixh(IX_IndexHandle()), relName(relName_),
   nOFilters(nOutFilters), oFilters(NULL)
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

  oFilters = new Condition[nOFilters];
  for(int i = 0; i < nOFilters; i++) {
    oFilters[i] = outFilters[i]; // shallow copy
  }

  explain << "IndexScan\n";
  explain << "   relName = " << relName << "\n";
  explain << "   attrName = " << indexAttrName << "\n";
  if(cond.rhsValue.data != NULL)
    explain << "   ScanCond = " << cond << "\n";
  if(nOutFilters > 0) {
    explain << "   nFilters = " << nOutFilters << "\n";
    for (int i = 0; i < nOutFilters; i++)
      explain << "   filters[" << i << "]:" << outFilters[i] << "\n";
  }

  status = 0;
}

string IndexScan::Explain()
{
  return indent + explain.str();
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
  delete [] oFilters;
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
  RC rc;
  bool found = false;

  while(!found) {

    rc = ifs.GetNextEntry(rid);
    if (rc != 0) return rc;

    RM_Record rec;
    rc = rmh.GetRec(rid, rec);
    if (rc != 0 ) return rc;
    char * buf;
    rc = rec.GetData(buf);
    if (rc != 0 ) return rc;

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
    }

    if(recordIn) {
      t.Set(buf);
      t.SetRid(rid);
      found = true;
    }
  } // while
  return rc;
}
