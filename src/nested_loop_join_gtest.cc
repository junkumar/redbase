#include "nested_loop_join.h"
#include "file_scan.h"
#include "index_scan.h"
#include "gtest/gtest.h"

class NestedLoopJoinTest : public ::testing::Test {
protected:
	NestedLoopJoinTest() {}
	virtual void SetUp() 
	{
		// Clean up any existing test database
    stringstream command;
    command << "rm -rf " << dbname;
    system (command.str().c_str());
	}

	virtual void TearDown() 
  {
		// Clean up test database
    stringstream command;
    command << "rm -rf " << dbname;
    system (command.str().c_str());
	}
  const char * dbname = "nested_loop_join_test";
};


TEST_F(NestedLoopJoinTest, Cons) {
    RC rc;
    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);

    // Database setup
    stringstream command;
 
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

    // Test Case 1: Basic self-join on 'in' table
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        
        Iterator* leftScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);

        Condition selfJoinCondition = createJoinCondition("left", "in", "right", "in");
        Condition joinConditions[] = {selfJoinCondition};

        NestedLoopJoin nlj(leftScan, rightScan, status, 1, joinConditions);
        ASSERT_EQ(status, 0);

        rc = nlj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(nlj.GetAttrCount(), nlj.TupleLength());
        t.SetAttr(nlj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = nlj.GetNext(t);
            if(rc == nlj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0); // Safety check
        }
        
        EXPECT_EQ(5, resultCount);
        rc = nlj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 2: Type mismatch error test
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        
        Iterator* leftScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);

        // Join int column with float column - should fail
        Condition typeMismatchJoin = createJoinCondition("left", "in", "right", "out");
        Condition joinConditions[] = {typeMismatchJoin};

        NestedLoopJoin nlj(leftScan, rightScan, status, 1, joinConditions);
        ASSERT_EQ(status, QL_JOINKEYTYPEMISMATCH);
    }

    // Test Case 3: Bad attribute name error test
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        
        Iterator* leftScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);

        // Use non-existent attribute name
        Condition badAttrJoin = createJoinCondition("left", "ffdfdfdf", "right", "out");
        Condition joinConditions[] = {badAttrJoin};

        NestedLoopJoin nlj(leftScan, rightScan, status, 1, joinConditions);
        ASSERT_EQ(status, QL_BADJOINKEY);
    }

    // Test Case 4: Join between different relations
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        Condition starsScanCondition = createScanCondition("stars", "starid");
        
        Iterator* leftScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new FileScan(smm, rmm, "stars", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        Condition crossTableJoin = createJoinCondition("in", "in", "stars", "soapid");
        Condition joinConditions[] = {crossTableJoin};

        NestedLoopJoin nlj(leftScan, rightScan, status, 1, joinConditions);
        ASSERT_EQ(status, 0);
        
        rc = nlj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(nlj.GetAttrCount(), nlj.TupleLength());
        t.SetAttr(nlj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = nlj.GetNext(t);
            if(rc == nlj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(15, resultCount);
        rc = nlj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 5: No matching join (empty result)
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        Condition starsScanCondition = createScanCondition("stars", "starid");
        
        Iterator* leftScan = new FileScan(smm, rmm, "in", status, inScanCondition);
        ASSERT_EQ(status, 0);
        Iterator* rightScan = new FileScan(smm, rmm, "stars", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        // Join incompatible columns - should return no results
        Condition noMatchJoin = createJoinCondition("in", "bw", "stars", "stname");
        Condition joinConditions[] = {noMatchJoin};

        NestedLoopJoin nlj(leftScan, rightScan, status, 1, joinConditions);
        ASSERT_EQ(status, 0);
        
        rc = nlj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(nlj.GetAttrCount(), nlj.TupleLength());
        t.SetAttr(nlj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = nlj.GetNext(t);
            if(rc == nlj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(0, resultCount);
        rc = nlj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 6: One side Index scan with join
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "bw", status, inScanCondition);
        ASSERT_EQ(status, 0);

        // Get stars table metadata properly
        int attrCount = -1;
        DataAttrInfo* starsAttrs;
        smm.GetFromTable("stars", attrCount, starsAttrs);
        
        Condition starsScanCondition;
        starsScanCondition.op = EQ_OP;
        starsScanCondition.lhsAttr.attrName = starsAttrs[0].attrName;
        starsScanCondition.lhsAttr.relName = starsAttrs[0].relName;
        starsScanCondition.rhsValue.data = NULL;
        starsScanCondition.bRhsIsAttr = FALSE; // Fixed the bug

        Iterator* rightScan = new FileScan(smm, rmm, "stars", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        Condition indexJoin = createJoinCondition("in", "bw", "stars", "plays");
        Condition joinConditions[] = {indexJoin};

        NestedLoopJoin nlj(leftScan, rightScan, status, 1, joinConditions);
        ASSERT_EQ(status, 0);
        
        rc = nlj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(nlj.GetAttrCount(), nlj.TupleLength());
        t.SetAttr(nlj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = nlj.GetNext(t);
            if(rc == nlj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            resultCount++;
            if(resultCount > 20) ASSERT_EQ(1, 0);
        }
        
        EXPECT_EQ(1, resultCount);
        rc = nlj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 7: Cross product (no join conditions)
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "bw", status, inScanCondition);
        ASSERT_EQ(status, 0);

        int attrCount = -1;
        DataAttrInfo* starsAttrs;
        smm.GetFromTable("stars", attrCount, starsAttrs);
        
        Condition starsScanCondition;
        starsScanCondition.op = EQ_OP;
        starsScanCondition.lhsAttr.attrName = starsAttrs[0].attrName;
        starsScanCondition.lhsAttr.relName = starsAttrs[0].relName;
        starsScanCondition.rhsValue.data = NULL;
        starsScanCondition.bRhsIsAttr = FALSE;

        Iterator* rightScan = new FileScan(smm, rmm, "stars", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        // No join conditions - cross product
        NestedLoopJoin nlj(leftScan, rightScan, status, 0, nullptr);
        ASSERT_EQ(status, 0);
        
        rc = nlj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(nlj.GetAttrCount(), nlj.TupleLength());
        t.SetAttr(nlj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = nlj.GetNext(t);
            if(rc == nlj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            resultCount++;
        }
        
        EXPECT_EQ(29*5, resultCount); // Cross product size
        rc = nlj.Close();
        ASSERT_EQ(rc, 0);
    }

    // Test Case 8: Cross Product Multiple join conditions
    {
        Condition inScanCondition = createScanCondition("in", "bw");
        
        Iterator* leftScan = new IndexScan(smm, rmm, ixm, "in", "bw", status, inScanCondition);
        ASSERT_EQ(status, 0);

        int attrCount = -1;
        DataAttrInfo* starsAttrs;
        smm.GetFromTable("stars", attrCount, starsAttrs);
        
        Condition starsScanCondition;
        starsScanCondition.op = EQ_OP;
        starsScanCondition.lhsAttr.attrName = starsAttrs[0].attrName;
        starsScanCondition.lhsAttr.relName = starsAttrs[0].relName;
        starsScanCondition.rhsValue.data = NULL;
        starsScanCondition.bRhsIsAttr = FALSE;

        Iterator* rightScan = new FileScan(smm, rmm, "stars", status, starsScanCondition);
        ASSERT_EQ(status, 0);

        // Multiple join conditions
        Condition multiJoinConditions[] = {
            createJoinCondition("in", "in", "stars", "soapid", EQ_OP),
            createJoinCondition("in", "in", "stars", "starid", GT_OP)
        };

        NestedLoopJoin nlj(leftScan, rightScan, status, 2, multiJoinConditions);
        ASSERT_EQ(status, 0);
        
        rc = nlj.Open();
        ASSERT_EQ(rc, 0);

        Tuple t(nlj.GetAttrCount(), nlj.TupleLength());
        t.SetAttr(nlj.GetAttr());

        int resultCount = 0;
        while(1) {
            rc = nlj.GetNext(t);
            if(rc == nlj.Eof()) break;
            EXPECT_EQ(rc, 0);
            if(rc != 0) PrintErrorAll(rc);
            resultCount++;
        }
        
        EXPECT_EQ(2, resultCount);
        rc = nlj.Close();
        ASSERT_EQ(rc, 0);
    }

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

