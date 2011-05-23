//
//
#include "ix_indexscan.h"
#include <cerrno>
#include <cassert>
#include <cstdio>
#include <iostream>

using namespace std;

IX_IndexScan::IX_IndexScan(): bOpen(false), desc(false), eof(false), lastNode(NULL)
{
  pred = NULL;
  pixh = NULL;
  currNode = NULL;
  currPos = -1;
  currKey = NULL;
  currRid = RID(-1, -1);
  c = EQ_OP;
  value = NULL;
}

IX_IndexScan::~IX_IndexScan()
{
  // in case close was forgotten
  if (pred != NULL)
    delete pred;
  
  if(pixh != NULL && pixh->GetHeight() > 1) {
    if(currNode != NULL)
      delete currNode;
    if(lastNode != NULL)
      delete lastNode;
  }
}


RC IX_IndexScan::OpenScan(const IX_IndexHandle &fileHandle,
                          CompOp     compOp,
                          void       *value_,
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
  foundOne = false;

  pred = new Predicate(pixh->GetAttrType(),
                       pixh->GetAttrLength(),
                       0,
                       compOp,
                       value_,
                       pinHint);


  c = compOp;
  if(value_ != NULL) {
    value = value_; // TODO deep copy ?
    OpOptimize();
  }
  
  // cerr << "IX_IndexScan::OpenScan with value ";
  // if(value == NULL)
  //   cerr << "NULL" << endl;
  // else 
  //   cerr << *(int*)value << endl;
  // pixh->Print(cerr);
  return 0;
}

RC IX_IndexScan::GetNextEntry     (RID &rid)
{
  void * k = NULL;
  int i = -1;
  return GetNextEntry(k, rid, i);
}

RC IX_IndexScan::GetNextEntry(void *& k, RID &rid, int& numScanned)
{
  if(!bOpen)
    return IX_FNOTOPEN;
  assert(pixh != NULL && pred != NULL && bOpen);
  if(eof)
    return IX_EOF;

  bool currDeleted = false;

  // first time in
  if(currNode == NULL && currPos == -1) {
    // pixh->Print(cerr);
    if(!desc) {
      currNode = pixh->FetchNode(pixh->FindSmallestLeaf()->GetPageRID());
      currPos = -1;
    } else {
      currNode = pixh->FetchNode(pixh->FindLargestLeaf()->GetPageRID());
      currPos = currNode->GetNumKeys(); // 1 past
    }
    currDeleted = false;
  } else { // check if the entry at curr was deleted
    if (currKey != NULL && currNode != NULL && currPos != -1) {
      char* key = NULL;
      int ret = currNode->GetKey(currPos, (void*&)key);
      if(ret == -1) 
        return IX_PF; // TODO better error
      
      if(currNode->CmpKey(key, currKey) != 0) {
        currDeleted = true;          
      } else {
        if(!(currRid == currNode->GetAddr(currPos)))
          currDeleted = true;
      }
    }
  }

  for( ;
       (currNode != NULL);
       /* see end of loop */ ) 
  {
    // cerr << "GetNextEntry currPos was " << currPos << endl;
    int i = -1;


    if(!desc) {
      // first time in for loop ?
      if(!currDeleted) {
        i = currPos+1;
      } else {
        i = currPos;
        // cerr << "GetNextEntry deletion at curr ! Staying at pos " 
        //      << currPos << std::endl;
        currDeleted = false;
      }

      for (; i < currNode->GetNumKeys(); i++) 
      {
        char* key = NULL;
        int ret = currNode->GetKey(i, (void*&)key);
        numScanned++;
        if(ret == -1) 
          return IX_PF; // TODO better error
        // std::cerr << "GetNextEntry curr entry " << *(int*)key << std::endl;

        // save Node in object state for later.
        currPos = i;
        if (currKey == NULL)
          currKey = (void*) new char[pixh->GetAttrLength()];
        memcpy(currKey, key, pixh->GetAttrLength());
        currRid = currNode->GetAddr(i);

        if(pred->eval(key, pred->initOp())) {
          k = key;
          rid = currNode->GetAddr(i);
          // std::cerr << "GetNextRec pred match for entry " << *(int*)key << " " 
          //           << rid << std::endl;
          foundOne = true;
          return 0;
        } else {
          if(foundOne) {
            RC rc = EarlyExitOptimize(key);
            if(rc != 0) return rc;
            if(eof)
              return IX_EOF;
          }
        }
      }
    } else { // Descending
      // first time in for loop ?
      if(!currDeleted) {
        i = currPos-1;
      } else {
        i = currPos;
        // std::cerr << "GetNextEntry deletion at curr ! Staying at pos " 
        //           << currPos << std::endl;
        currDeleted = false;
      }

      for (; i >= 0; i--) 
      {
        // std::cerr << "GetNextRec ret pos " << currPos << std::endl;
        char* key = NULL;
        int ret = currNode->GetKey(i, (void*&)key);
        numScanned++;
        if(ret == -1) 
          return IX_PF; // TODO better error
        // std::cerr << "GetNextEntry curr entry " << *(int*)key << std::endl;

        // save Node in object state for later.
        currPos = i;
        if (currKey == NULL)
          currKey = (void*) new char[pixh->GetAttrLength()];
        memcpy(currKey, key, pixh->GetAttrLength());
        currRid = currNode->GetAddr(i);

        if(pred->eval(key, pred->initOp())) {
          // std::cerr << "GetNextRec pred match for RID " << current << std::endl;
          k = key;
          rid = currNode->GetAddr(i);
          foundOne = true;
          return 0;
        } else {
          if(foundOne) {
            RC rc = EarlyExitOptimize(key);
            if(rc != 0) return rc;
            if(eof)
              return IX_EOF;
          }
        }
      }

    }
    if( (lastNode!= NULL) && 
        currNode->GetPageRID() == lastNode->GetPageRID() )
      break;
    // Advance to a new page
    if(!desc) {
      PageNum right = currNode->GetRight();
      // cerr << "IX_INDEXSCAN moved to new page " << right << endl;
      pixh->UnPin(currNode->GetPageRID().Page());
      delete currNode;
      currNode = NULL;
      currNode = pixh->FetchNode(right);
      if(currNode != NULL)
        pixh->Pin(currNode->GetPageRID().Page());
      currPos = -1;
    }
    else {
      PageNum left = currNode->GetLeft();
      pixh->UnPin(currNode->GetPageRID().Page());
      delete currNode;
      currNode = NULL;
      currNode = pixh->FetchNode(left);
      if(currNode != NULL) {
        pixh->Pin(currNode->GetPageRID().Page());
        currPos = currNode->GetNumKeys();
      }
    }
  } // for j

  return IX_EOF;
}

RC IX_IndexScan::CloseScan()
{
  if(!bOpen)
    return IX_FNOTOPEN;
  assert(pixh != NULL || pred != NULL || bOpen);
  bOpen = false;
  if(pred != NULL)
    delete pred;
  pred = NULL;
  currNode = NULL;
  currPos = -1;
  if(currKey != NULL) {
    delete [] ((char*) currKey);
    currKey = NULL;
  }
  currRid = RID(-1, -1);
  lastNode = NULL;
  eof = false;
  return 0;
}

// for the iterator to use
RC IX_IndexScan::ResetState()
{
  currNode = NULL;
  currPos = -1;
  lastNode = NULL;
  eof = false;
  foundOne = false;

  return this->OpOptimize();
}

// exit early if we know we will not find other useful values thanks to sorted
// nature
RC IX_IndexScan::EarlyExitOptimize(void* now)
{
  // cerr << "in early exit opt" << endl;

  if(!bOpen)
    return IX_FNOTOPEN;

  if(value == NULL)
    return 0; //nothing to optimize

  // no opt possible
  if(c == NE_OP || c == NO_OP)
    return 0;


  if(currNode != NULL) {

    int cmp = currNode->CmpKey(now, value);
    if(c == EQ_OP && cmp != 0) {
      // cerr << "in early exit opt 3 cmp was " << cmp << endl;
      eof = true;
      return 0;
    }
    if((c == LT_OP || c == LE_OP) && cmp > 0 && !desc) {
      // cerr << "in early exit opt 3 cmp was " << cmp << endl;
      eof = true;
      return 0;
    }
    if((c == GT_OP || c == GE_OP) && cmp < 0 && desc) {
      // cerr << "in early exit opt 3 cmp was " << cmp << endl;
      eof = true;
      return 0;
    }
  }
  return 0;
}

// Set up current pointers based on btree
RC IX_IndexScan::OpOptimize()
{
  if(!bOpen)
    return IX_FNOTOPEN;
  
  if(value == NULL)
    return 0; //nothing to optimize

  // no opt possible
  if(c == NE_OP)
    return 0;
  RID r(-1, -1);

  if(currNode != NULL) delete currNode;
  currNode = pixh->FetchNode(pixh->FindLeafForceRight(value)->GetPageRID().Page());
  currPos = currNode->FindKey((const void*&)value);

  // find rightmost version of a value and go left from there.
  if((c == LE_OP || c == LT_OP) && desc == true) {
    lastNode = NULL;
    currPos = currPos + 1; // go one past
  }
  
  if((c == EQ_OP) && desc == true) {
    if(currPos == -1) {// key does not exist
      delete currNode;
      eof = true;
      return 0;
    }
    // reset cause you could miss first value
    lastNode = NULL;
    currPos = currPos + 1; // go one past
  }

  // find rightmost version of value lesser than and go left from there.
  if((c == GE_OP) && desc == true) {
    lastNode = NULL;
    delete currNode;
    currNode = NULL;
    currPos = -1;
  }

  if((c == GT_OP) && desc == true) {
    lastNode = pixh->FetchNode(currNode->GetPageRID());
    delete currNode;
    currNode = NULL;
    currPos = -1;
  }


  if(desc == false) {
    if((c == LE_OP || c == LT_OP)) {
      lastNode = pixh->FetchNode(currNode->GetPageRID());
      delete currNode;
      currNode = NULL;
      currPos = -1;
    }
    if((c == GT_OP)) {
      lastNode = NULL;
      // currNode = pixh->FetchNode(currNode->GetPageRID());
      // currNode->Print(cerr);
      // cerr << "GT curr was " << currNode->GetPageRID() << endl;
    }
    if((c == GE_OP)) {
      delete currNode;
      currNode = NULL;
      currPos = -1;
      lastNode = NULL;
    }
    if((c == EQ_OP)) {
      if(currPos == -1) { // key does not exist
        delete currNode;
        eof = true;
        return 0;
      }
      lastNode = pixh->FetchNode(currNode->GetPageRID());
      delete currNode;
      currNode = NULL;
      currPos = -1;
    }
  }

  int first = -1;
  if(currNode != NULL) first = currNode->GetPageRID().Page();
  // cerr << "first is " << first << endl;
  int last = -1;
  if(lastNode != NULL) last = lastNode->GetPageRID().Page();
  // cerr << "last  is " << last << endl;
  return 0;
}
