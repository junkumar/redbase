//
// File:        Sort.h
//

#ifndef SORT_H
#define SORT_H

#include "redbase.h"
#include "iterator.h"
#include "rm.h"

using namespace std;

class Sort: public Iterator {
 public:
  Sort(DataAttrInfo    attr[],
       AttrType     sortKeyType,
       int    sortKeyLength,
       int     sortKeyOffset,
       Iterator *    in,
       RC& status);

  virtual ~Sort();

  virtual RC Open();
  virtual RC GetNext(Tuple &t);
  virtual RC Close();
  
  RC IsValid();
  virtual RC Eof() const { return RM_EOF; }
  virtual DataAttrInfo* GetAttr() const { return attrs; }
  virtual int GetAttrCount() const { return attrCount; }

 private:
  bool bIterOpen;
  DataAttrInfo* attrs;
  int attrCount;
};

#endif // SORT_H
