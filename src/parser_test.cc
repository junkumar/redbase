//
// File:        parser_test.cc
// Description: Test the parser
// Authors:     Dallan Quass (quass@cs.stanford.edu)
//              Jason McHugh (mchughj@cs.stanford.edu)
//
// 1997: Changes for the statistics manager
//

#include <cstdio>
#include <iostream>
#include "redbase.h"
#include "parser.h"
#include "sm.h"
#include "ql.h"


//
// Global PF_Manager and RM_Manager variables
//
PF_Manager pfm;
RM_Manager rmm(pfm);
IX_Manager ixm(pfm);
SM_Manager smm(ixm, rmm);
QL_Manager qlm(smm, ixm, rmm);

int main(void)
{
    RC rc;

    if ((rc = smm.OpenDb("testdb"))) {
        PrintError(rc);
        return (1);
    }

    RBparse(pfm, smm, qlm);

    if ((rc = smm.CloseDb())) {
        PrintError(rc);
        return (1);
    }
}
