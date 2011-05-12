//
// File:        nested_loop_join.cc
//

#include "nested_loop_join.h"
#include "rm.h"
#include "sm.h"

using namespace std;

NestedLoopJoin::
NestedLoopJoin(const RelAttr & lhs,
               const RelAttr & rhs,
               Iterator *    am1,      // access for left i/p to join -R
               Iterator *    am2,      // access for right i/p to join -S
               
               //Cond **outFilter,   // Ptr to the output filter
               //Cond **rightFilter, // Ptr to filter applied on right i
               //FldSpec  * proj_list,
               // int        n_out_flds,
               Status   & s);
:bIterOpen(false)
{
  attrCount = 
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

RC NestedLoopJoin::IsValid()
{
  return (attrCount != -1 && attrs != NULL) ? 0 : SM_BADTABLE;
}

NestedLoopJoin::~NestedLoopJoin()
{
  ifs.CloseScan();
  pixm->CloseIndex(ixh);
  prmm->CloseFile(rmh);
  delete [] attrs;
}


// iterator interface
// acts as a (re)open after OpenScan has been called.
RC NestedLoopJoin::Open()
{
  if(bIterOpen)
    return IX_HANDLEOPEN;
  if(!ifs.IsOpen())
    return IX_FNOTOPEN;

  bIterOpen = true;
  return 0;
}

// iterator interface
RC NestedLoopJoin::Close()
{
  if(!bIterOpen)
    return IX_FNOTOPEN;
  if(!ifs.IsOpen())
    return IX_FNOTOPEN;

  bIterOpen = false;
  ifs.ResetState();
  return 0;
}

// iterator interface
RC NestedLoopJoin::GetNext(Tuple &t)
{
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
