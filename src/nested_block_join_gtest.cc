#include "nested_block_join.h"
#include "sm.h"
#include "gtest/gtest.h"
#include "projection.h"

class NestedBlockJoinTest : public ::testing::Test {
protected:
	NestedBlockJoinTest() {}
	virtual void SetUp() 
	{
	}

	virtual void TearDown() 
  {
	}
};

TEST_F(NestedBlockJoinTest, Contest) {
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
    command << "echo \"CREATE TABLE CUSTOMER ( C_CUSTKEY       i4,                        C_NAME          c25,                        C_ADDRESS       c40,                        C_NATIONKEY     i4,                        C_PHONE         c15,                        C_ACCTBAL       f4,                        C_MKTSEGMENT    c10,                        C_COMMENT       c117 );\"| ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index CUSTOMER(C_PHONE);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"CREATE TABLE SUPPLIER ( S_SUPPKEY       i4,                        S_NAME          c25,                        S_ADDRESS       c40,                        S_NATIONKEY     i4,                        S_PHONE         c15,                        S_ACCTBAL       f4,                        S_COMMENT       c101 );\" | ./redbase "
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load CUSTOMER(\\\"../../data/contest/customer.data\\\");\" | ./redbase " 
            << dbname;
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load SUPPLIER(\\\"../../data/contest/supplier.data\\\");\" | ./redbase " 
            << dbname;
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"print relcat;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    Condition cond;
    cond.op = EQ_OP;
    cond.lhsAttr.relName = "CUSTOMER";
    cond.lhsAttr.attrName = "C_NATIONKEY";
    cond.bRhsIsAttr = TRUE;
    cond.rhsAttr.relName = "SUPPLIER";
    cond.rhsAttr.attrName = "S_NATIONKEY";
    
    Condition conds[1];
    conds[0] = cond;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;
    FileScan* rfs = new FileScan(smm, rmm, "CUSTOMER", status);
    ASSERT_EQ(status, 0);

    FileScan* lfs = new FileScan(smm, rmm, "SUPPLIER", status);
    ASSERT_EQ(status, 0);

    NestedBlockJoin* it = new NestedBlockJoin(lfs, rfs, status, 1, conds);
    // PrintErrorAll(status);
    ASSERT_EQ(status, 0);

    AggRelAttr selAttrs[2];
    selAttrs[0].relName = "CUSTOMER";
    selAttrs[0].attrName = "C_CUSTKEY";
    selAttrs[1].relName = "SUPPLIER";
    selAttrs[1].attrName = "S_SUPPKEY";

    Projection fs(it, status, 2, selAttrs);

    rc=fs.Open();
    ASSERT_EQ(rc, 0);

    Tuple t = fs.GetTuple();

    int ns = 0;
    while(1) {
      rc = fs.GetNext(t);
      if(rc ==  fs.Eof())
        break;
      EXPECT_EQ(rc, 0);

      //cerr << t << endl;
      //cerr << ns << " records so far" << endl;
      ns++;
    }
    
    EXPECT_EQ(5929, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

TEST_F(NestedBlockJoinTest, ContestSmallBuf) {
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
    command << "echo \"CREATE TABLE CUSTOMER ( C_CUSTKEY       i4,                        C_NAME          c25,                        C_ADDRESS       c40,                        C_NATIONKEY     i4,                        C_PHONE         c15,                        C_ACCTBAL       f4,                        C_MKTSEGMENT    c10,                        C_COMMENT       c117 );\"| ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index CUSTOMER(C_PHONE);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"CREATE TABLE SUPPLIER ( S_SUPPKEY       i4,                        S_NAME          c25,                        S_ADDRESS       c40,                        S_NATIONKEY     i4,                        S_PHONE         c15,                        S_ACCTBAL       f4,                        S_COMMENT       c101 );\" | ./redbase "
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load CUSTOMER(\\\"../../data/contest/customer.data\\\");\" | ./redbase " 
            << dbname;
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load SUPPLIER(\\\"../../data/contest/supplier.data\\\");\" | ./redbase " 
            << dbname;
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"print relcat;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    Condition cond;
    cond.op = EQ_OP;
    cond.lhsAttr.relName = "CUSTOMER";
    cond.lhsAttr.attrName = "C_NATIONKEY";
    cond.bRhsIsAttr = TRUE;
    cond.rhsAttr.relName = "SUPPLIER";
    cond.rhsAttr.attrName = "S_NATIONKEY";
    
    Condition conds[1];
    conds[0] = cond;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;
    FileScan* rfs = new FileScan(smm, rmm, "CUSTOMER", status);
    ASSERT_EQ(status, 0);

    FileScan* lfs = new FileScan(smm, rmm, "SUPPLIER", status);
    ASSERT_EQ(status, 0);

    NestedBlockJoin* it = new NestedBlockJoin(lfs, rfs, status, 1, conds, 4);
    // PrintErrorAll(status);
    ASSERT_EQ(status, 0);

    AggRelAttr selAttrs[2];
    selAttrs[0].relName = "CUSTOMER";
    selAttrs[0].attrName = "C_CUSTKEY";
    selAttrs[1].relName = "SUPPLIER";
    selAttrs[1].attrName = "S_SUPPKEY";

    Projection fs(it, status, 2, selAttrs);

    rc=fs.Open();
    ASSERT_EQ(rc, 0);

    Tuple t = fs.GetTuple();

    int ns = 0;
    while(1) {
      rc = fs.GetNext(t);
      if(rc ==  fs.Eof())
        break;
      EXPECT_EQ(rc, 0);

      //cerr << t << endl;
      //cerr << ns << " records so far" << endl;
      ns++;
    }
    
    EXPECT_EQ(5929, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);
    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

TEST_F(NestedBlockJoinTest, Cons) {
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
    command << "echo \"create table soaps(soapid  i, sname  c28, network  c4, rating  f);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"CREATE TABLE networks(nid  i, name c4, viewers i);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load soaps(\\\"../soaps.data\\\");\" | ./redbase " 
            << dbname;
    cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load networks(\\\"../networks.data\\\");\" | ./redbase " 
            << dbname;
    cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    command.str("");
    command << "echo \"print relcat;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    Condition cond;
    cond.op = EQ_OP;
    cond.lhsAttr.relName = "soaps";
    cond.lhsAttr.attrName = "network";
    cond.bRhsIsAttr = TRUE;
    cond.rhsAttr.relName = "networks";
    cond.rhsAttr.attrName = "name";
    
    Condition conds[1];
    conds[0] = cond;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;
    FileScan* rfs = new FileScan(smm, rmm, "soaps", status);
    ASSERT_EQ(status, 0);

    FileScan* lfs = new FileScan(smm, rmm, "networks", status);
    ASSERT_EQ(status, 0);

    NestedBlockJoin fs(lfs, rfs, status, 1, conds);
    PrintErrorAll(status);
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

      cerr << t << "\t" << t.GetRid() << endl;
      //cerr << ns << " records so far" << endl;
      ns++;
    }
    
    EXPECT_EQ(9, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);


    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
