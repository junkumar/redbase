//
// File:        rm_record.cc
//

#include <cerrno>
#include <cstdio>
#include <cstring>
#include <iostream>
#include "rm.h"
#include "rm_rid.h"

using namespace std;

// The RM_Record class defines record objects. To materialize a record, a client
// creates an instance of this class and passes it to one of the RM_FileHandle
// or RM_FileScan methods that reads a record (described above). RM_Record
// objects should contain copies of records from the buffer pool, not records in
// the buffer pool itself.


// Terrible interface that breaks all C++ RAII rules
// Constructor allows the creation of null references that can be populated by
// later functions.
RM_Record::RM_Record() 
:recordSize(0), data(NULL)
{
}

RM_Record::~RM_Record()
{
	if (data != NULL) {
		delete [] data;
	}
}

// int RM_Record::getRecordSize() const
// {
// 	return recordSize;
// }

RC RM_Record::SetData     (char *pData, int size)
{
	if(recordSize != 0 && (size != recordSize - sizeof(RID)))
		return RM_RECSIZEMISMATCH;
	recordSize = size + sizeof(RID);
	if (data == NULL)
		data = new char[recordSize];
  strncpy(data, pData, size);
	RID r(20, 20); //TODO
	RID *pr = &r;
	strncpy(data + size, (char*) pr, sizeof(RID));
}

RC RM_Record::GetData     (char *&pData) const 
{
	if (data != NULL)
		pData = data;
	else 
		return RM_NULLRECORD;
}

RC RM_Record::GetRid     (RID &rid) const 
{
	if(data != NULL) 
	{
		char * pr = (char *) &rid;
		strncpy(pr, 
						data + recordSize - sizeof(RID),
						sizeof(RID));
	} 
	else 
		return RM_NULLRECORD;
}
