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


    command.str("");
    command << "echo \"help in;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    Condition cond;
    cond.op = EQ_OP;
    cond.lhsAttr.relName = "in";
    cond.lhsAttr.attrName = "bw";
    cond.rhsValue.data = NULL;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;

    FileScan lfs(smm, rmm, "in", cond, status);
    ASSERT_EQ(status, 0);
    FileScan rfs(smm, rmm, "in", cond, status);
    ASSERT_EQ(status, 0);

    NestedLoopJoin fs("in", "in", &lfs, &rfs, status);
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
      cerr << t << endl;
      ns++;
      if(ns > 20) ASSERT_EQ(1, 0);
    }
    
    EXPECT_EQ(5, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);

    {
      NestedLoopJoin fs("in", "out", &lfs, &rfs, status);
      ASSERT_EQ(status, QL_JOINKEYTYPEMISMATCH);
    }

    { // different relations

      FileScan lfs(smm, rmm, "in", cond, status);
      ASSERT_EQ(status, 0);

      FileScan rfs(smm, rmm, "stars", cond, status);
      ASSERT_EQ(status, 0);

      NestedLoopJoin fs("in", "soapid", &lfs, &rfs, status);
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
        cerr << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(15, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    { // nothing to join

      FileScan lfs(smm, rmm, "in", cond, status);
      ASSERT_EQ(status, 0);

      FileScan rfs(smm, rmm, "stars", cond, status);
      ASSERT_EQ(status, 0);

      NestedLoopJoin fs("bw", "stname", &lfs, &rfs, status);
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
        cerr << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(0, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    { // index on one side

      IndexScan lfs(smm, rmm, ixm, "in", "bw", cond, status);
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

      FileScan rfs(smm, rmm, "stars", cond2, status);
      ASSERT_EQ(status, 0);

      NestedLoopJoin fs("bw", "plays", &lfs, &rfs, status);
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
        cerr << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(1, ns);
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

