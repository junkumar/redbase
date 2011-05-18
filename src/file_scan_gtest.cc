#include "file_scan.h"
#include "sm.h"
#include "gtest/gtest.h"

struct TestRec {
    int   in;
    float out;
    char  bw[2];
};

class FileScanTest : public ::testing::Test {
protected:
	FileScanTest() {}
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

TEST_F(FileScanTest, Cons) {
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
    cond.op = EQ_OP;
    cond.lhsAttr.attrName = "in";
    cond.rhsValue.data = NULL;

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;
    FileScan fs(smm, rmm, "in", status, cond);
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

      cerr << t << "\t" << t.GetRid() << endl;
      ns++;
    }
    
    EXPECT_EQ(5, ns);
    (rc=fs.Close());
    ASSERT_EQ(rc, 0);


    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
