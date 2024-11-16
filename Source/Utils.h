#pragma once

class RecursionCounter
{
public:
	RecursionCounter(int* count);
	virtual ~RecursionCounter();

	int GetCount() const;

private:
	int* count;
};