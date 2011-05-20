//
// data_attr_info.h
//


#ifndef _ATTR_INFO
#define _ATTR_INFO

#include <iostream>
#include <cstring>
#include "redbase.h"      // For definition of MAXNAME
#include "catalog.h"

/* ostream &operator<<(ostream &s, const DataAttrInfo &ai) */
/* { */
/*    return */
/*       s << " relName=" << ai.relName */
/*         << " attrName=" << ai.attrName */
/*         << " attrType=" <<  */
/*      (ai.attrType == INT ? "INT" : */
/*       ai.attrType == FLOAT ? "FLOAT" : "STRING") */
/*         << " attrLength=" << ai.attrLength */
/*         << " offset=" << ai.offset */
/*         << " indexNo=" << ai.indexNo; */
/* } */

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
    offset = -1;
  };

  DataAttrInfo(const AttrInfo &a ) {
    memset(attrName, 0, MAXNAME + 1);
    strcpy (attrName, a.attrName);
    attrType = a.attrType;
    attrLength = a.attrLength;
    memset(relName, 0, MAXNAME + 1);
    indexNo = -1;
    offset = -1;
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

  static unsigned int size() { 
    return 2*(MAXNAME+1) + sizeof(AttrType) + 3*sizeof(int);
  }

  static unsigned int members() { 
    return 6;
  }

  int      offset;                // Offset of attribute
  AttrType attrType;              // Type of attribute
  int      attrLength;            // Length of attribute
  int      indexNo;               // Index number of attribute
  char     relName[MAXNAME+1];    // Relation name
  char     attrName[MAXNAME+1];   // Attribute name
};


#endif // _ATTR_INFO
