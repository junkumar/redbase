#include "ix_indexscan.h"
#include "ix_indexhandle.h"
#include "btree_node.h"
#include "ix_manager.h"
#include "gtest/gtest.h"
#include "rm_error.h"

class IX_IndexHandleTest : public ::testing::Test {
protected:
  IX_IndexHandleTest(): ixm(pfm) {}
  virtual void SetUp() 
  {
    RC rc;
    system("rm -f gtestfile.0");
    if(
      (rc = ixm.CreateIndex("gtestfile", 0, INT, sizeof(int))) 
      || (rc =  ixm.OpenIndex("gtestfile", 0, ifh))
//      || (rc =  is.OpenScan(ifh, NO_OP, NULL))
      )
      IX_PrintError(rc);

    // small size pages - 3 int keys per page
    int smallPage = (sizeof(RID) + sizeof(int))*(3+1);


    system("rm -f smallpagefile.0");
    if(
      (rc = ixm.CreateIndex("smallpagefile", 0, INT, sizeof(int), smallPage)) 
      || (rc =  ixm.OpenIndex("smallpagefile", 0, sifh))
//      || (rc =  sis.OpenScan(sifh, NO_OP, NULL))
      )
      IX_PrintError(rc);
  }

  virtual void TearDown() 
  {
    RC rc;
    if(
      (rc =  ixm.CloseIndex(ifh))
      || (rc = ixm.DestroyIndex("gtestfile", 0)) 
      )
      IX_PrintError(rc);
  
    if(
      (rc =  ixm.CloseIndex(sifh))
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

// test if a leaf scan yields a sorted list of items with expected
// count
extern void ScanOrderedInt(IX_IndexHandle& fh, int numEntries) {
  IX_IndexScan s;
  RC rc =  s.OpenScan(fh, NO_OP, NULL);
  ASSERT_EQ(rc, 0);

  RID r;
  int prev = -100000;
  void * k;
  int count = 0;
  int ns = 0;
  while((s.GetNextEntry(k, r, ns)) != IX_EOF) {
    int curr = (*(int*)k);
    ASSERT_LE(prev, curr);
    prev = curr;
    count++;
    // cerr << "IX Scan entry - " << curr 
    //      << " Count " << count << endl; 
  }
  ASSERT_EQ(numEntries, count);

  rc =  s.CloseScan();
  ASSERT_EQ(rc, 0);

  // Desc
  rc =  s.OpenScan(fh, NO_OP, NULL, NO_HINT, true);
  ASSERT_EQ(rc, 0);

  prev = 999999999;
  count = 0;
  while((s.GetNextEntry(k, r, ns)) != IX_EOF) {
    int curr = (*(int*)k);
    ASSERT_GE(prev, curr);
    prev = curr;
    count++;
    // cerr << "IX Scan entry - " << curr 
    //      << " Count " << count << endl; 
  }
  ASSERT_EQ(numEntries, count);

  rc =  s.CloseScan();
  ASSERT_EQ(rc, 0);
}


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

  ScanOrderedInt(ifh, 9);

  for (int i = 0; i < 9; i++) {
    RID r(i, i);
    RID s;
    RC rc = ifh.Search(&i, s);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(r, s);
  }

  ScanOrderedInt(ifh, 9);

  ASSERT_EQ(ifh.GetRoot()->GetNumKeys(), 9);
  ifh.GetRoot()->SetLeft(4);
  ASSERT_EQ(ifh.GetRoot()->GetLeft(), 4);

  RC rc = ixm.CloseIndex(ifh);
  ASSERT_EQ(rc, 0);

  rc = ixm.OpenIndex("gtestfile", 0, ifh);
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(ifh.GetRoot()->GetLeft(), 4);
  ASSERT_EQ(ifh.GetRoot()->GetNumKeys(), 9);

  for (int i = 0; i < 9; i++) {
    RID r(i, i);
    RID s;
    rc = ifh.Search(&i, s);
    if(rc != 0)
      PrintErrorAll(rc);
    ASSERT_EQ(rc, 0);
    ASSERT_EQ(r, s);
  }
  ScanOrderedInt(ifh, 9);
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
    // cerr << "pages " << ifh.GetNumPages() << endl;
  }
  ASSERT_EQ(ifh.GetHeight(), 1);
  ASSERT_EQ(ifh.GetNumPages(), 2);

  ScanOrderedInt(ifh, 340);

  for (int i = 0; i < 340; i++) {
    int j = i + 340;
    RID r(i, i);
    RC rc;
    rc = ifh.InsertEntry(&j, r);
    ASSERT_EQ(rc, 0);
  }
  ASSERT_EQ(ifh.GetHeight(), 2);
  // first split at 340 - 3 pages
  // second split at 340 + 340/2  - 510 - 4 pages
  // thir split at 510 + 340/2 - 680 - 5 pages
  //ASSERT_EQ(ifh.GetNumPages(), 4);

  // ifh.Print(cerr);
  ScanOrderedInt(ifh, 680);


  int i = 680;
  RC rc = ifh.InsertEntry(&i, RID());
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(ifh.GetNumPages(), 6);

  ScanOrderedInt(ifh, 681);

}

TEST_F(IX_IndexHandleTest, SmallPage) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 1
  int entries[] = {100,102,3,5,11,31,30};

  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    // sifh.Print(cerr);
    ScanOrderedInt(sifh, i+1);
    // cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 3);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  ASSERT_EQ(sifh.GetHeight(), 3);
  // sifh.Print(cerr);

  ScanOrderedInt(sifh, 8);
}

TEST_F(IX_IndexHandleTest, SmallPageStr) {
  system("rm -f smallpagefile.0");
  // small size pages - 3 int keys per page
  int smallPage = (sizeof(RID) + sizeof(int))*(3+1);

  RC rc;
  IX_IndexHandle ssifh;
  if(
      (rc = ixm.CreateIndex("smallpagefile", 0, STRING, 4, smallPage)) 
      || (rc =  ixm.OpenIndex("smallpagefile", 0, ssifh))
      )
      IX_PrintError(rc);


  const BtreeNode * root = ssifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
  
  char entries[5];

  for (int i = 0; i < 7; i++) {
    RID r(0, i);
    RC rc;
    sprintf(entries, "%dcrc", i); 
    rc = ssifh.InsertEntry(entries, r);
    ASSERT_EQ(rc, 0);
    ssifh.Print(cerr);
    // cerr << endl << endl;
  }
  ASSERT_EQ(ssifh.GetHeight(), 2);
  
}


TEST_F(IX_IndexHandleTest, 2SmallPage) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {3,5,11,31,30,100,102};
  
  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    // sifh.Print(cerr);
    // cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 2);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  // sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

  ScanOrderedInt(sifh, 8);
}

TEST_F(IX_IndexHandleTest, 3SmallPage) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {3,5,11,30,31,100,102};
  
  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    // sifh.Print(cerr);
    // cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 2);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  // sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

  ScanOrderedInt(sifh, 8);
}

TEST_F(IX_IndexHandleTest, 4SmallPage) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {102,100,31,30,11,5,3};
  
  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    // sifh.Print(cerr);
    // cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 3);
  
  // simple insert
  int i = 32;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  // sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

  ScanOrderedInt(sifh, 8);
}

TEST_F(IX_IndexHandleTest, SmallDups) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {31,30,31,30,11,5,3};
  
  for (int i = 0; i < 7; i++) {
    RID r(i, i);
    RC rc;
    // cerr << "Inserting " << *(entries+i) << endl;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    // sifh.Print(cerr);
    // cerr << endl << endl;
  }
  ASSERT_EQ(sifh.GetHeight(), 3);
  
  // simple insert
  int i = 32;
  // cerr << "Inserting " << i << endl;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  // sifh.Print(cerr);
  ASSERT_EQ(sifh.GetHeight(), 3);

  ScanOrderedInt(sifh, 8);
}

TEST_F(IX_IndexHandleTest, 2SmallDups) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);
 
  // Simple insert - order 2
  int entries[] = {31,31,31,31,31,31,31};
  
  for (int i = 0; i < 20; i++) {
    RID r(i, i);
    RC rc;
    // cerr << "Inserting " << *(entries+0) << endl;
    rc = sifh.InsertEntry(entries+0, r);
    ASSERT_EQ(rc, 0);
    // // sifh.Print(cerr);
    ScanOrderedInt(sifh, i+1);
    // cerr << endl << endl;
  }
  
  // simple insert
  int i = 32;
  // cerr << "Inserting " << i << endl;
  RC rc = sifh.InsertEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  // // sifh.Print(cerr);

  ScanOrderedInt(sifh, 21);
}

void DeleteTest(int * entries, IX_IndexHandle& sifh) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);

  for (int i = 0; i < 7; i++) {
    RID r(0, 0);
    RC rc;
    // cerr << "Inserting " << *(entries+i) << endl;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    sifh.Print(cerr);
    ScanOrderedInt(sifh, i+1);
    cerr << endl << endl;
  }
  
  // simple delete
  int i = 3;
  cerr << "Deleting " << i << endl;
  RC rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);

  ScanOrderedInt(sifh, 6);

  // simple delete - largest key in leaf node
  i = 11;
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);

  ScanOrderedInt(sifh, 5);

  // simple delete - largest key in leaf node + dup
  i = 5;
  rc = sifh.InsertEntry(&i, RID(1,1));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(1,1));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 5);

  // simple delete - diff RID + dup
  i = 5;
  rc = sifh.InsertEntry(&i, RID(11,11));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 5);

  // underflow delete
  i = 5;
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(11,11));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 4);

  // underflow delete - multi-level
  i = 102;
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 3);

  // underflow delete - multi-level - root
  i = 100;
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);

  i = 31;
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 1);

  // underflow delete - last key - root
  i = 30;
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(0,0));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 0);
}

TEST_F(IX_IndexHandleTest, DelOrder1) {

  // Simple insert - order 1
  int entries[] = {102,100,31,30,11,5,3};
  DeleteTest(entries, sifh);
}

TEST_F(IX_IndexHandleTest, DelOrder2) {

  // Simple insert - order 2
  int entries[] = {3,5,11,30,31,100,102};
  DeleteTest(entries, sifh);
}


TEST_F(IX_IndexHandleTest, DelOrder3) {

  // Simple insert - order 3
  int entries[] = {31,102,100,30,3,11,5};
  DeleteTest(entries, sifh);
}

void DeleteTestDups(int * entries, IX_IndexHandle& sifh) {
  const BtreeNode * root = sifh.GetRoot();
  ASSERT_EQ(root->GetMaxKeys(), 3);

  for (int i = 0; i < 7; i++) {
    RID r(i, i);
    RC rc;
    cerr << "Inserting " << *(entries+i) << "\t" << r << endl;
    rc = sifh.InsertEntry(entries+i, r);
    ASSERT_EQ(rc, 0);
    sifh.Print(cerr);
    ScanOrderedInt(sifh, i+1);
    cerr << endl << endl;
  }
  
  int i = entries[0];
  RC rc = sifh.DeleteEntry(&i, RID(0,0));
  cerr << "Deleting " << i << endl;
  EXPECT_EQ(rc, 0);
  // non-existent delete
  rc = sifh.DeleteEntry(&i, RID(0,0));
  EXPECT_NE(rc, 0);

  sifh.Print(cerr);

  ScanOrderedInt(sifh, 6);

  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(1,1));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);

  ScanOrderedInt(sifh, 5);

  // simple delete - largest key in leaf node + dup
  rc = sifh.InsertEntry(&i, RID(10,10));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(10,10));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 5);

  int j = 32;
  rc = sifh.InsertEntry(&j, RID(11,11));
  ASSERT_EQ(rc, 0);
  // second identical insert should fail.
  rc = sifh.InsertEntry(&j, RID(11,11));
  ASSERT_NE(rc, 0);
  sifh.Print(cerr);

  cerr << "Deleting " << j << endl;
  rc = sifh.DeleteEntry(&j, RID(11,11));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 5);

  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(2,2));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 4);

  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(3,3));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);

  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(4,4));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 2);

  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(5,5));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 1);

  cerr << "Deleting " << i << endl;
  rc = sifh.DeleteEntry(&i, RID(6,6));
  ASSERT_EQ(rc, 0);
  sifh.Print(cerr);
  ScanOrderedInt(sifh, 0);

}

TEST_F(IX_IndexHandleTest, DelDups) {
  int entries[] = {31,31,31,31,31,31,31};
  DeleteTestDups(entries, sifh);
}

TEST_F(IX_IndexHandleTest, over40pages) {
  int numEntries = 0;
  for( int page = 1; page < 49; page++) 
  {
    RID rid;
    for ( int entry = 0; entry < ifh.GetRoot()->GetMaxKeys(); entry++ )
    {
      int t = 10;
      RC rc = ifh.InsertEntry((char *)&t, RID(page, numEntries++));
      if(rc != 0)
        cerr << "Page " << page << " Entry " << entry << endl;
      ASSERT_EQ(rc, 0);
    }
  }

  IX_IndexScan s;
  RC rc =  s.OpenScan(ifh, NO_OP, NULL);
  ASSERT_EQ(rc, 0);

  RID r;
  void * k;
  int count = 0;
  int ns = 0;
  while((s.GetNextEntry(k, r, ns)) != IX_EOF) {
    int curr = (*(int*)k);
    EXPECT_EQ(10, curr);
    EXPECT_EQ(count, r.Slot());
    count++;
    // cerr << "IX Scan entry - " << curr 
    //      << " Count " << count << endl; 
  }
  ASSERT_EQ(48*ifh.GetRoot()->GetMaxKeys(), count);

  rc =  s.CloseScan();
  ASSERT_EQ(rc, 0);  
}

// too slow
TEST_F(IX_IndexHandleTest, Big3levels) {
  int nEntries = 0;
  for( int level = 1; level <= 2; ) 
  {
    RID rid;
    for ( int entry = 0; entry < ifh.GetRoot()->GetMaxKeys(); entry++ )
    {
      int t = nEntries % 1000000;
      // if (nEntries > 6969) {
      //   cerr << "at " << nEntries << endl;
      //   ifh.GetRoot()->Print(cerr);
      // }
      RC rc = ifh.InsertEntry((char *)&t, RID(level, entry));
      if(rc != 0) {
        cerr << "Level " << level 
             << " nEntries " << nEntries 
             << " npages " << ifh.GetNumPages() 
             << endl;
        PrintErrorAll(rc);
      }
      ASSERT_EQ(rc, 0);
      nEntries++;
    }
    level = ifh.GetHeight();
    // cerr << "Going to Level " << level << " with nEntries " << nEntries << endl;
  }
}

TEST_F(IX_IndexHandleTest, sifh4levels) {
  int nEntries = 0;
  for( int level = 1; level < 7; ) 
  {
    RID rid;
    for ( int entry = 0; entry < sifh.GetRoot()->GetMaxKeys(); entry++ )
    {
      int t = level*100 + rand()%100000;
//      int t = level*100 + (entry * rand()%10);
      // cerr << "Inserting " << t << endl;

      RC rc = sifh.InsertEntry((char *)&t, RID(level, entry));
      if(rc != 0) {
        cerr << "Level " << level 
             << " nEntries " << nEntries 
             << " npages " << sifh.GetNumPages() 
             << endl;
        PrintErrorAll(rc);
      }
      ASSERT_EQ(rc, 0);
      nEntries++;
    }
    level = sifh.GetHeight();
    // cerr << "Going to Level " << level << " with nEntries " 
    //      << nEntries << endl;
    // sifh.Print(cerr);
    // cerr << "---------------------" << endl;
  }
}
