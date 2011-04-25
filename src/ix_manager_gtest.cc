#include "ix_manager.h"
#include "btree_node.h"
#include "gtest/gtest.h"

class IX_ManagerTest : public ::testing::Test {
};

TEST_F(IX_ManagerTest, Cons) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    IX_IndexHandle fh;
    
    const char * filename = "ixtfile";
    int indexNo = 0;
    system("rm -f ixtfile.0");
    if(
      (rc = ixm.CreateIndex(filename, indexNo, INT, sizeof(int))) 
      )
      IX_PrintError(rc);
    ASSERT_EQ(0, rc);

    ASSERT_EQ(0, system("ls ixtfile.0 > /dev/null"));

    rc = ixm.CreateIndex(filename, indexNo, INT, sizeof(int));
    ASSERT_EQ(IX_PF, rc);

    ASSERT_EQ(0, system("ls ixtfile.0 > /dev/null"));

    if(
      (rc =  ixm.OpenIndex(filename, indexNo, fh))
      )
      IX_PrintError(rc);
    ASSERT_EQ(0, rc);

    ASSERT_EQ(0, system("ls ixtfile.0 > /dev/null"));

    if(
      (rc =  ixm.CloseIndex(fh))
      )
      IX_PrintError(rc);
    ASSERT_EQ(0, rc);

    ASSERT_EQ(0, system("ls ixtfile.0 > /dev/null"));

    if(
      (rc =  ixm.OpenIndex(filename, indexNo, fh))
      )
      IX_PrintError(rc);
    ASSERT_EQ(0, rc);

    ASSERT_EQ(0, system("ls ixtfile.0 > /dev/null"));

    if(
      (rc =  ixm.CloseIndex(fh))
      )
      IX_PrintError(rc);
    ASSERT_EQ(0, rc);

    ASSERT_EQ(0, system("ls ixtfile.0 > /dev/null"));

    if(
      (rc =  ixm.DestroyIndex(filename, indexNo))
      )
      IX_PrintError(rc);
    ASSERT_EQ(0, rc);
    // non zero exit - file should be gone.
    ASSERT_NE(0, system("ls ixtfile.0 > /dev/null 2>/dev/null"));

}

TEST_F(IX_ManagerTest, ZeroRec) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    IX_IndexHandle fh;
    
    const char * filename = "ixtfile";
    int indexNo = 0;
    system("rm -f ixtfile.0");
    (rc = ixm.CreateIndex(filename, indexNo, INT, 0));
    ASSERT_NE(0, rc);

    ASSERT_NE(0, system("ls ixtfile.0 > /dev/null"));

    (rc =  ixm.OpenIndex(filename, indexNo, fh));
    ASSERT_NE(0, rc);

    (rc =  ixm.CloseIndex(fh));
    ASSERT_NE(0, rc);
}
