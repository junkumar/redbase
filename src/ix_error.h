//
// ix_error.h
//


#ifndef IX_ERROR_H
#define IX_ERROR_H

#include "redbase.h"
//
// Print-error function
//
void IX_PrintError(RC rc);

#define IX_LASTWARN START_IX_WARN

#define IX_SIZETOOBIG      (START_IX_ERR - 0)  // key size too big
#define IX_PF              (START_IX_ERR - 1)  // error in PF
#define IX_BADIXPAGE       (START_IX_ERR - 2)  
#define IX_RECSIZEMISMATCH (START_IX_ERR - 3)  // record size mismatch
#define IX_HANDLEOPEN      (START_IX_ERR - 4)
#define IX_BADOPEN         (START_IX_ERR - 5)
#define IX_FNOTOPEN        (START_IX_ERR - 6)
#define IX_BAD_RID         (START_IX_ERR - 7)
#define IX_EOF             (START_IX_ERR - 8)  // end of file

#define IX_LASTERROR IX_EOF

#endif // IX_ERROR_H
