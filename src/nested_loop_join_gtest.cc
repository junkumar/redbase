#include "nested_loop_join.h"
#include "file_scan.h"
#include "index_scan.h"
#include "gtest/gtest.h"

struct TestRec {
    int   in;
    float out;
    char  bw[2];
};

class NestedLoopJoinTest : public ::testing::Test {
protected:
	NestedLoopJoinTest() {}
	virtual void SetUp() 
	{
	}

	virtual void TearDown() 
  {
	}

};


TEST_F(NestedLoopJoinTest, Cons) {
    RC rc;
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);

    const char * dbname = "test";
    stringstream command;
    command << "rm -rf " << dbname;
    rc = system (command.str().c_str());

    command.str("");
    command << "./dbcreate " << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table in(in i, out f, bw c4);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table stars(starid  i, stname  c20, plays  c12, soapid  i);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index in(bw);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    command.str("");
    command << "echo \"load in(\\\"../data\\\");\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load stars(\\\"../stars.data\\\");\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    // command.str("");
    // command << "echo \"help in;\" | ./redbase " 
    //         << dbname;
    // rc = system (command.str().c_str());
    // ASSERT_EQ(rc, 0);

    Condition cond;
    cond.op = EQ_OP;
    cond.lhsAttr.relName = "in";
    cond.lhsAttr.attrName = "bw";
    cond.rhsValue.data = NULL;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;

    Iterator* lfs = new FileScan(smm, rmm, "in", status, cond);
    ASSERT_EQ(status, 0);
    Iterator* rfs = new FileScan(smm, rmm, "in", status, cond);
    ASSERT_EQ(status, 0);

    Condition jcond;
    jcond.op = EQ_OP;
    jcond.lhsAttr.relName = "left";
    jcond.rhsAttr.relName = "right";
    jcond.bRhsIsAttr = TRUE;
    jcond.lhsAttr.attrName = "in";
    jcond.rhsAttr.attrName = "in";

    Condition conds[5];
    conds[0] = jcond;

    NestedLoopJoin fs(lfs, rfs, status, 1, conds);
    ASSERT_EQ(status, 0);

    rc=fs.Open();
    ASSERT_EQ(rc, 0);

    Tuple t(fs.GetAttrCount(), fs.TupleLength());
    t.SetAttr(fs.GetAttr());

    int ns = 0;
    while(1) {
      rc = fs.GetNext(t);
      if(rc ==  fs.Eof())
        break;
      EXPECT_EQ(rc, 0);
      if(rc != 0)
        PrintErrorAll(rc);
      // cerr << t << endl;
      ns++;
      if(ns > 20) ASSERT_EQ(1, 0);
    }
    
    EXPECT_EQ(5, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);
 
    { // bad types
      Iterator* lfs = new FileScan(smm, rmm, "in", status, cond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new FileScan(smm, rmm, "in", status, cond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "in";
      conds[0].rhsAttr.attrName = "out";
      NestedLoopJoin fs(lfs, rfs, status, 1, conds);
      ASSERT_EQ(status, QL_JOINKEYTYPEMISMATCH);
    }

    { // bad attrname
      Iterator* lfs = new FileScan(smm, rmm, "in", status, cond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new FileScan(smm, rmm, "in", status, cond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "ffdfdfdf";
      conds[0].rhsAttr.attrName = "out";
      NestedLoopJoin fs(lfs, rfs, status, 1, conds);
      ASSERT_EQ(status, QL_BADJOINKEY);
    }

    { // different relations

      Iterator* lfs = new FileScan(smm, rmm, "in", status, cond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new FileScan(smm, rmm, "stars", status, cond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "in";
      conds[0].rhsAttr.attrName = "soapid";
      NestedLoopJoin fs(lfs, rfs, status, 1, conds);
      ASSERT_EQ(status, 0);
      rc=fs.Open();
      ASSERT_EQ(rc, 0);

      Tuple t(fs.GetAttrCount(), fs.TupleLength());
      t.SetAttr(fs.GetAttr());

      int ns = 0;
      while(1) {
        rc = fs.GetNext(t);
        if(rc ==  fs.Eof())
          break;
        EXPECT_EQ(rc, 0);
        if(rc != 0)
          PrintErrorAll(rc);
        // cerr << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(15, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }
    
  
    { // nothing to join

      Iterator* lfs = new FileScan(smm, rmm, "in", status, cond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new FileScan(smm, rmm, "stars", status, cond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "bw";
      conds[0].rhsAttr.attrName = "stname";
      NestedLoopJoin fs(lfs, rfs, status, 1, conds);
      ASSERT_EQ(status, 0);
      rc=fs.Open();
      ASSERT_EQ(rc, 0);

      Tuple t(fs.GetAttrCount(), fs.TupleLength());
      t.SetAttr(fs.GetAttr());

      int ns = 0;
      while(1) {
        rc = fs.GetNext(t);
        if(rc ==  fs.Eof())
          break;
        EXPECT_EQ(rc, 0);
        if(rc != 0)
          PrintErrorAll(rc);
        // cerr << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(0, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    { // index on one side
      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "bw", status, cond);
      ASSERT_EQ(status, 0);

      int ac = -1;
      DataAttrInfo * attr;
      smm.GetFromTable("stars", ac, attr);
      Condition cond2;
      cond2.op = EQ_OP;
      cond2.lhsAttr.attrName = attr[0].attrName;
      cond2.lhsAttr.relName = attr[0].relName;
      cond2.rhsValue.data = NULL;
      cond2.rhsValue.data = FALSE;

      Iterator* rfs = new FileScan(smm, rmm, "stars", status, cond2);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "bw";
      conds[0].rhsAttr.attrName = "plays";
      NestedLoopJoin fs(lfs, rfs, status, 1, conds);
      ASSERT_EQ(status, 0);
      rc=fs.Open();
      ASSERT_EQ(rc, 0);

      Tuple t(fs.GetAttrCount(), fs.TupleLength());
      t.SetAttr(fs.GetAttr());

      int ns = 0;
      while(1) {
        rc = fs.GetNext(t);
        if(rc ==  fs.Eof())
          break;
        EXPECT_EQ(rc, 0);
        if(rc != 0)
          PrintErrorAll(rc);
        // cerr << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(1, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    { // cross product

      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "bw", status, cond);
      ASSERT_EQ(status, 0);

      int ac = -1;
      DataAttrInfo * attr;
      smm.GetFromTable("stars", ac, attr);
      Condition cond2;
      cond2.op = EQ_OP;
      cond2.lhsAttr.attrName = attr[0].attrName;
      cond2.lhsAttr.relName = attr[0].relName;
      cond2.rhsValue.data = NULL;
      cond2.rhsValue.data = FALSE;

      Iterator* rfs = new FileScan(smm, rmm, "stars", status, cond2);
      ASSERT_EQ(status, 0);

      //rc = smm.Print("in");
      //rc = smm.Print("stars");

      conds[0].lhsAttr.attrName = "bw";
      conds[0].rhsAttr.attrName = "plays";
      NestedLoopJoin fs(lfs, rfs, status, 0, conds);
      ASSERT_EQ(status, 0);
      rc=fs.Open();
      ASSERT_EQ(rc, 0);

      Tuple t(fs.GetAttrCount(), fs.TupleLength());
      t.SetAttr(fs.GetAttr());

      int ns = 0;
      while(1) {
        rc = fs.GetNext(t);
        if(rc ==  fs.Eof())
          break;
        EXPECT_EQ(rc, 0);
        if(rc != 0)
          PrintErrorAll(rc);
        //cerr << t << endl;
        ns++;
      }
    
      EXPECT_EQ(29*5, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    { // cross product - more than 1 condition

      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "bw", status, cond);
      ASSERT_EQ(status, 0);

      int ac = -1;
      DataAttrInfo * attr;
      smm.GetFromTable("stars", ac, attr);
      Condition cond2;
      cond2.op = EQ_OP;
      cond2.lhsAttr.attrName = attr[0].attrName;
      cond2.lhsAttr.relName = attr[0].relName;
      cond2.rhsValue.data = NULL;
      cond2.rhsValue.data = FALSE;

      Iterator* rfs = new FileScan(smm, rmm, "stars", status, cond2);
      ASSERT_EQ(status, 0);

      // rc = smm.Print("in");
      // rc = smm.Print("stars");

      conds[0].lhsAttr.attrName = "in";
      conds[0].lhsAttr.relName = "in";
      conds[0].rhsAttr.attrName = "soapid";
      conds[0].lhsAttr.relName = "stars";
      conds[0].op = EQ_OP;

      conds[1].lhsAttr.attrName = "in";
      conds[1].lhsAttr.relName = "in";
      conds[1].rhsAttr.attrName = "starid";
      conds[1].lhsAttr.relName = "stars";
      conds[1].op = GT_OP;
      NestedLoopJoin fs(lfs, rfs, status, 2, conds);
      ASSERT_EQ(status, 0);
      rc=fs.Open();
      ASSERT_EQ(rc, 0);

      Tuple t(fs.GetAttrCount(), fs.TupleLength());
      t.SetAttr(fs.GetAttr());

      int ns = 0;
      while(1) {
        rc = fs.GetNext(t);
        if(rc ==  fs.Eof())
          break;
        EXPECT_EQ(rc, 0);
        if(rc != 0)
          PrintErrorAll(rc);
        // cerr << t << endl;
        ns++;
      }
    
      EXPECT_EQ(2, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

