//
// File:        predicate.cc
//

#include <cerrno>
#include <cassert>
#include <cstdio>
#include <iostream>
#include "predicate.h"

using namespace std;

bool Predicate::eval(const char *buf, CompOp c) const {
  if(c == NO_OP || value == NULL) {
    return true;
  }
  const char * attr = buf + attrOffset;
    
  if(c == LT_OP) {
    if(attrType == INT) {
      return *attr < *((int *)value);
    }
    if(attrType == FLOAT) {
      return *attr < *((float *)value);
    }
    if(attrType == STRING) {
      return strncmp(attr, (char *)value, attrLength) < 0;
    }
  }
  if(c == GT_OP) {
    if(attrType == INT) {
      return *attr > *((int *)value);
    }
    if(attrType == FLOAT) {
      return *attr > *((float *)value);
    }
    if(attrType == STRING) {
      return strncmp(attr, (char *)value, attrLength) > 0;
    }
  }
  if(c == EQ_OP) {
    if(attrType == INT) {
      return *attr == *((int *)value);
    }
    if(attrType == FLOAT) {
      return *attr == *((float *)value);
    }
    if(attrType == STRING) {
      return strncmp(attr, (char *)value, attrLength) == 0;
    }
  }
  if(c == LE_OP) {
    return this->eval(buf, LT_OP) || this->eval(buf, EQ_OP); 
  }
  if(c == GE_OP) {
    return this->eval(buf, GT_OP) || this->eval(buf, EQ_OP); 
  }
  if(c == NE_OP) {
    return !this->eval(buf, EQ_OP);
  }
  assert("Bad value for c - should never get here.");
  return true;
}
