#include "rm.h"
#include <cmath>
#include <cstring>
#include <cassert>

bitmap::bitmap(int numBits): size(numBits)
{
	buffer = new char[this->numChars()];
	this->reset();
}

bitmap::bitmap(char * buf, int numBits): size(numBits)
{
	buffer = new char[this->numChars()];
	memcpy(buffer, buf, this->numChars());
}

int bitmap::to_char_buf(char * b, int len) const //copy content to char buffer -
{
	assert(b != NULL && len == this->numChars());
	memcpy((void*)b, buffer, len);
	return 0;
}

bitmap::~bitmap()
{
	delete [] buffer;
}

int bitmap::numChars() const
{
	int numChars = (size / 8);
	if((size % 8) != 0)
		numChars++;
	return numChars;
}

bool bitmap::reset()
{
  // faster - return memset((void*)buffer, 0, this->numChars());
	for( int i = 0; i < size; i++) {
		bitmap::reset(i);
	}
}

bool bitmap::reset(unsigned int bitNumber)
{
	assert(bitNumber <= (size - 1));
	int byte = bitNumber/8;
	int offset = bitNumber%8;
	
	buffer[byte] &= ~(1 << offset);
	return true;
}

bool bitmap::set(unsigned int bitNumber)
{
	assert(bitNumber <= size - 1);
	int byte = bitNumber/8;
	int offset = bitNumber%8;

	buffer[byte] |= (1 << offset);
	return true;
}

bool bitmap::set()
{
	for( int i = 0; i < size; i++) {
		bitmap::set(i);
	}
}

// bool bitmap::flip(unsigned int bitNumber)
// {
// 	assert(bitNumber <= size - 1);
// 	int byte = bitNumber/8;
// 	int offset = bitNumber%8;

// 	buffer[byte] ^= (1 << offset);
// 	return true;
// }

bool bitmap::test(unsigned int bitNumber) const
{
	assert(bitNumber <= size - 1);
	int byte = bitNumber/8;
	int offset = bitNumber%8;

	return buffer[byte] & (1 << offset);
}


ostream& operator <<(ostream & os, const bitmap& b)
{
  os << "[";
	for(int i=0; i < b.getSize(); i++)
	{
		if( i % 8 == 0 && i != 0 )
			os << ".";
		os << b.test(i) ? 1 : 0;
	}
	os << "]";
  return os;
}
