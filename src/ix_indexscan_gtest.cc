#include "ix_indexscan.h"
#include "ix_indexhandle.h"
#include "btree_node.h"
#include "ix_manager.h"
#include "gtest/gtest.h"
#include "rm_error.h"


class IX_IndexScanTest : public ::testing::Test {
protected:
  IX_IndexScanTest(): ixm(pfm) {}
	virtual void SetUp() 
	{
    RC rc;
		system("rm -f gtestfile.0");
		if(
			(rc = ixm.CreateIndex("gtestfile", 0, INT, sizeof(int))) 
      || (rc =	ixm.OpenIndex("gtestfile", 0, ifh))
//      || (rc =	is.OpenScan(ifh, NO_OP, NULL))
			)
			IX_PrintError(rc);

    // small size pages - 3 int keys per page
    int smallPage = (sizeof(RID) + sizeof(int))*(3+1);


    system("rm -f smallpagefile.0");
    if(
      (rc = ixm.CreateIndex("smallpagefile", 0, INT, sizeof(int), smallPage)) 
      || (rc =	ixm.OpenIndex("smallpagefile", 0, sifh))
//      || (rc =	sis.OpenScan(sifh, NO_OP, NULL))
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

// void ScanOrderedInt(IX_IndexHandle& fh, int numEntries) {
//   IX_IndexScan s;
//   RC rc =	s.OpenScan(fh, NO_OP, NULL);
//   ASSERT_EQ(rc, 0);

//   RID r;
//   int prev = -100000;
//   void * k;
//   int count = 0;
//   while((s.GetNextEntry(k, r)) != IX_EOF) {
//     int curr = (*(int*)k);
//     ASSERT_LE(prev, curr);
//     cerr << "IX Scan entry - " << curr << endl; 
//     prev = curr;
//     count++;
//   }
//   ASSERT_EQ(numEntries, count);

//   rc =	s.CloseScan();
//   ASSERT_EQ(rc, 0);
// }

TEST_F(IX_IndexScanTest, DeleteDuring) {
	IX_IndexScan fs;
	RC rc;

  RID r;
	std::vector<RID> vec;
	int count = 400;
	for( int i = 0; i < count; i++)
	{
    r = RID(i,i);
		ifh.InsertEntry((char*) &i, r);
		vec.push_back(r);
	}

  (rc=fs.OpenScan(ifh, NO_OP, NULL, NO_HINT));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  while(numRecs < count/2) {
    RID rid; 
    rc = fs.GetNextEntry(rid);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(vec[numRecs], rid);
    // cerr << pBuf->num << "\t" << rid << endl;
    numRecs++;
  }

// Delete from second half
	for( int i = 0; i < 10; i++)
	{
    int j = random() % count/2;
    j += count/2;
    // cerr << "Deleting " << "\t" << j << endl;
    int key = vec[j].Page();
    rc = ifh.DeleteEntry(&key, vec[j]);
    EXPECT_EQ(0, rc);

    RID result;
    rc = ifh.Search(&key, result);
    EXPECT_NE(0, rc); // entry should no longer be returned.

    vec[j] = RID(-1,-1);
  }

  while(1) {
    RID result; 
    rc = fs.GetNextEntry(result);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    EXPECT_EQ(vec[result.Page()], result);
    // cerr << pBuf->num << "\t" << rid << endl;

    numRecs++;
  }

  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, count-10);

}

TEST_F(IX_IndexScanTest, OpOptimizeAsc) {
	IX_IndexScan fs;
	RC rc;

  RID r;
	int count = 400;
  int numScanned = 0;
  void * null = NULL;

	for( int i = 1; i <= count; i++)
	{
    r = RID(i,i);
		ifh.InsertEntry((char*) &i, r);
    r = RID(i,2*i);
		ifh.InsertEntry((char*) &i, r);
	}

  // cerr << "per btree node " << ifh.GetRoot()->GetMaxKeys() << endl;

  // cerr << "EQ_OP non-existent " << endl;

  int val = count + 20;
  (rc=fs.OpenScan(ifh, EQ_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  numScanned = 0;
  while(1) {
    RID rid;
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, 0);
  ASSERT_EQ(numScanned, 0);

  // cerr << "EQ_OP " << endl;

  val = 20;
  (rc=fs.OpenScan(ifh, EQ_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  EXPECT_EQ(numRecs, 2);
  // EXPECT_EQ(numScanned, (val-0)*2);
  EXPECT_LT(numScanned, (count)*2);
  //cerr << "numScanned " << numScanned << endl;

  // cerr << "LT_OP " << endl;

  val = 60;
  (rc=fs.OpenScan(ifh, LT_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (val-1)*2);
  // EXPECT_EQ(numScanned, (val-0)*2);
  EXPECT_LT(numScanned, (count)*2);


  val = 60;
  (rc=fs.OpenScan(ifh, LE_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (val)*2);
  // EXPECT_EQ(numScanned, (val-0)*2);
  EXPECT_LT(numScanned, (count)*2);


  val = 118;
  (rc=fs.OpenScan(ifh, GT_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (count-val)*2);
//  EXPECT_EQ(numScanned, (count-val)*2);
  EXPECT_LT(numScanned, (count)*2);



  val = 178;
  (rc=fs.OpenScan(ifh, GE_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (count-val+1)*2);
  EXPECT_EQ(numScanned, (count)*2);


  val = 118;
  (rc=fs.OpenScan(ifh, NE_OP, &val, NO_HINT));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (count-1)*2);
  EXPECT_EQ(numScanned, (count)*2);
}

TEST_F(IX_IndexScanTest, OpOptimizeDesc) {
	IX_IndexScan fs;
	RC rc;

  RID r;
	int count = 400;
  int numScanned = 0;
  void * null = NULL;

	for( int i = 1; i <= count; i++)
	{
    r = RID(i,i);
		ifh.InsertEntry((char*) &i, r);
    r = RID(i,2*i);
		ifh.InsertEntry((char*) &i, r);
	}

  // cerr << "per btree node " << ifh.GetRoot()->GetMaxKeys() << endl;

  // cerr << "EQ_OP non-existent " << endl;

  int val = count + 20;
  (rc=fs.OpenScan(ifh, EQ_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);

  int numRecs = 0;
  numScanned = 0;
  while(1) {
    RID rid;
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, 0);
  ASSERT_EQ(numScanned, 0);

  // cerr << "EQ_OP " << endl;

  val = 20;
  (rc=fs.OpenScan(ifh, EQ_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  EXPECT_EQ(numRecs, 2);
  EXPECT_LT(numScanned, (val-0)*2);


  val = 260;
  (rc=fs.OpenScan(ifh, LT_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);
  // cerr << "LT_OP " << endl;

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (val-1)*2);
  EXPECT_EQ(numScanned, (val-0)*2);

  // cerr << "LE_OP " << endl;

  val = 360;
  (rc=fs.OpenScan(ifh, LE_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (val)*2);
  EXPECT_EQ(numScanned, (val-0)*2);

  // cerr << "GT_OP " << endl;

  val = 118;
  (rc=fs.OpenScan(ifh, GT_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (count-val)*2);
  // EXPECT_EQ(numScanned, (count-val)*2);
  EXPECT_LT(numScanned, (count)*2);

  // cerr << "GE_OP " << endl;

  val = 118;
  (rc=fs.OpenScan(ifh, GE_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (count-val+1)*2);
  EXPECT_LT(numScanned, (count)*2);

  // cerr << "NE_OP " << endl;

  val = 118;
  (rc=fs.OpenScan(ifh, NE_OP, &val, NO_HINT, true));
  ASSERT_EQ(rc, 0);

  numScanned = 0;
  numRecs = 0;
  while(1) {
    RID rid; 
    rc = fs.GetNextEntry(null, rid, numScanned);
    if(rc == IX_EOF)
      break;
    EXPECT_EQ(rc, 0);
    // cerr << rid << endl;
    numRecs++;
  }
  (rc=fs.CloseScan());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(numRecs, (count-1)*2);
  EXPECT_EQ(numScanned, (count)*2);
}

// // Needs contest db to exist.
// TEST_F(IX_IndexScanTest, Customer) {
// 	IX_IndexScan fs;
// 	RC rc;
  
//   IX_IndexHandle cfh;
//   rc =	ixm.OpenIndex("contest/CUSTOMER", 73, cfh);
//   ASSERT_EQ(rc, 0);


//   (rc=fs.OpenScan(cfh, NO_OP, NULL, NO_HINT));
//   ASSERT_EQ(rc, 0);

//   int numRecs = 0;
//   while(1) {
//     RID rid; 
//     rc = fs.GetNextEntry(rid);
//     if(rc == IX_EOF)
//       break;
//     EXPECT_EQ(rc, 0);
//     // cerr << pBuf->num << "\t" << rid << endl;
//     numRecs++;
//   }

//   (rc=fs.CloseScan());
//   ASSERT_EQ(rc, 0);
//   EXPECT_EQ(numRecs, 1500);

// }
