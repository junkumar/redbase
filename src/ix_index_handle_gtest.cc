#include "ix_index_handle.h"
#include "btree_node.h"
#include "ix_index_manager.h"
#include "gtest/gtest.h"

#define STRLEN 29
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

class IX_IndexHandleTest : public ::testing::Test {
protected:
  IX_IndexHandleTest(): ixm(pfm) {}
	virtual void SetUp() 
	{
    RC rc;
		system("rm -f gtestfile.0");
		if(
			(rc = ixm.CreateIndex("gtestfile", 0, INT, sizeof(int))) 
      || (rc =	ixm.OpenIndex("gtestfile", 0, ifh))
			)
			IX_PrintError(rc);

	}

	virtual void TearDown() 
  {
    RC rc;
		if(
      (rc =	ixm.CloseIndex(ifh))
			|| (rc = ixm.DestroyIndex("gtestfile", 0)) 
			)
			IX_PrintError(rc);
	}

  // Declares the variables your tests want to use.
  IX_Manager ixm;
  PF_Manager pfm;
  IX_IndexHandle ifh;
};


TEST_F(IX_IndexHandleTest, RootInsert) {
  for (int i = 0; i < 9; i++) {
    RID r(i, i);
    RC rc;
    rc = ifh.InsertEntry(&i, r);
    ASSERT_EQ(rc, 0);

    RID s;
    rc = ifh.Search(&i, s);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(r, s);
  }

  for (int i = 0; i < 9; i++) {
    RID r(i, i);
    RID s;
    RC rc = ifh.Search(&i, s);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(r, s);
  }

  RC rc = ixm.CloseIndex(ifh);
  ASSERT_EQ(rc, 0);

  rc = ixm.OpenIndex("gtestfile", 0, ifh);
  ASSERT_EQ(rc, 0);

  for (int i = 0; i < 9; i++) {
    RID r(i, i);
    RID s;
    rc = ifh.Search(&i, s);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(r, s);
  }
}

TEST_F(IX_IndexHandleTest, RootOverflow) {
  for (int i = 0; i < 340; i++) {
    RID r(i, i);
    RC rc;
    rc = ifh.InsertEntry(&i, r);
    cerr << "insert of " << i << " done" << endl;
    ASSERT_EQ(rc, 0);

    RID s;
    rc = ifh.Search(&i, s);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(r, s);
  }
  cerr << "first 340 done" << endl;
  ASSERT_EQ(ifh.GetHeight(), 1);
  ASSERT_EQ(ifh.GetNumPages(), 2);


  for (int i = 0; i < 340; i++) {
    int j = i + 340;
    RID r(i, i);
    RC rc;
    rc = ifh.InsertEntry(&j, r);
    cerr << "insert of " << j << " done" << endl;
    ASSERT_EQ(rc, 0);
  }
  cerr << "second 340 done" << endl;
  ASSERT_EQ(ifh.GetHeight(), 1);
  ASSERT_EQ(ifh.GetNumPages(), 3);

  int i = 680;
  RC rc = ifh.InsertEntry(&i, RID());
  ASSERT_EQ(rc, 0);

}
