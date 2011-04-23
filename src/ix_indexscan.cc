//
//
#include "ix_indexscan.h"
#include <cerrno>
#include <cassert>
#include <cstdio>
#include <iostream>

using namespace std;

IX_IndexScan::IX_IndexScan(): bOpen(false), desc(false)
{
  pred = NULL;
  pixh = NULL;
  currNode = NULL;
  currPos = -1;
}

IX_IndexScan::~IX_IndexScan()
{
}


RC IX_IndexScan::OpenScan(const IX_IndexHandle &fileHandle,
                          CompOp     compOp,
                          void       *value,
                          ClientHint pinHint,
                          bool desc) 
{
  if (bOpen)
  {
    // scan is already open
    return IX_HANDLEOPEN;
  }

  if((compOp < NO_OP) ||
      compOp > GE_OP)
    return IX_FCREATEFAIL;


  pixh = const_cast<IX_IndexHandle*>(&fileHandle);
  if((pixh == NULL) ||
     pixh->IsValid() != 0)
    return IX_FCREATEFAIL;

  bOpen = true;
  if(desc) 
    this->desc = true;

  pred = new Predicate(pixh->GetAttrType(),
                       pixh->GetAttrLength(),
                       0,
                       compOp,
                       value,
                       pinHint);

  return 0;
}

RC IX_IndexScan::GetNextEntry     (RID &rid)
{
  void * k = NULL;
  return GetNextEntry(k, rid);
}

RC IX_IndexScan::GetNextEntry(void *& k, RID &rid)
{
  if(!bOpen)
    return IX_FNOTOPEN;
  assert(pixh != NULL && pred != NULL && bOpen);

  // first time in
  if(currNode == NULL && currPos == -1) {
    if(!desc) {
      currNode = pixh->FindSmallestLeaf();
      currPos = -1;
    } else {
      currNode = pixh->FindLargestLeaf();
      currPos = currNode->GetNumKeys(); // 1 past
    }
  }
  
  for( BtreeNode* j = currNode;
       j != NULL;
       /* see end of loop */ ) 
  {
    // cerr << "GetNextEntry j's RID was " << j->GetPageRID() << endl;
    int i = -1;
    if(!desc) {
      if(currNode == j) // first time in for loop ?
        i = currPos+1;
      else {
        i = 0;
        currNode = j; // save Node in object state for later.
      }

      for (; i < currNode->GetNumKeys(); i++) 
      {
        currPos = i; // save Node in object state for later.

        // std::cerr << "GetNextRec ret pos " << currPos << std::endl;
        char* key = NULL;
        int ret = currNode->GetKey(i, (void*&)key);
        if(ret == -1) 
          return IX_PF; // TODO better error
      
        if(pred->eval(key, pred->initOp())) {
          // std::cerr << "GetNextRec pred match for RID " << current << std::endl;
          k = key;
          rid = currNode->GetAddr(i);
          return 0;
        }
      }
    } else { // Descending
      if(currNode == j) // first time in for loop ?
        i = currPos-1;
      else {
        currNode = j; // save Node in object state for later.
        i = currNode->GetNumKeys()-1;
      }

      for (; i >= 0; i--) 
      {
        currPos = i; // save Node in object state for later.

        // std::cerr << "GetNextRec ret pos " << currPos << std::endl;
        char* key = NULL;
        int ret = currNode->GetKey(i, (void*&)key);
        if(ret == -1) 
          return IX_PF; // TODO better error
      
        if(pred->eval(key, pred->initOp())) {
          // std::cerr << "GetNextRec pred match for RID " << current << std::endl;
          k = key;
          rid = currNode->GetAddr(i);
          return 0;
        }
      }

    }
    // Advance j
    if(!desc)
      j = pixh->FetchNode(j->GetRight());
    else
      j = pixh->FetchNode(j->GetLeft());
  }

  return IX_EOF;
}

RC IX_IndexScan::CloseScan()
{
  if(!bOpen)
    return IX_FNOTOPEN;
  assert(pixh != NULL || pred != NULL || bOpen);
  bOpen = false;
  if (pred != NULL)
    delete pred;
  currNode = NULL;
  currPos = -1;
  return 0;
}
