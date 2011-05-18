//
// File:        iterator.h
//

#ifndef ITERATOR_H
#define ITERATOR_H

#include "redbase.h"
#include "data_attr_info.h"
#include <sstream>

using namespace std;

class DataAttrInfo;

// abstraction to hide details of offsets and type conversions
class Tuple {
 public:
  Tuple(int ct, int length_): count(ct), length(length_) {
    data = new char[length];
  }
  Tuple(const Tuple& rhs): count(rhs.count), length(rhs.length) {
    data = new char[length];
    memcpy(data, rhs.data, length);
    SetAttr(rhs.GetAttributes());
  }

  ~Tuple() { delete [] data; }
  int GetLength() const { return length; }
  int GetAttrCount() const { return count; }
  DataAttrInfo* GetAttributes() const { return attrs; }
  void Set(const char * buf) {
    assert(buf != NULL);
    memcpy(data, buf, length);
  }
  void GetData(const char *& buf) const { buf = data; }
  void GetData(char *& buf) { buf = data; }
  void SetAttr(DataAttrInfo* pa) { attrs = pa; }

  void Get(const char* attrName, void*& p) const {
    assert(attrs != NULL);
    for (int i = 0; i < count; i++) {
      if(strcmp(attrs[i].attrName, attrName) == 0) {
        p = (data+attrs[i].offset);
        return;
      }
    }
  }

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

namespace {
  std::ostream &operator<<(std::ostream &os, const Tuple &t) {
    os << "{";
    DataAttrInfo* attrs = t.GetAttributes();    

    for (int pos = 0; pos < t.GetAttrCount(); pos++) {
      void * k = NULL;
      AttrType attrType = attrs[pos].attrType;
      t.Get(attrs[pos].attrName, k);
      if( attrType == INT )
        os << *((int*)k);
      if( attrType == FLOAT )
        os << *((float*)k);
      if( attrType == STRING ) {
        for(int i=0; i < attrs[pos].attrLength; i++) {
          if(((char*)k)[i] == 0) break;
          os << ((char*)k)[i];
        }
      }
      os << ", ";
    }
    os << "\b\b";
    os << "}";
    return os;
  }
};

class Iterator {
 public:
  Iterator():bIterOpen(false) {}
  virtual ~Iterator() {}

  virtual RC Open() = 0;
  virtual RC GetNext(Tuple &t) = 0;
  virtual RC Close() = 0;

  // compare the return of GetNext() with Eof() to know when iterator has
  // finished
  virtual RC Eof() const = 0;

  // return must be good enough to use with Tuple::SetAttr()
  virtual DataAttrInfo* GetAttr() const { return attrs; }
  virtual int GetAttrCount() const { return attrCount; }

  virtual int TupleLength() const {
    int l = 0;
    DataAttrInfo* a = GetAttr();
    for(int i = 0; i < GetAttrCount(); i++) 
      l += a[i].attrLength;
    return l;
  }
  
  virtual string Explain() const { return explain.str(); }

 protected:
  bool bIterOpen;
  DataAttrInfo* attrs;
  int attrCount;
  stringstream explain;
};

#endif // ITERATOR_H
