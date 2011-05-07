//
// File:        iterator.h
//

#ifndef ITERATOR_H
#define ITERATOR_H

#include "redbase.h"
#include "printer.h"

using namespace std;

// abstraction to hide details of offsets and type conversions
class Tuple {
 public:
  Tuple(int ct, int length_): count(ct), length(length_) {
    data = new char[length];
  }
  ~Tuple() { delete [] data; }
  int GetLength() const { return length; }
  int GetAttrCount() const { return count; }
  DataAttrInfo* GetAttributes() const { return attrs; }
  void Set(const char * buf) {
    assert(buf != NULL);
    memcpy(data, buf, length);
  }
  void GetData(char *& buf) { buf = data; }
  void SetAttr(DataAttrInfo* pa) { attrs = pa; }

  void Get(const char* attrName, int& intAttr) const {
    assert(attrs != NULL);
    for (int i = 0; i < count; i++) {
      if(strcmp(attrs[i].attrName, attrName) == 0) {
        intAttr = *(int*)(data+attrs[i].offset);
        return;
      }
    }
  }
  void Get(const char* attrName, float& floatAttr) const {
    assert(attrs != NULL);
    for (int i = 0; i < count; i++) {
      if(strcmp(attrs[i].attrName, attrName) == 0) {
        floatAttr = *(float*)(data+attrs[i].offset);
        return;
      }
    }
  }
  void Get(const char* attrName, char strAttr[]) const {
    assert(attrs != NULL);
    for (int i = 0; i < count; i++) {
      if(strcmp(attrs[i].attrName, attrName) == 0) {
        strncpy(strAttr,
                (char*)(data+attrs[i].offset),
                attrs[i].attrLength);
        return;
      }
    }
  }
 private:
  char * data;
  DataAttrInfo * attrs;
  int count;
  int length;
};

class Iterator {
 public:
  Iterator() {}
  virtual ~Iterator() {}

  virtual RC Open() = 0;
  virtual RC GetNext(Tuple &t) = 0;
  virtual RC Close() = 0;

  // compare the return of GetNext() with Eof() to know when iterator has
  // finished
  virtual RC Eof() const = 0;

  // return must be good enough to use with Tuple::SetAttr()
  virtual DataAttrInfo* GetAttr() const = 0;
};

#endif // ITERATOR_H
