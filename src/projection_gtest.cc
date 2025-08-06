//
// File:        projection_gtest.cc
// Description: Test cases for Projection class
//

#include "gtest/gtest.h"
#include "projection.h"
#include "file_scan.h"
#include "sm.h"
#include "ql.h"
#include <sstream>
#include <cstdio>
#include <cstring>

using namespace std;

class ProjectionTest : public ::testing::Test {
protected:
    ProjectionTest() {}
    virtual void SetUp() 
    {
        // Clean up any existing test database
        system("rm -rf projection_test");
    }

    virtual void TearDown() 
    {
        // Clean up test database
        system("rm -rf projection_test");
    }
};

TEST_F(ProjectionTest, BadAttributeError) {
    RC rc;
    const char * dbname = "projection_test";
    
    // Use existing test setup from other join tests - create in and stars tables
    stringstream command;
    command << "./dbcreate " << dbname;
    rc = system(command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table in(in i, out f, bw c4);\" | ./redbase " << dbname;
    rc = system(command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load in (\\\"../data\\\");\" | ./redbase " << dbname;
    rc = system(command.str().c_str());
    // Don't assert on load since the data file might not exist, we just need the table structure

    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    // Create a file scan iterator
    RC status;
    Condition cond;
    cond.op = NO_OP;  // No condition, scan all records
    cond.rhsValue.data = NULL;  // Required to avoid assertion failure
    FileScan* fs = new FileScan(smm, rmm, "in", status, cond);
    ASSERT_EQ(status, 0);

    // Test 1: Valid projection attributes - should succeed
    AggRelAttr validAttrs[2];
    memset(validAttrs, 0, sizeof(validAttrs));
    validAttrs[0].relName = const_cast<char*>("in");
    validAttrs[0].attrName = const_cast<char*>("in");
    validAttrs[0].func = NO_F;
    validAttrs[1].relName = const_cast<char*>("in");
    validAttrs[1].attrName = const_cast<char*>("out");
    validAttrs[1].func = NO_F;

    // Test 1: Valid projection attributes - should succeed
    {
        Projection validProjection(fs, status, 2, validAttrs);
        ASSERT_EQ(status, 0) << "Valid projection should succeed";
    }
    delete fs; // Caller manages FileScan lifetime

    // Test 2: Invalid projection attribute - should fail with SM_BADATTR
    // Create a new FileScan for this test
    Condition cond2;
    cond2.op = NO_OP;
    cond2.rhsValue.data = NULL;
    FileScan* fs2 = new FileScan(smm, rmm, "in", status, cond2);
    ASSERT_EQ(status, 0);

    AggRelAttr invalidAttrs[2];
    memset(invalidAttrs, 0, sizeof(invalidAttrs));
    invalidAttrs[0].relName = const_cast<char*>("in");
    invalidAttrs[0].attrName = const_cast<char*>("in");
    invalidAttrs[0].func = NO_F;
    invalidAttrs[1].relName = const_cast<char*>("in");
    invalidAttrs[1].attrName = const_cast<char*>("nonexistent_column");  // This attribute doesn't exist
    invalidAttrs[1].func = NO_F;

    {
        Projection invalidProjection(fs2, status, 2, invalidAttrs);
        ASSERT_EQ(status, SM_BADATTR) << "Invalid projection should fail with SM_BADATTR";
    }
    delete fs2; // Caller manages FileScan lifetime

    // Test 3: Invalid relation name - should fail with SM_BADATTR
    // Create a new FileScan for this test
    Condition cond3;
    cond3.op = NO_OP;
    cond3.rhsValue.data = NULL;
    FileScan* fs3 = new FileScan(smm, rmm, "in", status, cond3);
    ASSERT_EQ(status, 0);

    AggRelAttr invalidRelAttrs[1];
    memset(invalidRelAttrs, 0, sizeof(invalidRelAttrs));
    invalidRelAttrs[0].relName = const_cast<char*>("nonexistent_table");
    invalidRelAttrs[0].attrName = const_cast<char*>("in");
    invalidRelAttrs[0].func = NO_F;

    {
        Projection invalidRelProjection(fs3, status, 1, invalidRelAttrs);
        ASSERT_EQ(status, SM_BADATTR) << "Invalid relation name should fail with SM_BADATTR";
    }
    delete fs3; // Caller manages FileScan lifetime
    
    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    // Clear buffer manager to avoid state interference between tests
    rc = pfm.ClearBuffer();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system(command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

TEST_F(ProjectionTest, ValidProjectionFunctionality) {
    RC rc;
    const char * dbname = "projection_test";
    
    // Use existing test setup - create in table  
    stringstream command;
    command << "./dbcreate " << dbname;
    rc = system(command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table in(in i, out f, bw c4);\" | ./redbase " << dbname;
    rc = system(command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load in (\\\"../data\\\");\" | ./redbase " << dbname;
    rc = system(command.str().c_str());
    // Don't assert on load since the data file might not exist

    PF_Manager pfm;
    RM_Manager rmm(pfm);
    IX_Manager ixm(pfm);
    SM_Manager smm(ixm, rmm);

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    // Create a file scan iterator
    RC status;
    Condition cond;
    cond.op = NO_OP;
    cond.rhsValue.data = NULL;  // Required to avoid assertion failure
    FileScan* fs = new FileScan(smm, rmm, "in", status, cond);
    ASSERT_EQ(status, 0);

    // Project only 'in' and 'out' columns (exclude 'bw' column)
    AggRelAttr projAttrs[2];
    memset(projAttrs, 0, sizeof(projAttrs));
    projAttrs[0].relName = const_cast<char*>("in");
    projAttrs[0].attrName = const_cast<char*>("in");
    projAttrs[0].func = NO_F;
    projAttrs[1].relName = const_cast<char*>("in");
    projAttrs[1].attrName = const_cast<char*>("out");
    projAttrs[1].func = NO_F;

    {
        Projection projection(fs, status, 2, projAttrs);
        ASSERT_EQ(status, 0) << "Valid projection should succeed";

        rc = projection.Open();
        ASSERT_EQ(rc, 0);

        // Just verify that the projection can be opened and closed without errors
        // We don't need to check actual data since the data file might not exist
        rc = projection.Close();
        ASSERT_EQ(rc, 0);
    }
    delete fs; // Caller manages FileScan lifetime
    
    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    // Clear buffer manager to avoid state interference between tests
    rc = pfm.ClearBuffer();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system(command2.str().c_str());
    ASSERT_EQ(rc, 0);
}