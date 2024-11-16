#include "Utils.h"

RecursionCounter::RecursionCounter(int* count)
{
	this->count = count;
	(*this->count)++;
}

/*virtual*/ RecursionCounter::~RecursionCounter()
{
	(*this->count)--;
}

int RecursionCounter::GetCount() const
{
	return *this->count;
}