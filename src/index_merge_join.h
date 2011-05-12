//
// File:        IndexMergeJoin.h
//

#ifndef INDEXMERGEJOIN_H
#define INDEXMERGEJOIN_H

#include "redbase.h"
#include "iterator.h"
#include "rm.h"

using namespace std;

class IndexMergeJoin: public Iterator {
 public:
  IndexMergeJoin(DataAttrInfo    rattr[],  // Array containing field types of R.
                 int     len_in_r,        // # of columns in R.
                 DataAttrInfo    sattr[],  // Array containing field types of S.
                 int     len_in_s,        // # of columns in S.                 
                 Iterator *    it_r,      // access for left i/p to join -R
                 Iterator *    it_s,      // access for right i/p to join -S
                 //Cond **outFilter,   // Ptr to the output filter
                 //Cond **rightFilter, // Ptr to filter applied on right i
                 //FldSpec  * proj_list,
                 //int        len_out,
                 RC& status);

  virtual ~IndexMergeJoin();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();
  
  RC IsValid();
  virtual RC Eof() const { return IX_EOF; }
  virtual DataAttrInfo* GetAttr() const { return attrs; }
  virtual int GetAttrCount() const { return attrCount; }

 private:
  IX_Manager* pixm;
  RM_Manager* prmm;
  RM_FileHandle rmh;
  // used for iterator interface
  bool bIterOpen;
  DataAttrInfo* attrs;
  int attrCount;
};

#endif // INDEXMERGEJOIN_H
