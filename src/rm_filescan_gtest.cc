#include "rm_rid.h"
#include "rm.h"
#include "gtest/gtest.h"

#define STRLEN 29
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

class RM_FileScanTest : public ::testing::Test {
protected:
	RM_FileScanTest(): rmm(pfm)
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

TEST_F(RM_FileScanTest, Update) {
	RM_FileScan fs;
	RC rc;

  TestRec t;
  RID r;
	std::vector<RID> vec;
	int count = 400;
	for( int i = 0; i < count; i++)
	{
		t.num = i;
		fh.InsertRec((char*) &t, r);
		vec.push_back(r);
	}
	ASSERT_EQ(fh.GetNumPages(), 5);

	// check within same open
	for( int i = 0; i < count; i++)
	{
		RM_Record rec;
		fh.GetRec(vec[i], rec);
		TestRec * pBuf;
		rec.GetData((char*&)pBuf);
		// std::cerr << vec[i] << "pBuf->num " << pBuf->i << " i " << i << std::endl;
		ASSERT_EQ(pBuf->num, i);
	}

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count);

// update
  for( int i = 0; i < 10; i++)
	{
    int j = random() % count;
    RM_Record rec;
		fh.GetRec(vec[j], rec);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    vec[pBuf->num] = RID(-1,-1);
    pBuf->num = vec.size();
    fh.UpdateRec(rec);
    RID rid;
    rec.GetRid(rid);
    vec.push_back(rid);
  }

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    // cerr << pBuf->num << endl;
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count);
}


TEST_F(RM_FileScanTest, UpdateAll) {
	RM_FileScan fs;
	RC rc;

  TestRec t;
  RID r;
	std::vector<RID> vec;
	int count = 4000;
	for( int i = 0; i < count; i++)
	{
		t.num = i;
		fh.InsertRec((char*) &t, r);
		vec.push_back(r);
	}
	//ASSERT_EQ(fh.GetNumPages(), 5);

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count);

// update all
  for( int i = 0; i < count; i++)
	{
    RM_Record rec;
		fh.GetRec(vec[i], rec);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    vec[pBuf->num] = RID(-1,-1);
    pBuf->num = vec.size();
    fh.UpdateRec(rec);
    RID rid;
    rec.GetRid(rid);
    vec.push_back(rid);
  }

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(pBuf->num, count + numRecs);
    EXPECT_EQ(vec[pBuf->num], (rid));
    // cerr << pBuf->num << "\t" << rid << endl;
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count);

  int val = 7999;
  void * pval = &val;
  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  EQ_OP, pval, NO_HINT));
  ASSERT_EQ(rc, 0);

  numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    // cerr << pBuf->num << "\t" << rid << endl;
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, 1);

}

TEST_F(RM_FileScanTest, Delete) {
	RM_FileScan fs;
	RC rc;

  TestRec t;
  RID r;
	std::vector<RID> vec;
	int count = 400;
	for( int i = 0; i < count; i++)
	{
		t.num = i;
		fh.InsertRec((char*) &t, r);
		vec.push_back(r);
	}

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count);



// Delete
	for( int i = 0; i < 10; i++)
	{
    int j = random() % count;
    fh.DeleteRec(vec[j]);
    vec[j] = RID(-1,-1);
  }

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  numRecs = 0;
  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    numRecs++;
  }  

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count-10);

}

TEST_F(RM_FileScanTest, DeleteDuring) {
	RM_FileScan fs;
	RC rc;

  TestRec t;
  RID r;
	std::vector<RID> vec;
	int count = 400;
	for( int i = 0; i < count; i++)
	{
		t.num = i;
		fh.InsertRec((char*) &t, r);
		vec.push_back(r);
	}

  (rc=fs.OpenScan(fh,INT,sizeof(int),offsetof(TestRec, num),
                  NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  while(numRecs < count/2) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    // cerr << pBuf->num << "\t" << rid << endl;
    numRecs++;
  }

// Delete from second half
	for( int i = 0; i < 10; i++)
	{
    int j = random() % count/2;
    j += count/2;
    // cerr << "Deleting " << "\t" << j << endl;
    rc = fh.DeleteRec(vec[j]);
    EXPECT_EQ(0, rc);

    RM_Record rec;
    rc = fh.GetRec(vec[j], rec);
    EXPECT_NE(0, rc); // rec should no longer be returned.

    vec[j] = RID(-1,-1);
  }

  while(1) {
    RM_Record rec; 
    rc = fs.GetNextRec(rec);
    if(rc == RM_EOF)
      break;
    EXPECT_EQ(rc, 0);
    TestRec * pBuf;
		rec.GetData((char*&)pBuf);
    RID rid;
    rec.GetRid(rid);
    EXPECT_EQ(vec[pBuf->num], (rid));
    // cerr << pBuf->num << "\t" << rid << endl;

    numRecs++;
  }

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count-10);

}
