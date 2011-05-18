//
// printer.h
//

// This file contains the interface for the Printer class and some
// functions that will be used by both the SM and QL components.

#ifndef _HELPER
#define _HELPER

#include <iostream>
#include <cstring>
#include "redbase.h"      // For definition of MAXNAME
#include "catalog.h"
#include "data_attr_info.h"
#include "iterator.h"

#define MAXPRINTSTRING  ((2*MAXNAME) + 5)

// Print some number of spaces
void Spaces(int maxLength, int printedSoFar);

class DataAttrInfo;
class Tuple;

class Printer {
 public:
  // Constructor.  Takes as arguments an array of attributes along with
  // the length of the array.
  Printer(const DataAttrInfo *attributes, const int attrCount);
  Printer(const Tuple& t);

  ~Printer();

  void PrintHeader(std::ostream &c) const;

  // Two flavors for the Print routine.  The first takes a char* to the
  // data and is useful when the data corresponds to a single record in
  // a table -- since in this situation you can just send in the
  // RecData.  The second will be useful in the QL layer.
  void Print(std::ostream &c, const char * const data);
  void Print(std::ostream &c, const void * const data[]);
  void Print(std::ostream &c, const Tuple& t);

  void PrintFooter(std::ostream &c) const;
  
 private:
  void Init(const DataAttrInfo *attributes_, const int attrCount_);

 private:
  DataAttrInfo *attributes;
  int attrCount;

  // An array of strings for the header information
  char **psHeader;
  // Number of spaces between each attribute
  int *spaces;

  // The number of tuples printed
  int iCount;
};


#ifndef mmin
#define mmin(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef mmax
#define mmax(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif
