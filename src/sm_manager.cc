//
// File:        SM component stubs
// Description: Print parameters of all SM_Manager methods
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//

#include <cstdio>
#include <iostream>
#include <fstream>
#include <sstream>
#include "redbase.h"
#include "sm.h"
#include "ix.h"
#include "rm.h"
#include "printer.h"
#include "catalog.h"

using namespace std;

SM_Manager::SM_Manager(IX_Manager &ixm_, RM_Manager &rmm_)
  :rmm(rmm_), ixm(ixm_), bDBOpen(false)
{
  memset(cwd, 0, 1024);
}

SM_Manager::~SM_Manager()
{
}

RC SM_Manager::OpenDb(const char *dbName)
{
  if(dbName == NULL)
    return SM_BADOPEN;
  if(bDBOpen) {
    return SM_DBOPEN; 
  }

  if (getcwd(cwd, 1024) < 0) {
    cerr << " getcwd error." << endl;
    return SM_NOSUCHDB;
  }

  if (chdir(dbName) < 0) {
    cerr << " chdir error to " << dbName 
         << ". Does the db exist ?\n";
    return SM_NOSUCHDB;
  }

  RC rc;
  if((rc =	rmm.OpenFile("attrcat", attrfh))
     || (rc =	rmm.OpenFile("relcat", relfh))
    ) 
    return(rc);
  
  bDBOpen = true;
  return (0);
}

RC SM_Manager::CloseDb()
{
  if(!bDBOpen) {
    return SM_NOTOPEN; 
  }
  RC rc;
  if((rc =	rmm.CloseFile(attrfh))
     || (rc =	rmm.CloseFile(relfh))
    ) 
    return(rc);

  if (chdir(cwd) < 0) {
    cerr << " chdir error to " << cwd 
         << ".\n";
    return SM_NOSUCHDB;
  }

  bDBOpen = false;
  return (0);
}

RC SM_Manager::CreateTable(const char *relName,
                           int        attrCount,
                           AttrInfo   *attributes)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(relName == NULL || attrCount <= 0 || attributes == NULL) {
    return SM_BADTABLE;
  }

  if(strcmp(relName, "relcat") == 0 ||
     strcmp(relName, "attrcat") == 0
    ) {
    return SM_BADTABLE;
  }

  RID rid;
  RC rc;

  DataAttrInfo * d = new DataAttrInfo[attrCount];
  int size = 0;
  for (int i = 0; i < attrCount; i++) {
    d[i] = DataAttrInfo(attributes[i]);
    d[i].offset = size;
    size += attributes[i].attrLength;
    strcpy (d[i].relName, relName);

    if ((rc = attrfh.InsertRec((char*) &d[i], rid)) < 0
      )
      return(rc);
  }

  if(
    (rc = rmm.CreateFile(relName, size)) 
    ) 
    return(rc);

  DataRelInfo rel;
  strcpy(rel.relName, relName);
  rel.attrCount = attrCount;
  rel.recordSize = size;
  rel.numPages = 1; // initially
  rel.numRecords = 0;

	if ((rc = relfh.InsertRec((char*) &rel, rid)) < 0
		)
    return(rc);  

  // attrfh.ForcePages();
  // relfh.ForcePages();
  delete [] d;
  return (0);
}

// Get the first matching row for relName
// contents are return in rel and the RID the record is located at is
// returned in rid.
// method returns SM_NOSUCHTABLE if relName was not found
RC SM_Manager::GetRelFromCat(const char* relName, 
                             DataRelInfo& rel,
                             RID& rid) const
{
  RC invalid = IsValid(); if(invalid) return invalid;
  if(relName == NULL)
    return SM_BADTABLE;

  void * value = const_cast<char*>(relName);
  RM_FileScan rfs;
  RC rc = rfs.OpenScan(relfh,STRING,MAXNAME+1,offsetof(DataRelInfo, relName),
                       EQ_OP, value, NO_HINT);
  if (rc != 0 ) return rc;

  RM_Record rec;
  rc = rfs.GetNextRec(rec);
  if(rc == RM_EOF)
    return SM_NOSUCHTABLE; // no such table

  rc = rfs.CloseScan();
  if (rc != 0 ) return rc;

  DataRelInfo * prel;
  rec.GetData((char *&) prel);
  rel = *prel;
  rec.GetRid(rid);

  return 0;
}

// Get the first matching row for relName, attrName
// contents are returned in attr
// location of record is returned in rid
// method returns SM_NOSUCHENTRY if attrName was not found
RC SM_Manager::GetAttrFromCat(const char* relName,
                              const char* attrName,
                              DataAttrInfo& attr,
                              RID& rid) const
{
  RC invalid = IsValid(); if(invalid) return invalid;
  if(relName == NULL || attrName == NULL) {
    return SM_BADTABLE;
  }

  RC rc;
  RM_FileScan rfs;
  RM_Record rec;
  DataAttrInfo * data;
  if ((rc = rfs.OpenScan(attrfh,
                         STRING,
                         MAXNAME+1,
                         offsetof(DataAttrInfo, relName),
                         EQ_OP,
                         (void*) relName))) 
    return (rc);

  bool attrFound = false;
  while (rc!=RM_EOF) {
    rc = rfs.GetNextRec(rec);

    if (rc!=0 && rc!=RM_EOF)
      return (rc);

    if (rc!=RM_EOF) {
      rec.GetData((char*&)data);
      if(strcmp(data->attrName, attrName) == 0) {
        attrFound = true;
        break;
      }
    }
  }

  if ((rc = rfs.CloseScan()))
    return (rc);

  if(!attrFound)
    return SM_BADATTR;
  
  attr = *data;
  rec.GetRid(rid);
  return 0;
}



RC SM_Manager::DropTable(const char *relName)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(relName == NULL) {
    return SM_BADTABLE;
  }

  if(strcmp(relName, "relcat") == 0 ||
     strcmp(relName, "attrcat") == 0
    ) {
    return SM_BADTABLE;
  }
          
  RM_FileScan rfs;
  RM_Record rec;
  DataRelInfo * data;
  RC rc;
  if ((rc = rfs.OpenScan(relfh,
                         STRING,
                         MAXNAME+1,
                         offsetof(DataRelInfo, relName),
                         EQ_OP,
                         (void*) relName))) 
    return (rc);

  bool attrFound = false;
  while (rc!=RM_EOF) {
    rc = rfs.GetNextRec(rec);

    if (rc!=0 && rc!=RM_EOF)
      return (rc);

    if (rc!=RM_EOF) {
      rec.GetData((char*&)data);
      if(strcmp(data->relName, relName) == 0) {
        attrFound = true;
        break;
      }
    }
  }

  if ((rc = rfs.CloseScan()))
    return (rc);

  if(!attrFound)
    return SM_NOSUCHTABLE;
  
  RID rid;
  rec.GetRid(rid);

  if(
    (rc = rmm.DestroyFile(relName)) 
    )
    return(rc);
  
  // update relcat
  if ((rc = relfh.DeleteRec(rid)) != 0)
    return rc;

  {
    RM_Record rec;
    DataAttrInfo * adata;
    if ((rc = rfs.OpenScan(attrfh,
                           STRING,
                           MAXNAME+1,
                           offsetof(DataAttrInfo, relName),
                           EQ_OP,
                           (void*) relName))) 
      return (rc);
    
    while (rc!=RM_EOF) {
      rc = rfs.GetNextRec(rec);
      
      if (rc!=0 && rc!=RM_EOF)
      return (rc);
      
      if (rc!=RM_EOF) {
        rec.GetData((char*&)adata);
        if(strcmp(adata->relName, relName) == 0) {
          if(adata->indexNo != -1) // drop indexes also
            this->DropIndex(relName, adata->attrName);
          RID rid;
          rec.GetRid(rid);
          if ((rc = attrfh.DeleteRec(rid)) != 0)
            return rc;
        }
      }
    }
    
    if ((rc = rfs.CloseScan()))
      return (rc);
  }
  return (0);
}

RC SM_Manager::CreateIndex(const char *relName,
                           const char *attrName)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(relName == NULL || attrName == NULL) {
    return SM_BADTABLE;
  }

  DataAttrInfo attr;
  DataAttrInfo * data = &attr;

  RC rc;
  RID rid;
  rc = GetAttrFromCat(relName, attrName, attr, rid);
  if(rc != 0) return rc;

  // index already exists
  if(data->indexNo != -1)
    return SM_INDEXEXISTS;
  // otherwise here is a new one
  data->indexNo = data->offset;

  if(
    (rc = ixm.CreateIndex(relName, data->indexNo, 
                          data->attrType, data->attrLength)) 
    )
    return(rc);

  // update attrcat
  RM_Record rec;
  rec.Set((char*)data, DataAttrInfo::size(), rid);
  if ((rc = attrfh.UpdateRec(rec)) != 0)
    return rc;
  return (0);
}

RC SM_Manager::DropIndex(const char *relName,
                         const char *attrName)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(relName == NULL || attrName == NULL) {
    return SM_BADTABLE;
  }

  RM_FileScan rfs;
  RM_Record rec;
  DataAttrInfo * data;
  RC rc;
  if ((rc = rfs.OpenScan(attrfh,
                         STRING,
                         MAXNAME+1,
                         offsetof(DataAttrInfo, relName),
                         EQ_OP,
                         (void*) relName))) 
    return (rc);

  bool attrFound = false;
  while (rc!=RM_EOF) {
    rc = rfs.GetNextRec(rec);

    if (rc!=0 && rc!=RM_EOF)
      return (rc);

    if (rc!=RM_EOF) {
      rec.GetData((char*&)data);
      if(strcmp(data->attrName, attrName) == 0) {
        data->indexNo = -1;
        attrFound = true;
        break;
      }
    }
  }

  if ((rc = rfs.CloseScan()))
    return (rc);

  if(!attrFound)
    return SM_BADATTR;
  
  RID rid;
  rec.GetRid(rid);

  if(
    (rc = ixm.DestroyIndex(relName, data->offset))
    )
    return(rc);

  // update attrcat
  rec.Set((char*)data, DataAttrInfo::size(), rid);
  if ((rc = attrfh.UpdateRec(rec)) != 0)
    return rc;
  return (0);
}

RC SM_Manager::Load(const char *relName,
                    const char *fileName)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(relName == NULL || fileName == NULL) {
    return SM_BADTABLE;
  }

  ifstream ifs(fileName);
  if(ifs.fail())
    return SM_BADTABLE;
  
  RM_FileHandle rfh;
  RC rc;
  if((rc =	rmm.OpenFile(relName, rfh))
    ) 
    return(rc);

  int attrCount = -1;
  DataAttrInfo * attributes;
  rc = GetFromTable(relName, attrCount, attributes);
  if(rc != 0) return rc;

  IX_IndexHandle * indexes = new IX_IndexHandle[attrCount];

  int size = 0;
  for (int i = 0; i < attrCount; i++) {
    size += attributes[i].attrLength;
    if(attributes[i].indexNo != -1) {
      ixm.OpenIndex(relName, attributes[i].indexNo, indexes[i]);
    }
  }

  char * buf = new char[size];
  int numLines = 0;
  while(!ifs.eof()) {
    string line;
    getline(ifs, line);
    if(line.length() == 0) continue; // ignore last newline
    numLines++;
    // cerr << "Line " << numLines << ": " << line << endl;
    // tokenize line
    string token;
    std::istringstream iss(line);
    int i = 0;
    while ( getline(iss, token, ',') )
    {
      assert(i < attrCount);
      istringstream ss(token);
      if(attributes[i].attrType == INT) {
        int val;
        ss >> val;
        memcpy(buf + attributes[i].offset , &val, attributes[i].attrLength);
      }
      if(attributes[i].attrType == FLOAT) {
        float val;
        ss >> val;
        memcpy(buf + attributes[i].offset, &val, attributes[i].attrLength);
      }
      if(attributes[i].attrType == STRING) {
        string val = token;
        // cerr << "Line " << numLines << ": token : " << token << endl;
        if(val.length() > attributes[i].attrLength) {
          cerr << "SM_Manager::Load truncating to " 
               << attributes[i].attrLength << " - " << val << endl; 
        }
        memcpy(buf + attributes[i].offset, val.c_str(), attributes[i].attrLength);
      }
      i++;
    }
    RID rid;
    if ((rc = rfh.InsertRec(buf, rid)) < 0
      )
      return(rc);
    for (int i = 0; i < attrCount; i++) {
      if(attributes[i].indexNo != -1) {
        indexes[i].InsertEntry(buf + attributes[i].offset, 
                               rid);
      }
    }

  }

  delete [] buf;
  delete [] attributes;

  // update numRecords in relcat
  DataRelInfo r;
  RID rid;
  rc = GetRelFromCat(relName, r, rid);
  if(rc != 0) return rc;

  r.numRecords += numLines;
  r.numPages = rfh.GetNumPages();
  RM_Record rec;
  rec.Set((char*)&r, DataRelInfo::size(), rid);
  if ((rc = relfh.UpdateRec(rec)) != 0)
    return rc;

  if((rc =	rmm.CloseFile(rfh))
    ) 
    return(rc);

  for (int i = 0; i < attrCount; i++) {
    if(attributes[i].indexNo != -1) {
      ixm.CloseIndex(indexes[i]);
    }
  }

  delete [] indexes;
  ifs.close();
  return (0);
}

RC SM_Manager::Print(const char *relName)
{
  RC invalid = IsValid(); if(invalid) return invalid;
  DataAttrInfo *attributes;
  RM_FileHandle rfh;
  RM_FileHandle* prfh;

  int attrCount;
  RM_Record rec;
  char *data;
  RC rc;

  if(strcmp(relName, "relcat") == 0)
    prfh = &relfh;
  else if(strcmp(relName, "attrcat") == 0)
    prfh = &attrfh;
  else {
    rc = rmm.OpenFile(relName, rfh);
    if (rc !=0) return rc;
    prfh = &rfh;
  }

  rc = GetFromTable(relName, attrCount, attributes);
  if (rc !=0) return rc;

  // Instantiate a Printer object and print the header information
  Printer p(attributes, attrCount);
  p.PrintHeader(cout);

  RM_FileScan rfs;

  if ((rc = rfs.OpenScan(*prfh, INT, sizeof(int), 0, NO_OP, NULL))) 
    return (rc);

  // Print each tuple
  while (rc!=RM_EOF) {
    rc = rfs.GetNextRec(rec);

    if (rc!=0 && rc!=RM_EOF)
      return (rc);

    if (rc!=RM_EOF) {
      rec.GetData(data);
      p.Print(cout, data);
    }
  }

  // Print the footer information
  p.PrintFooter(cout);

  // Close the scan, file, delete the attributes pointer, etc.
  if((rc = rfs.CloseScan()))
    return (rc);
   
  if((0 == rfh.IsValid())) {
    if (rc = rmm.CloseFile(rfh))
      return (rc);
  }

  delete [] attributes;
  return (0);
}

RC SM_Manager::Set(const char *paramName, const char *value)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  // cout << "Set\n"
  //      << "   paramName=" << paramName << "\n"
  //      << "   value    =" << value << "\n";
  return (0);
}

RC SM_Manager::Help()
{
  RC invalid = IsValid(); if(invalid) return invalid;
  DataAttrInfo * attributes;
  DataAttrInfo nameattr[1];
  int attrCount;
  RM_Record rec;
  char *data;
  RC rc;

  rc = GetFromTable("relcat", attrCount, attributes);
  if (rc !=0) return rc;

  for (int i = 0; i < attrCount; i++) {
    if(strcmp(attributes[i].attrName, "relName") == 0)
      nameattr[0] = attributes[i];
  }

  // Instantiate a Printer object and print the header information
  Printer p(nameattr, 1);
  p.PrintHeader(cout);

  RM_FileScan rfs;

  if ((rc = rfs.OpenScan(relfh, INT, sizeof(int), 0, NO_OP, NULL))) 
    return (rc);

  // Print each tuple
  while (rc!=RM_EOF) {
    rc = rfs.GetNextRec(rec);

    if (rc!=0 && rc!=RM_EOF)
      return (rc);

    if (rc!=RM_EOF) {
      rec.GetData(data);
      p.Print(cout, data);
    }
  }

  // Print the footer information
  p.PrintFooter(cout);

  // Close the scan, file, delete the attributes pointer, etc.
  if ((rc = rfs.CloseScan()))
    return (rc);
   
  delete [] attributes;
  return (0);
}

RC SM_Manager::Help(const char *relName)
{
  RC invalid = IsValid(); if(invalid) return invalid;
  DataAttrInfo * attributes;
  DataAttrInfo nameattr[DataAttrInfo::members()-1];
  DataAttrInfo relinfo;
  int attrCount;
  RM_Record rec;
  char *data;
  RC rc;

  rc = GetFromTable("attrcat", attrCount, attributes);
  if (rc !=0) return rc;

  int j = 0;
  for (int i = 0; i < attrCount; i++) {
    if(strcmp(attributes[i].attrName, "relName") != 0) {
      nameattr[j] = attributes[i];
      j++;
    } else {
      relinfo = attributes[i];
    }
  }

  // Instantiate a Printer object and print the header information
  Printer p(nameattr, attrCount-1);
  p.PrintHeader(cout);

  RM_FileScan rfs;

  if ((rc = rfs.OpenScan(attrfh,
                         STRING,
                         relinfo.attrLength,
                         relinfo.offset,
                         EQ_OP,
                         (void*) relName))) 
    return (rc);

  // Print each tuple
  while (rc!=RM_EOF) {
    rc = rfs.GetNextRec(rec);

    if (rc!=0 && rc!=RM_EOF)
      return (rc);

    if (rc!=RM_EOF) {
      rec.GetData(data);
      p.Print(cout, data);
    }
  }

  // Print the footer information
  p.PrintFooter(cout);

  // Close the scan, file, delete the attributes pointer, etc.
  if ((rc = rfs.CloseScan()))
    return (rc);
   
  delete [] attributes;

  return (0);
}

// Users will call - RC invalid = IsValid(); if(invalid) return invalid; 
RC SM_Manager::IsValid () const
{
  bool ret = true;
  ret = ret && bDBOpen;
  return ret ? 0 : SM_BADOPEN;
}

// attributes is allocated and returned. destruction is user's
// responsibility
RC SM_Manager::GetFromTable(const char *relName,
                            int&        attrCount,
                            DataAttrInfo   *&attributes)
{
  RC invalid = IsValid(); if(invalid) return invalid;

  if(relName == NULL)
    return SM_NOSUCHTABLE;

  void * value = const_cast<char*>(relName);
  RM_FileScan rfs;
  RC rc = rfs.OpenScan(relfh,STRING,MAXNAME+1,offsetof(DataRelInfo, relName),
                       EQ_OP, value, NO_HINT);
  if (rc != 0 ) return rc;

  RM_Record rec;
  rc = rfs.GetNextRec(rec);
  if(rc == RM_EOF)
    return SM_NOSUCHTABLE; // no such table

  DataRelInfo * prel;
  rec.GetData((char *&) prel);

  rc = rfs.CloseScan();
  if (rc != 0 ) return rc;

  attrCount = prel->attrCount;
  attributes = new DataAttrInfo[attrCount];

  RM_FileScan afs;
  rc = afs.OpenScan(attrfh,STRING,MAXNAME+1,offsetof(DataAttrInfo, relName),
                    EQ_OP, value, NO_HINT);

  int numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = afs.GetNextRec(rec);
    if(rc == RM_EOF || numRecs > attrCount)
      break;
    DataAttrInfo * pattr;
		rec.GetData((char*&)pattr);
    attributes[numRecs] = *pattr;
    numRecs++;
  }
  
  if(numRecs != attrCount) {
    // too few or too many
    return SM_BADTABLE;
  }

  rc = afs.CloseScan();
  if (rc != 0 ) return rc;

  return 0;
}
