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

#define MAXPRINTSTRING  ((2*MAXNAME) + 5)

//
// DataAttrInfo
//
// This struct stores the information that is kept within in
// attribute catalog.  It identifies a relation name, attribute name
// and the location type and length of the attribute.
//
struct DataAttrInfo
{
    // Default constructor
    DataAttrInfo() {
       memset(relName, 0, MAXNAME + 1);
       memset(attrName, 0, MAXNAME + 1);
    };

    // Copy constructor
    DataAttrInfo( const DataAttrInfo &d ) {
       strcpy (relName, d.relName);
       strcpy (attrName, d.attrName);
       offset = d.offset;
       attrType = d.attrType;
       attrLength = d.attrLength;
       indexNo = d.indexNo;
    };

    DataAttrInfo& operator=(const DataAttrInfo &d) {
       if (this != &d) {
          strcpy (relName, d.relName);
          strcpy (attrName, d.attrName);
          offset = d.offset;
          attrType = d.attrType;
          attrLength = d.attrLength;
          indexNo = d.indexNo;
       }
       return (*this);
    };

    char     relName[MAXNAME+1];    // Relation name
    char     attrName[MAXNAME+1];   // Attribute name
    int      offset;                // Offset of attribute
    AttrType attrType;              // Type of attribute
    int      attrLength;            // Length of attribute
    int      indexNo;               // Index number of attribute
};

// Print some number of spaces
void Spaces(int maxLength, int printedSoFar);

class Printer {
public:
    // Constructor.  Takes as arguments an array of attributes along with
    // the length of the array.
    Printer(const DataAttrInfo *attributes, const int attrCount);
    ~Printer();

    void PrintHeader(std::ostream &c) const;

    // Two flavors for the Print routine.  The first takes a char* to the
    // data and is useful when the data corresponds to a single record in
    // a table -- since in this situation you can just send in the
    // RecData.  The second will be useful in the QL layer.
    void Print(std::ostream &c, const char * const data);
    void Print(std::ostream &c, const void * const data[]);

    void PrintFooter(std::ostream &c) const;

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


#ifndef min
#define min(a,b) (((a) < (b)) ? (a) : (b))
#endif

#ifndef max
#define max(a,b) (((a) > (b)) ? (a) : (b))
#endif

#endif
