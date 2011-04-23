#include "rm_rid.h"
#include "rm.h"
#include "gtest/gtest.h"

#define STRLEN 29
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

class RM_FileHandleTest : public ::testing::Test {
protected:
	RM_FileHandleTest(): rmm(pfm)
		{}
	virtual void SetUp() 
	{
		RC rc;
		system("rm -f gtestfile");
		if(
			(rc = rmm.CreateFile("gtestfile", sizeof(TestRec))) 
			 || (rc =	rmm.OpenFile("gtestfile", fh))
			)
			RM_PrintError(rc);
	}

	virtual void TearDown() 
  {
		rmm.DestroyFile("gtestfile");
	}

  // Declares the variables your tests want to use.
	PF_Manager pfm;
	RM_Manager rmm;
	RM_FileHandle fh;
};

// Setup will call both constructor and Open()

TEST_F(RM_FileHandleTest, FullSize) {
	ASSERT_EQ(40UL ,sizeof(TestRec));
	ASSERT_EQ(40 ,fh.fullRecordSize());
	ASSERT_EQ(101, fh.GetNumSlots());
}

TEST_F(RM_FileHandleTest, ZeroRec) {
	RM_FileHandle sfh;
	RC rc;
 	system("rm -f gtestfilesmall");
  (rc = rmm.CreateFile("gtestfilesmall", 0));
  ASSERT_EQ(rc, RM_BADRECSIZE);

	(rc =	rmm.OpenFile("gtestfilesmall", sfh));
  ASSERT_LT(rc, 0);

	rc = rmm.CloseFile(sfh);
  ASSERT_LT(rc, 0);

	rc = rmm.DestroyFile("gtestfilesmall");
  ASSERT_LT(rc, 0);
}

TEST_F(RM_FileHandleTest, SmallRec) {
	struct SmallRec {
		int i;
	};
	RM_FileHandle sfh;
	RC rc;
 	system("rm -f gtestfilesmall");
	if(
		(rc = rmm.CreateFile("gtestfilesmall", sizeof(SmallRec))) 
		|| (rc =	rmm.OpenFile("gtestfilesmall", sfh))
		)
		RM_PrintError(rc);

	ASSERT_EQ(4UL, sizeof(SmallRec));
	ASSERT_EQ(4 ,sfh.fullRecordSize());
	ASSERT_EQ(988, sfh.GetNumSlots());

	rmm.CloseFile(sfh);
	rmm.DestroyFile("gtestfilesmall");
}


TEST_F(RM_FileHandleTest, Persist) {
	RC rc;
	RM_FileHandle fh2;

	system("rm -f gtestfile2");
	if(
		(rc = rmm.CreateFile("gtestfile2", sizeof(TestRec))) < 0
		)
	RM_PrintError(rc);
	ASSERT_EQ(RM_FNOTOPEN, rmm.CloseFile(fh2));

	if ((rc =	rmm.OpenFile("gtestfile2", fh2)) < 0
		|| (rc =	rmm.CloseFile(fh2)) < 0
		|| (rc =	rmm.OpenFile("gtestfile2", fh2)) < 0
		)
		RM_PrintError(rc);

	ASSERT_EQ(fh2.fullRecordSize(), 40);
	ASSERT_EQ(fh2.GetNumPages(), 1);
	
	rmm.CloseFile(fh2);
	rmm.DestroyFile("gtestfile2");
}


TEST_F(RM_FileHandleTest, WrongDelGet) {
	TestRec t;
	RID rid;
  RC rc;
  RM_Record rec;

	rc = fh.DeleteRec(RID(100,100));
  ASSERT_NE(0, rc);

	rc = fh.InsertRec((char*) &t, rid);
  ASSERT_EQ(0, rc);
  
	rc = fh.DeleteRec(rid);
  ASSERT_EQ(0, rc); // first ok

	rc = fh.DeleteRec(rid);
  ASSERT_NE(0, rc); // second should fail

  rec.Set((char*)&t, sizeof(t), rid);
	rc = fh.UpdateRec(rec);
  ASSERT_NE(0, rc); // update should also fail

	rc = fh.GetRec(rid, rec);
  ASSERT_NE(0, rc); // update should also fail

  rec.Set((char*)&t, sizeof(t), RID(1000, 1000));
	rc = fh.UpdateRec(rec);
  ASSERT_NE(0, rc); // should fail

	rc = fh.InsertRec((char*) &t, rid);
  ASSERT_EQ(0, rc);

  rec.Set((char*)&t, sizeof(t), rid);
	rc = fh.UpdateRec(rec);
  ASSERT_EQ(0, rc);

	rc = fh.UpdateRec(rec);
  ASSERT_EQ(0, rc); // 2nd should work too

}

TEST_F(RM_FileHandleTest, FreeList) {
	TestRec t;
	RID p1rid;
	RID p2rid;
	fh.InsertRec((char*) &t, p1rid);
	// std::cerr << "p1 RID was " << p1rid << std::endl;
  // std::cerr << "slots were " << fh.GetNumSlots() << std::endl;
	for( int i = 0; i < fh.GetNumSlots() + 5; i++)
	{
		t.num = i;
		RC rc = fh.InsertRec((char*) &t, p2rid);
    ASSERT_EQ(rc, 0);
		// std::cerr << "p2 RID was " << p2rid << std::endl;
	}
	// p1 should be full
	ASSERT_EQ(fh.GetNumPages(), 3);
	// std::cerr << "p2 RID was " << p2rid << std::endl;
	fh.DeleteRec(p1rid);
	RID newr;
	fh.InsertRec((char*) &t, newr);
	ASSERT_EQ(newr, p1rid);
	fh.InsertRec((char*) &t, newr);
	fh.InsertRec((char*) &t, newr);
}

TEST_F(RM_FileHandleTest, SmallRecIntegrity) {
	struct SmallRec {
		int i;
    int j;
    int k;
	};
	RM_FileHandle sfh;
	RC rc;
 	system("rm -f gtestfilesmall");
	if(
		(rc = rmm.CreateFile("gtestfilesmall", sizeof(SmallRec))) 
		|| (rc =	rmm.OpenFile("gtestfilesmall", sfh))
		)
		RM_PrintError(rc);

	ASSERT_EQ(sfh.GetNumPages(), 1);

	SmallRec t;
	RID r;
	std::vector<RID> vec;
	int count = 400;
	for( int i = 0; i < count; i++)
	{
		t.i = i;
		sfh.InsertRec((char*) &t, r);
		vec.push_back(r);
	}
	ASSERT_EQ(sfh.GetNumPages(), 3);

	// check within same open
	for( int i = 0; i < count; i++)
	{
		RM_Record rec;
		sfh.GetRec(vec[i], rec);
		SmallRec * pBuf;
		rec.GetData((char*&)pBuf);
		// std::cerr << vec[i] << "pBuf->i " << pBuf->i << " i " << i << std::endl;
		ASSERT_EQ(pBuf->i, i);
	}

	// check with new open
	rmm.CloseFile(sfh);
	RM_FileHandle sfh2;

	rc =	rmm.OpenFile("gtestfilesmall", sfh2);
	ASSERT_EQ(sfh2.GetNumPages(), 3);

	for( int i = 0; i < count; i++)
	{
		RM_Record rec;
		sfh2.GetRec(vec[i], rec);
		SmallRec * pBuf;
		rec.GetData((char*&)pBuf);
		// std::cerr << vec[i] << "pBuf->i " << pBuf->i << " i " << i << std::endl;
		ASSERT_EQ(pBuf->i, i);
	}
	rmm.CloseFile(sfh2);
	rmm.DestroyFile("gtestfilesmall");
}

TEST_F(RM_FileHandleTest, over40pages) {
  for( int page = 1; page < 60; page++) 
  {
    RID rid;
    for ( int record = 0; record < fh.GetNumSlots(); record++ )
    {
      TestRec t;
      
      RC rc = fh.InsertRec((char *)&t, rid);
      ASSERT_EQ(rc, 0);
    }
    ASSERT_EQ(rid, RID(page, fh.GetNumSlots()-1));
  }
}
