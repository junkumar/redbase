//
// File:        rm_filescan.cc
//

#include <cerrno>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

RM_FileScan::RM_FileScan(): bOpen(false)
{
  pred = NULL;
  prmh = NULL;
  current = RID(1,-1);
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
  if (bOpen)
  {
    // scan is already open
    return RM_HANDLEOPEN;
  }

  prmh = const_cast<RM_FileHandle*>(&fileHandle);
  if((prmh == NULL) || 
     prmh->IsValid() != 0)
    return RM_FCREATEFAIL;

  if(value != NULL) { // allow lazy init - dont check cond if value if NULL
    if((compOp < NO_OP) ||
       compOp > GE_OP)
      return RM_FCREATEFAIL;

    if((attrType < INT) ||
       (attrType > STRING))
      return RM_FCREATEFAIL;

    if(attrLength >= PF_PAGE_SIZE - (int)sizeof(RID) ||
       attrLength <= 0)
      return RM_RECSIZEMISMATCH;

    if((attrType == INT && (unsigned int)attrLength != sizeof(int)) ||
       (attrType == FLOAT && (unsigned int)attrLength != sizeof(float))
       ||
       (attrType == STRING && 
        ((unsigned int)attrLength <= 0 || 
         (unsigned int)attrLength > MAXSTRINGLEN)))
      return RM_FCREATEFAIL;

    if((attrOffset >= prmh->fullRecordSize()) ||
       attrOffset < 0)
      return RM_FCREATEFAIL;
  }

  bOpen = true;
  pred = new Predicate(attrType,       
                       attrLength,
                       attrOffset,
                       compOp,
                       value,
                       pinHint) ;
  return 0;
}

RC RM_FileScan::GotoPage(PageNum p)
{
  if(!bOpen)
    return RM_FNOTOPEN;
  assert(prmh != NULL && pred != NULL && bOpen);

  current = RID(p, -1);

  // set up to be at the slot before the first slot with data
  RM_Record rec;
  RC rc = GetNextRec(rec);
  RID rid;
  rec.GetRid(rid);
  current = RID(p, rid.Slot()-1);
  return 0;
}

RC RM_FileScan::GetNextRec     (RM_Record &rec)
{
  if(!bOpen)
    return RM_FNOTOPEN;
  assert(prmh != NULL && pred != NULL && bOpen);

  PF_PageHandle ph;
  RM_PageHdr pHdr(prmh->GetNumSlots());
  RC rc;
  
  for( int j = current.Page(); j < prmh->GetNumPages(); j++) {
    if((rc = prmh->pfHandle->GetThisPage(j, ph))
       // Needs to be called everytime GetThisPage is called.
       || (rc = prmh->pfHandle->UnpinPage(j)))
      return rc;

    if((rc = prmh->GetPageHeader(ph, pHdr)))
      return rc;
    bitmap b(pHdr.freeSlotMap, prmh->GetNumSlots());
    int i = -1;
    if(current.Page() == j)
      i = current.Slot()+1;
    else
      i = 0;
    for (; i < prmh->GetNumSlots(); i++) 
    {
      if (!b.test(i)) { 
        // not free - means this is useful data to us
        current = RID(j, i);
        prmh->GetRec(current, rec);
        // std::cerr << "GetNextRec ret RID " << current << std::endl;
        char * pData = NULL;
        rec.GetData(pData);
        if(pred->eval(pData, pred->initOp())) {
          // std::cerr << "GetNextRec pred match for RID " << current << std::endl;
          return 0;
        } else {
          // get next rec
        }
      }
    }
  }
  return RM_EOF;
}

RC RM_FileScan::CloseScan()
{
  if(!bOpen)
    return RM_FNOTOPEN;
  assert(prmh != NULL && pred != NULL);
  bOpen = false;
  if (pred != NULL)
    delete pred;
  current = RID(1,-1);
  return 0;
}
