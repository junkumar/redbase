#include "rm_rid.h"
#include "rm.h"
#include "gtest/gtest.h"

class RM_RecordTest : public testing::Test {
protected:
	virtual void SetUp() {}
	virtual void TearDown() {}

	int getRecordSize(RM_Record * pr) const {
		return pr->recordSize;
	}
  // Declares the variables your tests want to use.
	RM_Record r;
};

TEST_F(RM_RecordTest, DefCons) {
	ASSERT_TRUE(getRecordSize(&r) == 0);
}
