//
// File:        predicate.cc
//

#include <cerrno>
#include <cassert>
#include <cstdio>
#include <cstring>
#include <iostream>
#include <type_traits>
#include "predicate.h"

using namespace std;

// Template function for safe unaligned reads
template<typename T>
T read_unaligned(const void* ptr) {
    static_assert(std::is_trivially_copyable<T>::value, "T must be trivially copyable");
    T result;
    std::memcpy(&result, ptr, sizeof(T));
    return result;
}

bool AlmostEqualRelative(float A, float B, float maxRelativeError=0.000001)
{
    if (A == B)
        return true;
    float relativeError = fabs((A - B) / B);
    if (relativeError <= maxRelativeError)
        return true;
    return false;
}


bool Predicate::eval(const char *buf, CompOp c) const {
  return this->eval(buf, NULL, c);
}

bool Predicate::eval(const char *buf, const char* rhs, CompOp c) const {
  const void * value_ = rhs;
  if(rhs == NULL)
    value_ = value;

  if(c == NO_OP || value_ == NULL) {
    return true;
  }
  const char * attr = buf + attrOffset;

  // cerr << "Predicate::eval " << *(reinterpret_cast<const int*>(attr)) << " " << *(reinterpret_cast<const int*>(value_)) << endl;

  if(c == LT_OP) {
    if(attrType == INT) {
      return read_unaligned<int>(attr) < read_unaligned<int>(value_);
    }
    if(attrType == FLOAT) {
      return read_unaligned<float>(attr) < read_unaligned<float>(value_);
    }
    if(attrType == STRING) {
      return strncmp(attr, (char *)value_, attrLength) < 0;
    }
  }
  if(c == GT_OP) {
    if(attrType == INT) {
      return read_unaligned<int>(attr) > read_unaligned<int>(value_);
    }
    if(attrType == FLOAT) {
      return read_unaligned<float>(attr) > read_unaligned<float>(value_);
    }
    if(attrType == STRING) {
      return strncmp(attr, (char *)value_, attrLength) > 0;
    }
  }
  if(c == EQ_OP) {
    if(attrType == INT) {
      return read_unaligned<int>(attr) == read_unaligned<int>(value_);
    }
    if(attrType == FLOAT) {
      return read_unaligned<float>(attr) == read_unaligned<float>(value_);
      // Alternative: return AlmostEqualRelative(read_unaligned<float>(attr), read_unaligned<float>(value_));
    }
    if(attrType == STRING) {
      return strncmp(attr, (char *)value_, attrLength) == 0;
    }
  }
  if(c == LE_OP) {
    return this->eval(buf, rhs, LT_OP) || this->eval(buf, rhs, EQ_OP); 
  }
  if(c == GE_OP) {
    return this->eval(buf, rhs, GT_OP) || this->eval(buf, rhs, EQ_OP); 
  }
  if(c == NE_OP) {
    return !this->eval(buf, rhs, EQ_OP);
  }
  assert("Bad value for c - should never get here.");
  return true;
}
