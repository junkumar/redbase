//
// File:        rm_record.cc
//

#include <cerrno>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "rm.h"

using namespace std;

class Predicate {
public:
  Predicate() {}
  ~Predicate() {}

  Predicate(AttrType   attrTypeIn,       
            int        attrLengthIn,
            int        attrOffsetIn,
            CompOp     compOpIn,
            void       *valueIn,
            ClientHint pinHintIn) 
    {
      attrType = attrTypeIn;       
      attrLength = attrLengthIn;
      attrOffset = attrOffsetIn;
      compOp = compOpIn;
      value = valueIn;
      pinHint = pinHintIn;
    }

  bool eval(const char *buf, CompOp c) const {
    if(c == NO_OP || value == NULL) {
      return true;
    }
    const char * attr = buf + attrOffset;
    
    if(c == LT_OP) {
      if(attrType == INT) {
        return *attr < *((int *)value);
      }
      if(attrType == FLOAT) {
        return *attr < *((float *)value);
      }
      if(attrType == STRING) {
        return strncmp(attr, (char *)value, attrLength) < 0;
      }
    }
    if(c == GT_OP) {
      if(attrType == INT) {
        return *attr > *((int *)value);
      }
      if(attrType == FLOAT) {
        return *attr > *((float *)value);
      }
      if(attrType == STRING) {
        return strncmp(attr, (char *)value, attrLength) > 0;
      }
    }
    if(c == EQ_OP) {
      if(attrType == INT) {
        return *attr == *((int *)value);
      }
      if(attrType == FLOAT) {
        return *attr == *((float *)value);
      }
      if(attrType == STRING) {
        return strncmp(attr, (char *)value, attrLength) == 0;
      }
    }
    if(c == LE_OP) {
      return this->eval(buf, LT_OP) || this->eval(buf, EQ_OP); 
    }
    if(c == GE_OP) {
      return this->eval(buf, GT_OP) || this->eval(buf, EQ_OP); 
    }
    if(c == NE_OP) {
      return !this->eval(buf, EQ_OP);
    }
    assert("Bad value for c - should never get here.");
    return true;
  }

  CompOp initOp() const { return compOp; }



private:
  AttrType   attrType;
  int        attrLength;
  int        attrOffset;
  CompOp     compOp;
  void*      value;
  ClientHint pinHint;
};


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
  if (prmh != NULL || pred != NULL || bOpen)
  {
    // scan is already open
    return RM_HANDLEOPEN;
  }
  bOpen = true;
  pred = new Predicate(attrType,       
                       attrLength,
                       attrOffset,
                       compOp,
                       value,
                       pinHint) ;
  prmh = const_cast<RM_FileHandle*>(&fileHandle);
  return 0;
}

RC RM_FileScan::GetNextRec     (RM_Record &rec)
{
  assert(prmh != NULL || pred != NULL || bOpen);
  if(!bOpen)
    return RM_FNOTOPEN;

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
  assert(prmh != NULL || pred != NULL || bOpen);
  if(!bOpen)
    return RM_FNOTOPEN;
  bOpen = false;
  if (pred != NULL)
    delete pred;
  return 0;
}
