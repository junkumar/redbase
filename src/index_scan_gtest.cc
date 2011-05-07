#include "index_scan.h"
#include "sm.h"
#include "gtest/gtest.h"

struct TestRec {
    int   in;
    float out;
    char  bw[2];
};

class IndexScanTest : public ::testing::Test {
protected:
	IndexScanTest() {}
	virtual void SetUp() 
	{
	}

	virtual void TearDown() 
  {
	}

  // Declares the variables your tests want to use.
	// SM_Manager smm;
	// RM_Manager rmm;
};

TEST_F(IndexScanTest, Cons) {
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
    command << "echo \"create table in(in i, out f, bw c2);\" | ./redbase " 
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
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    // no such table
    command.str("");
    command << "echo \"help in;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    Condition cond;
    cond.op = GT_OP;
    cond.lhsAttr.relName = "in";
    cond.lhsAttr.attrName = "bw";
    cond.bRhsIsAttr = FALSE;
    char * val = "c";
    cond.rhsValue.data = val;
    cond.rhsValue.type = STRING;


    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;
    IndexScan fs(smm, rmm, ixm, "in", "bw", cond, status);
    ASSERT_EQ(status, 0);

    rc=fs.Open();
    ASSERT_EQ(rc, 0);

    Tuple t(3, sizeof(int)+sizeof(float)+2);
    t.SetAttr(fs.GetAttr());

    int ns = 0;
    while(1) {
      rc = fs.GetNext(t);
      if(rc ==  fs.Eof())
        break;
      EXPECT_EQ(rc, 0);
      int in = -1;
      float out = -1;
      char bw[3]; bw[2] = 0;
      t.Get("in", in);
      t.Get("out", out);
      t.Get("bw", bw);

      cerr << in << "\t" << out << "\t" << bw << endl;
      ns++;
    }
    
    EXPECT_EQ(3, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);


    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
