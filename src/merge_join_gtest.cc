#include "merge_join.h"
#include "file_scan.h"
#include "index_scan.h"
#include "gtest/gtest.h"

class MergeJoinTest : public ::testing::Test {
};


TEST_F(MergeJoinTest, Cons) {
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
    command << "echo \"create index in(in);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index in(bw);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index stars(soapid);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index stars(stname);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index stars(plays);\" | ./redbase " 
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
    cond.lhsAttr.attrName = "in";
    cond.rhsValue.data = NULL;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;

    Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status, cond);
    ASSERT_EQ(status, 0);
    Iterator* rfs = new IndexScan(smm, rmm, ixm, "in", "in", status, cond);
    ASSERT_EQ(status, 0);

    Condition jcond;
    jcond.op = EQ_OP;
    jcond.lhsAttr.relName = "in";
    jcond.rhsAttr.relName = "in";
    jcond.bRhsIsAttr = TRUE;
    jcond.lhsAttr.attrName = "in";
    jcond.rhsAttr.attrName = "in";

    Condition conds[5];
    conds[0] = jcond;

    MergeJoin fs(lfs, rfs, status, 1, 0, conds);
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
      //cerr << "=-----= " << t << endl;
      ns++;
      if(ns > 20) ASSERT_EQ(1, 0);
    }
    
    EXPECT_EQ(5, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);

    { // descending order join
      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                          cond, 0, NULL, true);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                          cond, 0, NULL, true);
      ASSERT_EQ(status, 0);

      Condition jcond;
      jcond.op = EQ_OP;
      jcond.lhsAttr.relName = "in";
      jcond.rhsAttr.relName = "in";
      jcond.bRhsIsAttr = TRUE;
      jcond.lhsAttr.attrName = "in";
      jcond.rhsAttr.attrName = "in";

      Condition conds[5];
      conds[0] = jcond;

      MergeJoin fs(lfs, rfs, status, 1, 0, conds);
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
        cerr << "=-----= " << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(5, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }

    { // bad types
      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status, cond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "in", "in", status, cond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "in";
      conds[0].rhsAttr.attrName = "out";
      MergeJoin fs(lfs, rfs, status, 1, 0, conds);
      ASSERT_EQ(status, QL_JOINKEYTYPEMISMATCH);
    }

    { // bad attrname
      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status, cond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "in", "in", status, cond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "ffdfdfdf";
      conds[0].rhsAttr.attrName = "out";
      MergeJoin fs(lfs, rfs, status, 1, 0, conds);
      ASSERT_EQ(status, QL_BADJOINKEY);
    }

    { // different relations
      Condition lcond;
      lcond.op = EQ_OP;
      lcond.lhsAttr.relName = "in";
      lcond.lhsAttr.attrName = "in";
      lcond.rhsValue.data = NULL;

      Condition rcond;
      rcond.op = EQ_OP;
      rcond.lhsAttr.relName = "stars";
      rcond.lhsAttr.attrName = "soapid";
      rcond.rhsValue.data = NULL;


      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                          lcond, 0, NULL, false);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "stars", "soapid",
                                          status, rcond, 0, NULL, false);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "in";
      conds[0].rhsAttr.attrName = "soapid";
      MergeJoin fs(lfs, rfs, status, 1, 0, conds);
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
        cerr << "=---=" << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(15, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }
    
    { // different relations - cross product - desc
      Condition lcond;
      lcond.op = EQ_OP;
      lcond.lhsAttr.relName = "in";
      lcond.lhsAttr.attrName = "in";
      lcond.rhsValue.data = NULL;

      Condition rcond;
      rcond.op = EQ_OP;
      rcond.lhsAttr.relName = "stars";
      rcond.lhsAttr.attrName = "soapid";
      rcond.rhsValue.data = NULL;


      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                          lcond, 0, NULL, true);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "stars", "soapid",
                                          status, rcond, 0, NULL, true);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "in";
      conds[0].rhsAttr.attrName = "soapid";
      MergeJoin fs(lfs, rfs, status, 1, 0, conds);
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
        cerr << "**---=" << t << endl;
        ns++;
        if(ns > 20) ASSERT_EQ(1, 0);
      }
    
      EXPECT_EQ(15, ns);
      (rc=fs.Close());
      ASSERT_EQ(rc, 0);
    }
    


    { // nothing to join

      Condition lcond;
      lcond.op = EQ_OP;
      lcond.lhsAttr.relName = "in";
      lcond.lhsAttr.attrName = "bw";
      lcond.rhsValue.data = NULL;

      Condition rcond;
      rcond.op = EQ_OP;
      rcond.lhsAttr.relName = "stars";
      rcond.lhsAttr.attrName = "stname";
      rcond.rhsValue.data = NULL;


      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "bw", status, lcond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "stars", "stname", status, rcond);
      ASSERT_EQ(status, 0);


      conds[0].lhsAttr.attrName = "bw";
      conds[0].rhsAttr.attrName = "stname";
      MergeJoin fs(lfs, rfs, status, 1, 0, conds);
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

    { // cross product - more than 1 condition

      Condition lcond;
      lcond.op = EQ_OP;
      lcond.lhsAttr.relName = "in";
      lcond.lhsAttr.attrName = "in";
      lcond.rhsValue.data = NULL;

      Condition rcond;
      rcond.op = EQ_OP;
      rcond.lhsAttr.relName = "stars";
      rcond.lhsAttr.attrName = "soapid";
      rcond.rhsValue.data = NULL;


      Iterator* lfs = new IndexScan(smm, rmm, ixm, "in", "in", status, lcond);
      ASSERT_EQ(status, 0);
      Iterator* rfs = new IndexScan(smm, rmm, ixm, "stars", "soapid", status, rcond);
      ASSERT_EQ(status, 0);

      conds[0].lhsAttr.attrName = "in";
      conds[0].lhsAttr.relName = "in";
      conds[0].rhsAttr.attrName = "soapid";
      conds[0].rhsAttr.relName = "stars";
      conds[0].op = EQ_OP;

      conds[1].lhsAttr.attrName = "in";
      conds[1].lhsAttr.relName = "in";
      conds[1].rhsAttr.attrName = "starid";
      conds[1].rhsAttr.relName = "stars";
      conds[1].op = GT_OP;
      MergeJoin fs(lfs, rfs, status, 2, 0, conds);
      ASSERT_EQ(status, 0);
      rc=fs.Open();
      ASSERT_EQ(rc, 0);

      Tuple t = fs.GetTuple();

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

