//
// File:        nested_loop_join.cc
//

#include "nested_loop_join.h"
#include "ql_error.h"
#include "sm.h"
#include <algorithm>

using namespace std;

NestedLoopJoin::
NestedLoopJoin(const char * lJoinAttr,   // name of join key - left
               const char * rJoinAttr,   // name of join key - right
               Iterator *    lhsIt_,      // access for left i/p to join -R
               Iterator *    rhsIt_,      // access for right i/p to join -S
               
               //Cond **outFilter,   // Ptr to the output filter
               //Cond **rightFilter, // Ptr to filter applied on right i
               //FldSpec  * proj_list,
               // int        n_out_flds,
               RC   & status)
  :lhsIt(lhsIt_), rhsIt(rhsIt_),
   left(lhsIt->GetAttrCount(), lhsIt->TupleLength()),
   right(rhsIt->GetAttrCount(), rhsIt->TupleLength())
{
  if(lhsIt == NULL || rhsIt == NULL) {
    status = SM_NOSUCHTABLE;
    return;
  }

  if(lJoinAttr == NULL || rJoinAttr == NULL) {
    status = QL_BADJOINKEY;
    return;
  }

  attrCount = lhsIt->GetAttrCount() + rhsIt->GetAttrCount() - 1;
  assert(attrCount > 0);

  attrs = new DataAttrInfo[attrCount];
  
  std::copy(lhsIt->GetAttr(), lhsIt->GetAttr() + lhsIt->GetAttrCount(),
            attrs);

  rKey.offset = -1;
  lKey.offset = -1;

  DataAttrInfo * rattrs = rhsIt->GetAttr();
  bool jKeyPassed = false;
  for(int i = 0, j = lhsIt->GetAttrCount(); i < rhsIt->GetAttrCount(); i++) {
    if(strcmp(rattrs[i].attrName, rJoinAttr) == 0) {
      rKey = rattrs[i];
      jKeyPassed = true;
      continue;
    }
    attrs[j] = rattrs[i];
    attrs[j].offset += lhsIt->TupleLength();
    if(jKeyPassed)
      attrs[j].offset -= rKey.attrLength;
    j++;
  }
  
  DataAttrInfo * lattrs = lhsIt->GetAttr();
  for(int i = 0; i < lhsIt->GetAttrCount(); i++) {
    if(strcmp(lattrs[i].attrName, lJoinAttr) == 0) {
      lKey = lattrs[i];
    }
  }

  if(rKey.offset == -1 || lKey.offset == -1) {
    status = QL_BADJOINKEY;
    return;
  }

  if(rKey.attrType != lKey.attrType) {
    status = QL_JOINKEYTYPEMISMATCH;
    return;
  }

  char * buf = new char[TupleLength()];
  memset(buf, 0, TupleLength());
  left.Set(buf);
  left.SetAttr(lhsIt->GetAttr());
  right.Set(buf);
  right.SetAttr(rhsIt->GetAttr());
  delete buf;

  status = 0;
}

RC NestedLoopJoin::IsValid()
{
  return (attrCount != -1 && attrs != NULL) ? 0 : SM_BADTABLE;
}

NestedLoopJoin::~NestedLoopJoin()
{
  delete [] attrs;
}

RC NestedLoopJoin::Open()
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(bIterOpen)
    return QL_ALREADYOPEN;

  RC rc = lhsIt->Open();
  if(rc != 0) return rc;
  rc = rhsIt->Open();
  if(rc != 0) return rc;

  lhsIt->GetNext(left);
  bIterOpen = true;
  return 0;
}

RC NestedLoopJoin::Close()
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(!bIterOpen)
    return QL_FNOTOPEN;

  RC rc = lhsIt->Close();
  if(rc != 0) return rc;
  rc = rhsIt->Close();
  // may already be closed - so no need to check for errors

  char * buf = new char[TupleLength()];
  memset(buf, 0, TupleLength());
  left.Set(buf);
  right.Set(buf);

  bIterOpen = false;
  return 0;
}

RC NestedLoopJoin::GetNext(Tuple &t)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(!bIterOpen)
    return QL_FNOTOPEN;
  
  bool joined = false;
  
  while(!joined) {
    RC rc = rhsIt->GetNext(right);
    if (rc == rhsIt->Eof()) { // end of rhs - start again
      if((rc = rhsIt->Close()))
        return rc;
      rc = lhsIt->GetNext(left);
      if (rc == lhsIt->Eof()) { // end of both - exit
        return QL_EOF;
      }
      rc = rhsIt->Open();
      rc = rhsIt->GetNext(right);
    }

    // check for join
    void * a = NULL;
    void * b = NULL;
    left.Get(lKey.attrName, a);
    right.Get(rKey.attrName, b);

    // cerr << left << " - " << right << endl;

    if(CmpKey(lKey.attrType, lKey.attrLength, a, b) == 0) {
      joined = true;
      char * buf = new char[TupleLength()];
      memset(buf, 0, TupleLength());

      char *lbuf;
      left.GetData(lbuf);
      memcpy(buf, lbuf, left.GetLength());

      char *rbuf;
      right.GetData(rbuf);

      memcpy(buf + left.GetLength(), 
             rbuf,
             rKey.offset);
      memcpy(buf + left.GetLength() + rKey.offset, 
             rbuf + rKey.offset + rKey.attrLength,
             right.GetLength() - rKey.attrLength - rKey.offset);
      t.Set(buf);
      delete buf;
      return 0;
    }
  } // while
}

int NestedLoopJoin::CmpKey(AttrType attrType, int attrLength, 
                           const void* a,
                           const void* b) const
{
  if (attrType == STRING) {
    return memcmp(a, b, attrLength);
  } 

  if (attrType == FLOAT) {
    typedef float MyType;
    if ( *(MyType*)a >  *(MyType*)b ) return 1;
    if ( *(MyType*)a == *(MyType*)b ) return 0;
    if ( *(MyType*)a <  *(MyType*)b ) return -1;
  }

  if (attrType == INT) {
    typedef int MyType;
    if ( *(MyType*)a >  *(MyType*)b ) return 1;
    if ( *(MyType*)a == *(MyType*)b ) return 0;
    if ( *(MyType*)a <  *(MyType*)b ) return -1;
  }

  assert("should never get here - bad attrtype");
  return 0; // to satisfy gcc warnings
}
