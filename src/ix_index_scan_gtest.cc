#include "ix_index_scan.h"
#include "ix_index_handle.h"
#include "btree_node.h"
#include "ix_index_manager.h"
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

