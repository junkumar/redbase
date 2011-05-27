#include "sm.h"
#include "catalog.h"
#include "gtest/gtest.h"
#include <sstream>

class QL_ManagerTest : public ::testing::Test {
};

TEST_F(QL_ManagerTest, Cons) {
    RC rc;
    PF_Manager pfm;
    IX_Manager ixm(pfm);
    RM_Manager rmm(pfm);
    SM_Manager smm(ixm, rmm);


    const char * dbname = "qlfile";
    stringstream comm;
    comm << "rm -rf " << dbname;
    rc = system (comm.str().c_str());

    stringstream command;
    command << "./dbcreate " << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    command.str("");
    command << "echo \"create table soaps(soapid  i, sname  c28, network  c4, rating  f);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create table stars(starid  i, stname  c20, plays  c12, soapid  i);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"CREATE TABLE networks(nid  i, name c4, viewers i);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index soaps(soapid);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"create index stars(soapid);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    // wrong number of attrs
    command.str("");
    command << "echo \"insert into soaps values(\\\"The Good Wife\\\", \\\"CBS\\\", 4.0);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "echo \"insert into soaps values(133, \\\"The Good Wife\\\", \\\"CBS\\\", 4.0);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    // float 4 vs int 4 - error
    command.str("");
    command << "echo \"insert into soaps values(133, \\\"The Good Wife\\\", \\\"CBS\\\", 4);\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_NE(rc, 0);

    command.str("");
    command << "echo \"print soaps;\" | ./redbase "
            << dbname << " | ./counter.pl " ;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 1);

    command.str("");
    command << "echo \"load soaps(\\\"../soaps.data\\\");\" | ./redbase " 
            << dbname;
    cerr << command.str();
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);

    command.str("");
    command << "echo \"load stars(\\\"../stars.data\\\");\" | ./redbase " 
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
    command << "echo \"print soaps;\" | ./redbase "
            << dbname << " | ./counter.pl " ;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 10);

    command.str("");
    command << "echo \"print stars;\" | ./redbase "
            << dbname << " | ./counter.pl " ;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 29);

    command.str("");
    command << "echo \"print networks;\" | ./redbase "
            << dbname << " | ./counter.pl " ;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 4);

    command.str("");
    command << "echo \"queryplans on;reset io; select * from stars, soaps where soaps.soapid = stars.soapid; print io;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 29);

    command.str("");
    command << "echo \"queryplans on;reset io; select * from soaps, stars; print io;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 290-256); //actual 290 but shown as 290-256 - system

    command.str("");
    command << "echo \"queryplans on;select * from stars, networks, soaps where soaps.soapid = stars.soapid and soaps.network = networks.name;\" | ./redbase " 
            << dbname << " | ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 29);


    command.str("");
    command << "echo \"queryplans on;update soaps set rating = 4.1 where soapid = 133;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 1);

    command.str("");
    command << "echo \"queryplans on;update soaps set soapid = 132 where soapid = 133;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 1);

    command.str("");
    command << "echo \"queryplans on;update soaps set soapid = 133 where soapid = 132;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 1);

    command.str("");
    command << "echo \"queryplans on;delete from soaps where soapid = 133;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 1);
    
    command.str("");
    command << "echo \"queryplans on;delete from soaps where network = \\\"CBS\\\" and rating > 1.0;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 3);

    command.str("");
    command << "echo \"queryplans on;delete from soaps where soapid > 3 and rating > 1.0;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 3);

    command.str("");
    command << "echo \"print soaps;\" | ./redbase "
            << dbname << " | ./counter.pl " ;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 3);

    command.str("");
    command << "echo \"queryplans on;update soaps set rating = 1.111;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 3);

    command.str("");
    command << "echo \"queryplans on;delete from soaps;\" | ./redbase " 
            << dbname << "| ./counter.pl ";
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 3);

    command.str("");
    command << "echo \"print soaps;\" | ./redbase "
            << dbname << " | ./counter.pl " ;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc >> 8, 0);

    command.str("");
    command << "echo \"queryplans on;delete from soaps where sname = \\\"The Good Wife\\\";\" | ./redbase " 
            << dbname;
    rc = system (command.str().c_str());
    ASSERT_EQ(rc, 0);


    rc = smm.OpenDb(dbname);
    ASSERT_EQ(rc, 0);

    rc = smm.Print("soaps");
    ASSERT_EQ(rc, 0);
    
    rc = smm.Print("relcat");
    ASSERT_EQ(rc, 0);

    rc = smm.CloseDb();
    ASSERT_EQ(rc, 0);

    stringstream command2;
    command2 << "./dbdestroy " << dbname;
    rc = system (command2.str().c_str());
    ASSERT_EQ(rc, 0);
}
