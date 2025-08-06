#include "merge_join.h"
#include "file_scan.h"
#include "index_scan.h"
#include "gtest/gtest.h"

class MergeJoinTest : public ::testing::Test {
protected:
	MergeJoinTest() {}
	virtual void SetUp() 
	{
		// Clean up any existing test database
		system("rm -rf merge_join_test");
	}

	virtual void TearDown() 
  {
		// Clean up test database
		system("rm -rf merge_join_test");
	}
};


TEST_F(MergeJoinTest, Cons) {
    RC rc;
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);

    const char * dbname = "merge_join_test";
    
    // Database setup
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

    // Helper function to create a basic scan condition
    auto createScanCondition = [](const char* relName, const char* attrName) -> Condition {
        Condition scanCond;
        scanCond.op = EQ_OP;
        scanCond.lhsAttr.relName = const_cast<char*>(relName);
        scanCond.lhsAttr.attrName = const_cast<char*>(attrName);
        scanCond.rhsValue.data = NULL;
        scanCond.bRhsIsAttr = FALSE;
        return scanCond;
    };

    // Helper function to create a join condition
    auto createJoinCondition = [](const char* leftRel, const char* leftAttr,
                                 const char* rightRel, const char* rightAttr,
                                 CompOp op = EQ_OP) -> Condition {
        Condition joinCond;
        joinCond.op = op;
        joinCond.lhsAttr.relName = const_cast<char*>(leftRel);
        joinCond.lhsAttr.attrName = const_cast<char*>(leftAttr);
        joinCond.rhsAttr.relName = const_cast<char*>(rightRel);
        joinCond.rhsAttr.attrName = const_cast<char*>(rightAttr);
        joinCond.bRhsIsAttr = TRUE;
        return joinCond;
    };

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    RC status = -1;

    // Test Case 1: Basic ascending merge join
    {
        Condition inScanCondition = createScanCondition("in", "in");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);

        Condition selfJoinCondition = createJoinCondition("in", "in", "in", "in");
        Condition joinConditions[] = {selfJoinCondition};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, 0);

        rc = mj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(mj.GetAttrCount(), mj.TupleLength());
        t.SetAttr(mj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = mj.GetNext(t);
            if(rc == mj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            //cerr << "=-----= " << t << endl;
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0); // Safety check
        }
        
        EXPECT_EQ(5, resultCount);
        rc = mj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 2: Descending order merge join
    {
        Condition inScanCondition = createScanCondition("in", "in");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status, 
                                          inScanCondition, 0, NULL, true);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                           inScanCondition, 0, NULL, true);
        ASSERT_EQ(status, 0);

        Condition descJoinCondition = createJoinCondition("in", "in", "in", "in");
        Condition joinConditions[] = {descJoinCondition};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, 0);

        rc = mj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(mj.GetAttrCount(), mj.TupleLength());
        t.SetAttr(mj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = mj.GetNext(t);
            if(rc == mj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            //cerr << "=-----= " << t << endl;
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(5, resultCount);
        rc = mj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 3: Type mismatch error test
    {
        Condition inScanCondition = createScanCondition("in", "in");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);

        // Join int column with float column - should fail
        Condition typeMismatchJoin = createJoinCondition("in", "in", "in", "out");
        Condition joinConditions[] = {typeMismatchJoin};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, QL_JOINKEYTYPEMISMATCH);
    }

    // Test Case 4: Bad attribute name error test
    {
        Condition inScanCondition = createScanCondition("in", "in");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);

        // Use non-existent attribute name
        Condition badAttrJoin = createJoinCondition("in", "ffdfdfdf", "in", "out");
        Condition joinConditions[] = {badAttrJoin};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, QL_BADJOINKEY);
    }

    // Test Case 5: Cross-table merge join (ascending)
    {
        Condition inScanCondition = createScanCondition("in", "in");
        Condition starsScanCondition = createScanCondition("stars", "soapid");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                          inScanCondition, 0, NULL, false);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "stars", "soapid",
                                           status, starsScanCondition, 0, NULL, false);
        ASSERT_EQ(status, 0);

        Condition crossTableJoin = createJoinCondition("in", "in", "stars", "soapid");
        Condition joinConditions[] = {crossTableJoin};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, 0);
        
        rc = mj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(mj.GetAttrCount(), mj.TupleLength());
        t.SetAttr(mj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = mj.GetNext(t);
            if(rc == mj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            //cerr << "=---=" << t << endl;
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(15, resultCount);
        rc = mj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 6: Cross-table merge join (descending)
    {
        Condition inScanCondition = createScanCondition("in", "in");
        Condition starsScanCondition = createScanCondition("stars", "soapid");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status,
                                          inScanCondition, 0, NULL, true);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "stars", "soapid",
                                           status, starsScanCondition, 0, NULL, true);
        ASSERT_EQ(status, 0);

        Condition crossTableDescJoin = createJoinCondition("in", "in", "stars", "soapid");
        Condition joinConditions[] = {crossTableDescJoin};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, 0);
        
        rc = mj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(mj.GetAttrCount(), mj.TupleLength());
        t.SetAttr(mj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = mj.GetNext(t);
            if(rc == mj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            //cerr << "**---=" << t << endl;
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(15, resultCount);
        rc = mj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 7: String column merge join (no matches expected)
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        Condition starsScanCondition = createScanCondition("stars", "stname");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "bw", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "stars", "stname", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        Condition stringJoin = createJoinCondition("in", "bw", "stars", "stname");
        Condition joinConditions[] = {stringJoin};

        MergeJoin mj(leftScan, rightScan, status, 1, 0, joinConditions);
        ASSERT_EQ(status, 0);
        
        rc = mj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(mj.GetAttrCount(), mj.TupleLength());
        t.SetAttr(mj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = mj.GetNext(t);
            if(rc == mj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            // cerr << t << endl;
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(0, resultCount);
        rc = mj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 8: Multiple join conditions
    {
        Condition inScanCondition = createScanCondition("in", "in");
        Condition starsScanCondition = createScanCondition("stars", "soapid");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new IndexScan(smm, rmm, ixm, "stars", "soapid", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        // Multiple join conditions
        Condition multiJoinConditions[] = {
            createJoinCondition("in", "in", "stars", "soapid", EQ_OP),
            createJoinCondition("in", "in", "stars", "starid", GT_OP)
        };

        MergeJoin mj(leftScan, rightScan, status, 2, 0, multiJoinConditions);
        ASSERT_EQ(status, 0);
        
        rc = mj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t = mj.GetTuple();

        int resultCount = 0;
        while(1) {
            rc = mj.GetNext(t);
            if(rc == mj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            //cerr << t << endl;
            resultCount++;
        }
        
        EXPECT_EQ(2, resultCount);
        rc = mj.Close();
        ASSERT_EQ(rc, 0);
    }

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
