//
// sm_error.h
//


#ifndef SM_ERROR_H
#define SM_ERROR_H

#include "redbase.h"
//
// Print-error function
//
void SM_PrintError(RC rc);

#define SM_KEYNOTFOUND    (START_SM_WARN + 0)  // cannot find key
#define SM_INVALIDSIZE    (START_SM_WARN + 1)  // invalid entry size
#define SM_ENTRYEXISTS    (START_SM_WARN + 2)  // key,rid already
                                               // exists in index
#define SM_NOSUCHENTRY    (START_SM_WARN + 3)  // key,rid combination
                                               // does not exist in index

#define SM_LASTWARN SM_ENTRYEXISTS


#define SM_DBOPEN          (START_SM_ERR - 0)
#define SM_NOSUCHDB        (START_SM_ERR - 1)
#define SM_NOTOPEN         (START_SM_ERR - 2)  
#define SM_NOSUCHTABLE     (START_SM_ERR - 3)
#define SM_BADOPEN         (START_SM_ERR - 4)
#define SM_FNOTOPEN        (START_SM_ERR - 5)
#define SM_BADATTR         (START_SM_ERR - 6)
#define SM_BADTABLE        (START_SM_ERR - 7)
#define SM_INDEXEXISTS     (START_SM_ERR - 8)

#define SM_LASTERROR SM_INDEXEXISTS

#endif // SM_ERROR_H
