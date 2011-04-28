#include "btree_node.h"
#include "ix_manager.h"
#include "ix_indexhandle.h"
#include "gtest/gtest.h"

#define STRLEN 29
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

class BtreeNodeTest : public ::testing::Test {
protected:
  BtreeNodeTest(): ixm(pfm) {}
	virtual void SetUp() 
	{
    RC rc;
		system("rm -f gtestfile.0");
		if(
			(rc = ixm.CreateIndex("gtestfile", 0, INT, sizeof(int))) 
      || (rc =	ixm.OpenIndex("gtestfile", 0, ifh))
			)
			IX_PrintError(rc);

    ifh.pfHandle->GetThisPage(0, ph);
    // Needs to be called everytime GetThisPage is called.
    ifh.pfHandle->UnpinPage(0);

		system("rm -f gtestfile2.0");
		if(
			(rc = ixm.CreateIndex("gtestfile2", 0, INT, sizeof(int))) 
      || (rc =	ixm.OpenIndex("gtestfile2", 0, ifh2))
			)
			IX_PrintError(rc);

    rc = ifh2.pfHandle->GetThisPage(0, ph2);
    if (rc != 0) PF_PrintError(rc);

    // Needs to be called everytime GetThisPage is called.
    rc = ifh2.pfHandle->UnpinPage(0);
    if (rc != 0) PF_PrintError(rc);

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
      (rc =	ixm.CloseIndex(ifh2))
			|| (rc = ixm.DestroyIndex("gtestfile2", 0)) 
			)
			IX_PrintError(rc);
	}

  // Declares the variables your tests want to use.
  IX_Manager ixm;
  PF_Manager pfm;
  PF_PageHandle ph;
  PF_PageHandle ph2;
  IX_IndexHandle ifh;
  IX_IndexHandle ifh2;
};

// Setup will call both constructor and Open()

TEST_F(BtreeNodeTest, Cons) {
  BtreeNode b(INT, sizeof(int), ph);
  // 4092 / 12 = exactly 341
  // so 340 keys = 340 * 4
  // 340 RIDs = 340 * 8
  // total 4080 - 12 wasted bytes for numKeys/left/right
  ASSERT_EQ(340, b.GetMaxKeys());

  BtreeNode b2(FLOAT, sizeof(float), ph);
  // 4092 / 12 = exactly 341
  // so 340 keys = 340 * 4
  // 341 RIDs = 341 * 8
  // total 4088 - 4 wasted bytes
  ASSERT_EQ(340, b2.GetMaxKeys());

  BtreeNode b3(STRING, sizeof(char[10]), ph);
  // 4086 / 18 = exactly 227
  // so 227 keys = 227 * 10 = 2270
  // 228 RIDs = 227 * 8 + 8
  // total 4094 - 2 xtra bytes
  // so 226 keys
  ASSERT_EQ(226, b3.GetMaxKeys());  
}

TEST_F(BtreeNodeTest, GetSetKey) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i < 9; i++) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());
  for (int i = 9; i < 5; i--) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  BtreeNode b3(STRING, sizeof(char[10]), ph);
  for (int i = 0; i < 9; i++) {
    char buf[10];
    // shorter than 10 + with \0 for ASSERT_STREQ
    sprintf(buf, "yoyo %d", i);
    b3.Insert(buf, RID());
    const char * pbuf = NULL;
    b3.GetKey(i, (void*&)pbuf);
    ASSERT_STREQ(buf, pbuf);
  }
  ASSERT_TRUE(b3.isSorted());
  char buf[10];
  // will be somewhere in the middle
  sprintf(buf, "yoyo 2222");
  b3.Insert(buf, RID());
  ASSERT_TRUE(b3.isSorted());  

}

TEST_F(BtreeNodeTest, Insert) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i < 340; i++) {
    ASSERT_EQ(0, b.Insert(&i, RID()));
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());
  // full page
  int i = 341;
  ASSERT_EQ(-1, b.Insert(&i, RID()));
}

TEST_F(BtreeNodeTest, Find) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i < 10; i++) {
    ASSERT_EQ(0, b.Insert(&i, RID()));
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  int i = 10;
  const void * pi = &i;
  ASSERT_EQ(10, b.FindKeyPosition(pi));
  i = 6;
  ASSERT_EQ(6, b.FindKeyPosition(pi));
  i = -3;
  ASSERT_EQ(0, b.FindKeyPosition(pi));
  i = 6;
  ASSERT_EQ(0, b.Remove(pi));
  ASSERT_EQ(6, b.FindKeyPosition(pi));

  //exact FindKey
  i = -1;
  ASSERT_EQ(0, b.Insert(pi, RID(100,100)));
  // should be lowest key
  ASSERT_EQ(0, b.FindKey(pi)); // no rid, key only

  ASSERT_EQ(0, b.FindKey(pi, RID(100,100))); // right rid

  ASSERT_EQ(-1, b.FindKey(pi, RID(2,2))); // wrong rid


}

TEST_F(BtreeNodeTest, Remove) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i <= 9; i++) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  // non existent key
  int val = 1000; int *pval = &val;
  ASSERT_EQ(-2, b.Remove(pval));

  // smallest key
  val = 0;
  ASSERT_EQ(0, b.Remove(pval));
  ASSERT_TRUE(b.isSorted());
  ASSERT_EQ(9, b.GetNumKeys());

  // largest key
  val = 9;
  ASSERT_EQ(0, b.Remove(pval));
  ASSERT_TRUE(b.isSorted());
  ASSERT_EQ(8, b.GetNumKeys());


  // middle key
  val = 5;
  ASSERT_EQ(0, b.Remove(pval));
  ASSERT_TRUE(b.isSorted());
  ASSERT_EQ(7, b.GetNumKeys());

}


TEST_F(BtreeNodeTest, ReCons) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i <= 9; i++) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  // read back page as btree node and check values
  BtreeNode bsame(INT, sizeof(int), ph, false);
  ASSERT_EQ(10, bsame.GetNumKeys());
  for (int i = 0; i <= 9; i++) {
    const int * pi = NULL;
    bsame.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(bsame.isSorted());
}

TEST_F(BtreeNodeTest, Sort) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i <= 9; i++) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  // force bad key in the middle
  int val = 25;
  b.SetKey(3, &val);
  ASSERT_FALSE(b.isSorted());
}

TEST_F(BtreeNodeTest, SetLeft) {
  BtreeNode b(INT, sizeof(int), ph);
//  b.SetNumKeys(4);

  for (int i = 0; i <= 9; i++) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  b.SetLeft(4);
  ASSERT_EQ(b.GetLeft(), 4);

  b.SetRight(4);
  ASSERT_EQ(b.GetRight(), 4);

  ASSERT_EQ(b.GetNumKeys(), 10);

  for (int i = 0; i <= 340; i++) {
    int res = b.Insert(&i, RID());
    // if(res != 0)
    //   cerr << i << endl;
    ASSERT_GE(res, -1);
  }


  ASSERT_EQ(b.GetLeft(), 4);
  ASSERT_EQ(b.GetRight(), 4);
  ASSERT_EQ(b.GetNumKeys(), 340);

}

TEST_F(BtreeNodeTest, Split) {
  BtreeNode b(INT, sizeof(int), ph);
  for (int i = 0; i <= 9; i++) {
    b.Insert(&i, RID());
    const int * pi = NULL;
    b.GetKey(i, (void*&)pi);
    ASSERT_EQ(i, *pi);
  }
  ASSERT_TRUE(b.isSorted());

  BtreeNode r(INT, sizeof(int), ph2);
  ASSERT_EQ(r.IsValid(), 0);
  

  ASSERT_EQ(10, b.GetNumKeys());
  ASSERT_EQ(0, r.GetNumKeys());
  
  b.Split(&r);

  ASSERT_EQ(5, b.GetNumKeys());
  ASSERT_EQ(5, r.GetNumKeys());
  
  for (int i = 0; i < 5; i++) {
    const int * pi = NULL;
    r.GetKey(i, (void*&)pi);
    ASSERT_EQ(i+5, *pi);
  }
  ASSERT_TRUE(r.isSorted());

  for (int i = 21; i <= 29; i++) {
    b.Insert(&i, RID());
  }

  ASSERT_EQ(14, b.GetNumKeys());
  ASSERT_EQ(5, r.GetNumKeys());

  r.Merge(&b);

  ASSERT_EQ(0, b.GetNumKeys());
  ASSERT_EQ(19, r.GetNumKeys());
  ASSERT_TRUE(r.isSorted());


}
