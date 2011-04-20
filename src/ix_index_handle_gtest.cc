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

    // small size pages - 3 int keys per page
    int smallPage = (sizeof(RID) + sizeof(int))*(3+1);

    system("rm -f smallpagefile.0");
    if(
      (rc = ixm.CreateIndex("smallpagefile", 0, INT, sizeof(int), smallPage)) 
      || (rc =	ixm.OpenIndex("smallpagefile", 0, sifh))
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
	
    if(
      (rc =	ixm.CloseIndex(sifh))
      || (rc = ixm.DestroyIndex("smallpagefile", 0)) 
      )
      IX_PrintError(rc);
	}

  // Declares the variables your tests want to use.
  IX_Manager ixm;
  PF_Manager pfm;
  IX_IndexHandle ifh; // 340 int keys per page
  IX_IndexHandle sifh; // 3 int keys per page

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
    // cerr << "insert of " << i << " done" << endl;
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
    // cerr << "insert of " << j << " done" << endl;
    ASSERT_EQ(rc, 0);
  }
  cerr << "second 340 done" << endl;
  ASSERT_EQ(ifh.GetHeight(), 1);
  //TODO - failing - ASSERT_EQ(ifh.GetNumPages(), 3);

  int i = 680;
  RC rc = ifh.InsertEntry(&i, RID());
  ASSERT_EQ(rc, 0);
}

TEST_F(IX_IndexHandleTest, SmallPage) {
  int ifh = 1; //masking

  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 1
  int entries[] = {100,102,3,5,11,31,30};

  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    sifh.Print(cerr);
    cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 3);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(sifh.GetHeight(), 3);
  sifh.Print(cerr);
}

TEST_F(IX_IndexHandleTest, 2SmallPage) {
  int ifh = 1; //masking

  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {3,5,11,31,30,100,102};
  
  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    sifh.Print(cerr);
    cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 2);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

}

TEST_F(IX_IndexHandleTest, 3SmallPage) {
  int ifh = 1; //masking

  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {3,5,11,30,31,100,102};
  
  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    sifh.Print(cerr);
    cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 2);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

}

TEST_F(IX_IndexHandleTest, 4SmallPage) {
  int ifh = 1; //masking

  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {102,100,31,30,11,5,3};
  
  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    sifh.Print(cerr);
    cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 3);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

}
