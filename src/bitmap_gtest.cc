#include "rm.h"
#include "gtest/gtest.h"

class bitmapTest : public testing::Test {
protected:
	virtual void SetUp() {}
	virtual void TearDown() {}

};

TEST_F(bitmapTest, DefCons) {
	bitmap b(3);
	ASSERT_EQ(b.numChars(), 1);
	bitmap bb(30);
	ASSERT_EQ(bb.numChars(), 4);
	bitmap bbb(3000);
	ASSERT_EQ(bbb.numChars(), 375);
}

TEST_F(bitmapTest, Set3) {
	int s = 3;
	bitmap b(s);
	ASSERT_EQ(b.numChars(), 1);

	for(int i=0; i < s; i++)
	{
		ASSERT_EQ(b.test(i), false);
		b.set(i);
		ASSERT_EQ(b.test(i), true);
		b.reset(i);
		ASSERT_EQ(b.test(i), false);
	}

	for(int i=0; i < s; i++)
	{
		if (i % 2)
			b.set(i);
	}
	char buf[1];
	b.to_char_buf(buf,1);

	bitmap newb(buf, s);
	for(int i=0; i < newb.getSize(); i++)
	{
		if (i % 2)
			ASSERT_EQ(newb.test(i), true);
		else
			ASSERT_EQ(newb.test(i), false);
	}
}

TEST_F(bitmapTest, Set16) {
	int s = 16;
	bitmap b(s);
	ASSERT_EQ(b.numChars(), 2);

	for(int i=0; i < s; i++)
	{
		ASSERT_EQ(b.test(i), false);
		b.set(i);
		ASSERT_EQ(b.test(i), true);
		b.reset(i);
		ASSERT_EQ(b.test(i), false);
	}

	for(int i=0; i < s; i++)
	{
		if (i % 2)
			b.set(i);
	}
	char buf[2];
	b.to_char_buf(buf,2);
	// std::cerr << b << std::endl;

	bitmap newb(buf, s);
	// std::cerr << newb << std::endl;
	for(int i=0; i < newb.getSize(); i++)
	{
		if (i % 2)
			ASSERT_EQ(newb.test(i), true);
		else
			ASSERT_EQ(newb.test(i), false);
	}

}
