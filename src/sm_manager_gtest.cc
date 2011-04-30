#include "sm.h"
#include "catalog.h"
#include "gtest/gtest.h"
#include <sstream>

class SM_ManagerTest : public ::testing::Test {
};

TEST_F(SM_ManagerTest, Cons) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
    SM_Manager smm(ixm, rmm);

    {    
      DataRelInfo relcat_rel;
      strcpy(relcat_rel.relName, "relcat");
      relcat_rel.attrCount = DataRelInfo::members();
      relcat_rel.recordSize = DataRelInfo::size();
      relcat_rel.numPages = 1; // initially
      relcat_rel.numRecords = 2; // relcat & attrcat
      
      char buf[2000];
      memset(buf, 0 , 2000);
      
      memcpy(buf, &relcat_rel, DataRelInfo::size());
      
      DataRelInfo * prel = (DataRelInfo*) buf;
      ASSERT_EQ(2, prel->numRecords);
      ASSERT_EQ(1, prel->numPages);
    }

    const char * dbname = "smtfile";
    stringstream comm;
    comm << "rm -r " << dbname;
    rc = system (comm.str().c_str());

    stringstream command;
    command << "./dbcreate " << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    rc = smm.Print("relcat");
    ASSERT_EQ(rc, 0);
    
    rc = smm.Print("attrcat");
    ASSERT_EQ(rc, 0);

    rc = smm.Help();
    ASSERT_EQ(rc, 0);

    rc = smm.Help("attrcat");
    ASSERT_EQ(rc, 0);

    rc = smm.Help("relcat");
    ASSERT_EQ(rc, 0);

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
