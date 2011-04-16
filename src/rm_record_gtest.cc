#include "rm_rid.h"
#include "rm.h"
#include "gtest/gtest.h"

#define STRLEN 29
struct TestRec {
    char  str[STRLEN];
    int   num;
    float r;
};

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
	ASSERT_TRUE(getRecordSize(&r) == -1);
}

TEST_F(RM_RecordTest, SetData) {
	TestRec rec;
	memcpy(rec.str, "ll", STRLEN);
	rec.num = 13;
	rec.r = 1.3;
	RID id(10, 10);
	r.Set((char*)&rec, sizeof(rec), id);

	TestRec * pBuf;
	r.GetData((char*&)pBuf);
	ASSERT_EQ(pBuf->r, rec.r);
	ASSERT_STREQ(pBuf->str, rec.str);
	ASSERT_EQ(pBuf->num, rec.num);
	
	RID t;
	r.GetRid(t);
	ASSERT_TRUE(id == t);
}

TEST_F(RM_RecordTest, RM_PageHdr) {
	RM_PageHdr h(4);
	bitmap b(4);
	b.set();
	b.to_char_buf(h.freeSlotMap, b.numChars());

	char buf[40];
	char * pbuf = buf;
	h.to_buf(pbuf);

	RM_PageHdr h2(4);
	h2.from_buf(pbuf);
	bitmap b2(h2.freeSlotMap, 4);
	std::cerr << b2 << std::endl;
}
