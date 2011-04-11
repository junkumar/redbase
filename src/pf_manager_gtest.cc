#include "pf_internal.h"
#include "pf.h"
#include "gtest/gtest.h"


class PF_ManagerTest : public ::testing::Test {
protected:
	virtual void SetUp() 
	{
		RC rc;
		system("rm -f pftestfile");
		if(
			(rc = pfm.CreateFile("pftestfile")) 
			 || (rc =	pfm.OpenFile("pftestfile", fh))
			)
			PF_PrintError(rc);
	}

	virtual void TearDown() 
  {
		pfm.DestroyFile("pftestfile");
	}

  // Declares the variables your tests want to use.
	PF_Manager pfm;
	PF_FileHandle fh;
};

// Setup will call both constructor and Open()

TEST_F(PF_ManagerTest, Close) {
	pfm.CloseFile(fh);
}

TEST_F(PF_ManagerTest, Header) {
	pfm.OpenFile("pftestfile", fh);
	
	PF_PageHandle headerPage;
	char * pData;
	
	fh.AllocatePage(headerPage);
	headerPage.GetData(pData);
	const char * buf = "wwww";
	memcpy(pData, buf, 4);
	PageNum headerPageNum;
	headerPage.GetPageNum(headerPageNum);

	fh.MarkDirty(headerPageNum);
	fh.UnpinPage(headerPageNum);

	pfm.CloseFile(fh);

	pfm.OpenFile("pftestfile", fh);
	PF_PageHandle ph;
	fh.GetThisPage(0, ph);
	pfm.CloseFile(fh);

}

// TEST_F(PF_ManagerTest, Persist) {
// 	RC rc;
// 	PF_Handle fh2;

// 	system("rm gtestfile2");
// 	if(
// 		(rc = rmm.CreateFile("gtestfile2", sizeof(TestRec))) < 0
// 		|| (rc =	rmm.OpenFile("gtestfile2", fh2)) < 0
// //		|| (rc =	rmm.CloseFile(fh2)) < 0
// //		|| (rc =	rmm.OpenFile("gtestfile2", fh2)) < 0
// 		)
// 		RM_PrintError(rc);

// 	ASSERT_EQ(fh.GetFullRecordSize(), 48);
// 	ASSERT_EQ(fh.GetNumPages(), 1);
// }

