//
// File:        nested_loop_join.h
//

#ifndef NESTEDLOOPJOIN_H
#define NESTEDLOOPJOIN_H

#include "redbase.h"
#include "iterator.h"
#include "ix.h"
#include "sm.h"
#include "rm.h"

using namespace std;

class NestedLoopJoin: public Iterator {
 public:
  NestedLoopJoin(DataAttrInfo    in1[],  // Array containing field types of R.
                 int     len_in1,        // # of columns in R.
                 DataAttrInfo    in2[],  // Array containing field types of S.
                 int     len_in2,        // # of columns in S.                 
                 Iterator *    am1,      // access for left i/p to join -R
                 Iterator *    am2,      // access for right i/p to join -S
                 
                 //Cond **outFilter,   // Ptr to the output filter
                 //Cond **rightFilter, // Ptr to filter applied on right i
                 //FldSpec  * proj_list,
                 // int        n_out_flds,
                 RC& status);

  virtual ~NestedLoopJoin();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();

  RC IsValid();
  virtual RC Eof() const { return IX_EOF; }
  virtual DataAttrInfo* GetAttr() const { return attrs; }
  virtual int GetAttrCount() const { return attrCount; }

 private:
  IX_NestedLoopJoin ifs;
  IX_Manager* pixm;
  RM_Manager* prmm;
  RM_FileHandle rmh;
  IX_IndexHandle ixh;
  // used for iterator interface
  bool bIterOpen;
  DataAttrInfo* attrs;
  int attrCount;
};

#endif // NESTEDLOOPJOIN_H
