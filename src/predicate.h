//
// File:        predicate.h
//

#ifndef PRED_H
#define PRED_H

#include <cerrno>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "redbase.h"

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

  CompOp initOp() const { return compOp; }
  bool eval(const char *buf, CompOp c) const;
  bool eval(const char *lhsBuf, const char* rhsValue, CompOp c) const;

 private:
  AttrType   attrType;
  int        attrLength;
  int        attrOffset;
  CompOp     compOp;
  void*      value;
  ClientHint pinHint;
};

#endif // PRED_H
