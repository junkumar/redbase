//
// catalog.h
//
//
// This file defines structures useful for describing the DB catalogs
//

#ifndef CATALOG_H
#define CATALOG_H

#include "printer.h"
#include "parser.h"

// in printer.h
// struct DataAttrInfo {
//     char     relName[MAXNAME+1];    // Relation name
//     char     attrName[MAXNAME+1];   // Attribute name
//     int      offset;                // Offset of attribute
//     AttrType attrType;              // Type of attribute
//     int      attrLength;            // Length of attribute
//     int      indexNo;               // Index number of attribute
// }

struct DataRelInfo
{
  // Default constructor
  DataRelInfo() {
    memset(relName, 0, MAXNAME + 1);
  }

  DataRelInfo( char * buf ) {
    memcpy(this, buf, DataRelInfo::size());
  }

  // Copy constructor
  DataRelInfo( const DataRelInfo &d ) {
    strcpy (relName, d.relName);
    recordSize = d.recordSize;
    attrCount = d.attrCount;
    numPages = d.numPages;
    numRecords = d.numRecords;
  };

  DataRelInfo& operator=(const DataRelInfo &d) {
    if (this != &d) {
      strcpy (relName, d.relName);
      recordSize = d.recordSize;
      attrCount = d.attrCount;
      numPages = d.numPages;
      numRecords = d.numRecords;
    }
    return (*this);
  }

  static unsigned int size() { 
    return (MAXNAME+1) + 4*sizeof(int);
  }

  static unsigned int members() { 
    return 5;
  }

  int      recordSize;            // Size per row
  int      attrCount;             // # of attributes
  int      numPages;              // # of pages used by relation
  int      numRecords;            // # of records in relation
  char     relName[MAXNAME+1];    // Relation name
};


#endif // CATALOG_H
