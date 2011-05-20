#include "sm.h"
#include "catalog.h"
#include "gtest/gtest.h"
#include <sstream>

class SM_ManagerTest : public ::testing::Test {
};

TEST_F(SM_ManagerTest, Cons) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
    SM_Manager smm(ixm, rmm);

    {    
      DataRelInfo relcat_rel;
      strcpy(relcat_rel.relName, "relcat");
      relcat_rel.attrCount = DataRelInfo::members();
      relcat_rel.recordSize = DataRelInfo::size();
      relcat_rel.numPages = 1; // initially
      relcat_rel.numRecords = 2; // relcat & attrcat
      
      char buf[2000];
      memset(buf, 0 , 2000);
      
      memcpy(buf, &relcat_rel, DataRelInfo::size());
      
      DataRelInfo * prel = (DataRelInfo*) buf;
      ASSERT_EQ(2, prel->numRecords);
      ASSERT_EQ(1, prel->numPages);
    }

    const char * dbname = "smtfile";
    stringstream comm;
    comm << "rm -rf " << dbname;
    rc = system (comm.str().c_str());

    stringstream command;
    command << "./dbcreate " << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    rc = smm.Print("relcat");
    ASSERT_EQ(rc, 0);
    
    rc = smm.Print("attrcat");
    ASSERT_EQ(rc, 0);

    rc = smm.Help();
    ASSERT_EQ(rc, 0);

    rc = smm.Help("attrcat");
    ASSERT_EQ(rc, 0);

    rc = smm.Help("relcat");
    ASSERT_EQ(rc, 0);

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

TEST_F(SM_ManagerTest, CreateDrop) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
    SM_Manager smm(ixm, rmm);

    const char * dbname = "cdtest";
    stringstream command;
    command << "rm -rf " << dbname;
    rc = system (command.str().c_str());

    command.str("");
    command << "./dbcreate " << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    // no such table
    command.str("");
    command << "echo \"drop table in;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "echo \"create table in(in i, fl f);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "ls  " 
            << dbname << "/in";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    // should fail - no index yet
    command.str("");
    command << "echo \"drop index in(in);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "echo \"create index in(in);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index in(fl);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    // should fail the 2nd time - but redbase always return true
    command.str("");
    command << "echo \"create index in(fl);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "ls  " 
            << dbname << "/in.0";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "ls  " 
            << dbname << "/in.4";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"drop table in;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "ls  " 
            << dbname << "/in.0";
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "ls  " 
            << dbname << "/in.4";
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "ls  " 
            << dbname << "/in";
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "echo \"create table in(in i, out f, bw c2);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index in(out);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load in(\\\"../data\\\");\" | ./redbase " 
            << dbname;
    cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"print in;\" | ./redbase "
            << dbname 
      ;    // cerr << command.str() << endl;
    rc = system (command.str().c_str());


    command.str("");
    command << "echo \"print relcat;\" | ./redbase "
            << dbname 
            << " | grep ^in | perl -nle '@a = split /\\s+/; print $a[4]'";
    // cerr << command.str() << endl;
    rc = system (command.str().c_str());

    command.str("wc -l data | perl -nle '@a = split /\\s+/; print $a[1]'");
    rc = system (command.str().c_str());

    #if 0
    command.str("");
    command << "echo \"load in(\\\"../data.2700\\\");\" | ./redbase " 
            << dbname;
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"print relcat;\" | ./redbase "
            << dbname 
            << " | grep ^in | perl -nle '@a = split /\\s+/; print $a[3]; print $a[4]'";
    // cerr << command.str() << endl;
    rc = system (command.str().c_str());

    command.str("wc -l data.2700 data | grep total | perl -nle '@a = split /\\s+/; print $a[1]'");
    rc = system (command.str().c_str());
    #endif -- too slow

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}

TEST_F(SM_ManagerTest, loadafter_testdir) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
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


    IX_IndexHandle ifh;
    if (chdir(dbname) < 0) {
      cerr << " chdir error to " << dbname 
           << ". Does the db exist ?\n";
    }
    rc = ixm.OpenIndex("in", 8, ifh);
    ASSERT_EQ(rc, 0);

    IX_IndexScan fs;
    
    (rc=fs.OpenScan(ifh, NO_OP, NULL, NO_HINT));
    ASSERT_EQ(rc, 0);

    int ns = 0;
    while(1) {
      RID rid; 
      char * pbuf;
      rc = fs.GetNextEntry((void*&)pbuf, rid, ns);
      if(rc == IX_EOF)
        break;
      EXPECT_EQ(rc, 0);
      char buf[3];
      strncpy(buf, pbuf, 2);
      buf[2] = '\0';
      cerr << buf << "\t" << rid << endl;
    }
    
    EXPECT_EQ(5, ns);
    (rc=fs.CloseScan());
    ASSERT_EQ(rc, 0);
    if (chdir("..") < 0) {
      cerr << " chdir error to ..\n";  
    }    
}

TEST_F(SM_ManagerTest, loadbefore_testdir) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
    SM_Manager smm(ixm, rmm);

    const char * dbname = "test2";
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
    command << "echo \"load in(\\\"../data\\\");\" | ./redbase " 
            << dbname;
    // cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index in(bw);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    // no such table
    command.str("");
    command << "echo \"help in;\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    IX_IndexHandle ifh;
    if (chdir(dbname) < 0) {
      cerr << " chdir error to " << dbname 
           << ". Does the db exist ?\n";
    }
    rc = ixm.OpenIndex("in", 8, ifh);
    ASSERT_EQ(rc, 0);

    IX_IndexScan fs;
    
    (rc=fs.OpenScan(ifh, NO_OP, NULL, NO_HINT));
    ASSERT_EQ(rc, 0);

    int ns = 0;
    while(1) {
      RID rid; 
      char * pbuf;
      rc = fs.GetNextEntry((void*&)pbuf, rid, ns);
      if(rc == IX_EOF)
        break;
      EXPECT_EQ(rc, 0);
      char buf[3];
      strncpy(buf, pbuf, 2);
      buf[2] = '\0';
      // cerr << buf << "\t" << rid << endl;
    }
    
    ASSERT_EQ(5, ns);
    (rc=fs.CloseScan());
    ASSERT_EQ(rc, 0);
    if (chdir("..") < 0) {
      cerr << " chdir error to ..\n";  
    }    
}

TEST_F(SM_ManagerTest, SemCheck) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
    SM_Manager smm(ixm, rmm);

    const char * dbname = "cdtest";
    stringstream command;
    command << "rm -rf " << dbname;
    rc = system (command.str().c_str());

    command.str("");
    command << "./dbcreate " << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table in(in i, fl f, st c4);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table in2(inn i, fl f, stt c4);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load in(\\\"../data\\\");\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    ASSERT_EQ(0, smm.OpenDb(dbname));

    ASSERT_EQ(0, smm.SemCheck("in"));
    ASSERT_NE(0, smm.SemCheck("in0923094820984"));

    RelAttr ra;
    ra.relName = "in";
    ra.attrName = "in";
    ASSERT_EQ(0, smm.SemCheck(ra));
    ra.attrName = "in234234324234";
    ASSERT_NE(0, smm.SemCheck(ra));

    ra.relName = NULL;
    char * rels[2];
    rels[0] = "in";
    rels[1] = "in2";

    ra.attrName = "in";
    EXPECT_EQ(0, smm.FindRelForAttr(ra, 2, rels));
    ASSERT_STREQ("in", ra.relName);

    // ambiguous
    ra.relName = NULL;
    ra.attrName = "fl";
    EXPECT_EQ(SM_AMBGATTR, smm.FindRelForAttr(ra, 2, rels));
    ASSERT_STREQ(NULL, ra.relName);

    // conditions
    Condition cond;
    cond.lhsAttr.relName = "in";
    cond.lhsAttr.attrName = "in";
    cond.rhsAttr.relName = "in2";
    cond.rhsAttr.attrName = "inn";
    cond.op = EQ_OP;
    cond.bRhsIsAttr = TRUE;

    EXPECT_EQ(0, smm.SemCheck(cond));


    cond.lhsAttr.relName = "in";
    cond.lhsAttr.attrName = "in";
    cond.rhsAttr.relName = "in2";
    cond.rhsAttr.attrName = "fl";
    cond.op = EQ_OP;
    cond.bRhsIsAttr = TRUE;

    EXPECT_EQ(SM_TYPEMISMATCH, smm.SemCheck(cond));

    // cond.op = EQ_OP + 34343434;
    // EXPECT_EQ(SM_BADOP, smm.SemCheck(cond));


    ASSERT_EQ(0, smm.CloseDb());
    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
