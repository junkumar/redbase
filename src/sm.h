//
// sm.h
//   Data Manager Component Interface
//

#ifndef SM_H
#define SM_H

// Please do not include any other files than the ones below in this file.

#include "sm_error.h"
#include <stdlib.h>
#include <string.h>
#include "redbase.h"  // Please don't change these lines
#include "parser.h"
#include "rm.h"
#include "ix.h"
#include "catalog.h"
#include <string>
#include <map>

//
// SM_Manager: provides data management
//
class SM_Manager {
  friend class QL_Manager;
 public:
  SM_Manager    (IX_Manager &ixm_, RM_Manager &rmm_);
  ~SM_Manager   ();                             // Destructor

  RC OpenDb     (const char *dbName);           // Open the database
  RC CloseDb    ();                             // close the database

  RC CreateTable(const char *relName,           // create relation relName
                 int        attrCount,          //   number of attributes
                 AttrInfo   *attributes);       //   attribute data
  RC CreateIndex(const char *relName,           // create an index for
                 const char *attrName);         //   relName.attrName
  RC DropTable  (const char *relName);          // destroy a relation

  RC DropIndex  (const char *relName,           // destroy index on
                 const char *attrName);         //   relName.attrName
  RC Load       (const char *relName,           // load relName from
                 const char *fileName);         //   fileName
  RC Help       ();                             // Print relations in db
  RC Help       (const char *relName);          // print schema of relName

  RC Print      (const char *relName);          // print relName contents

  RC Set        (const char *paramName,         // set parameter to
                 const char *value);            //   value
  RC Get        (const string& paramName,       // get parameter's
                 string& value) const;          //   value

 public:
  RC IsValid() const;
  
  // attributes is allocated and returned back with attrCount elements.
  // attrCount is returned back with number of attributes
  RC GetFromTable(const char *relName,           // create relation relName
                  int&        attrCount,         // number of attributes
                  DataAttrInfo   *&attributes);  // attribute data
  
  // Get the first matching row for relName
  // contents are return in rel and the RID the record is located at is
  // returned in rid.
  // method returns SM_NOSUCHTABLE if relName was not found
  RC GetRelFromCat(const char* relName, 
                   DataRelInfo& rel,
                   RID& rid) const;

  // Get the first matching row for relName, attrName
  // contents are returned in attr
  // location of record is returned in rid
  // method returns SM_NOSUCHENTRY if attrName was not found
  RC GetAttrFromCat(const char* relName,
                    const char* attrName,
                    DataAttrInfo& attr,
                    RID& rid) const;

  RC GetNumPages(const char* relName) const;
  RC GetNumRecords(const char* relName) const;

  // Semantic checks for various parts of queries
  RC SemCheck(const char* relName) const;
  RC SemCheck(const RelAttr& ra) const;
  RC SemCheck(const AggRelAttr& ra) const;
  RC SemCheck(const Condition& cond) const;
  // for NULL relname - implicit relation name
  // return error if there is a clash and multiple relations have this attrName
  // user must free() ra.relName eventually
  RC FindRelForAttr(RelAttr& ra, int nRelations, 
                    const char * const
                    possibleRelations[]) const;
  
// Load a single record in the buf into the table relName
  RC LoadRecord(const char *relName,
                int buflen,
                const char buf[]);

  bool IsAttrIndexed(const char* relName, const char* attrName) const;
                                                                
  // temp operations on attrcat to make index appear to be missing
  RC DropIndexFromAttrCatAlone(const char *relName,
                               const char *attrName);
  RC ResetIndexFromAttrCatAlone(const char *relName,
                                const char *attrName);

 private:
  RM_Manager& rmm;
  IX_Manager& ixm;
  bool bDBOpen;
  RM_FileHandle relfh;
  RM_FileHandle attrfh;
  char cwd[1024];
  map<string, string> params;
};

#endif // SM_H
