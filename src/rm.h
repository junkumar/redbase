//
// rm.h
//
//   Record Manager component interface
//
// This file does not include the interface for the RID class.  This is
// found in rm_rid.h
//

#ifndef RM_H
#define RM_H

// Please DO NOT include any files other than redbase.h and pf.h in this
// file.  When you submit your code, the test program will be compiled
// with your rm.h and your redbase.h, along with the standard pf.h that
// was given to you.  Your rm.h, your redbase.h, and the standard pf.h
// should therefore be self-contained (i.e., should not depend upon
// declarations in any other file).

// Do not change the following includes
#include "redbase.h"
#include "rm_rid.h"
#include "pf.h"


#define RM_PAGE_LIST_END  -1       // end of list of free pages
#define RM_PAGE_USED      -2       // page is being used

//
// RM_FileHdr: Header structure for files
//
struct RM_FileHdr {
   int firstFree;     // first free page in the linked list
   int numPages;      // # of pages in the file
};

//
// RM_PageHdr: Header structure for pages
//
struct RM_PageHdr {
    int nextFree;       // nextFree can be any of these values:
                        //  - the number of the next free page
                        //  - RM_PAGE_LIST_END if this is last free page
                        //  - RM_PAGE_USED if the page is not free
};

// Justify the file header to the length of one page
const int RM_FILE_HDR_SIZE = PF_PAGE_SIZE + sizeof(RM_PageHdr);

//
// RM_Record: RM Record interface
//
class RM_Record {
	friend class RM_RecordTest;
  public:
    RM_Record ();
    ~RM_Record();

    // Return the data corresponding to the record.  Sets *pData to the
    // record contents.
    RC GetData(char *&pData) const;

		// Sets data in the record for a fixed recordsize of size.
    RC SetData(char *pData, int size);

    // Return the RID associated with the record
    RC GetRid (RID &rid) const;
  private:
		int recordSize;
		char * data;			
};

//
// RM_FileHandle: RM File interface
//
class RM_FileHandle {
public:
    RM_FileHandle ();
    RM_FileHandle (PF_FileHandle*);
		RC getPF_FileHandle(PF_FileHandle &);
    ~RM_FileHandle();

    // Given a RID, return the record
    RC GetRec     (const RID &rid, RM_Record &rec) const;

    RC InsertRec  (const char *pData, RID &rid);       // Insert a new record

    RC DeleteRec  (const RID &rid);                    // Delete a record
    RC UpdateRec  (const RM_Record &rec);              // Update a record

    // Forces a page (along with any contents stored in this class)
    // from the buffer pool to disk.  Default value forces all pages.
    RC ForcePages (PageNum pageNum = ALL_PAGES);
private:
    
   // IsValidPageNum will return TRUE if page number is valid and FALSE
   // otherwise
   int IsValidPageNum (PageNum pageNum) const;

   PF_FileHandle *pfHandle;                       // pointer to opened PF_FileHandle
   RM_FileHdr hdr;                                // file header
	 bool bFileOpen;                                // file open flag
   bool bHdrChanged;                              // dirty flag for file hdr
	 int recordSize;
};

//
// RM_FileScan: condition-based scan of records in the file
//
class RM_FileScan {
public:
    RM_FileScan  ();
    ~RM_FileScan ();

    RC OpenScan  (const RM_FileHandle &fileHandle,
                  AttrType   attrType,
                  int        attrLength,
                  int        attrOffset,
                  CompOp     compOp,
                  void       *value,
                  ClientHint pinHint = NO_HINT); // Initialize a file scan
    RC GetNextRec(RM_Record &rec);               // Get next matching record
    RC CloseScan ();                             // Close the scan
};

//
// RM_Manager: provides RM file management
//
class RM_Manager {
public:
    RM_Manager    (PF_Manager &pfm);
    ~RM_Manager   ();

    RC CreateFile (const char *fileName, int recordSize);
    RC DestroyFile(const char *fileName);
    RC OpenFile   (const char *fileName, RM_FileHandle &fileHandle);

    RC CloseFile  (RM_FileHandle &fileHandle);

private:
    PF_Manager&   pfm; // A reference to the external PF_Manager
};

//
// Print-error function
//
void RM_PrintError(RC rc);

#define RM_LASTWARN START_RM_WARN

#define RM_SIZETOOBIG      (START_RM_ERR - 0)  // record size too big
#define RM_PF              (START_RM_ERR - 1)  // error in PF
#define RM_NULLRECORD      (START_RM_ERR - 2)  
#define RM_RECSIZEMISMATCH (START_RM_ERR - 3)  // record size mismatch
#define RM_EOF             (START_RM_ERR - 4)  // end of file

#define RM_LASTERROR RM_EOF

#endif // RM_H
