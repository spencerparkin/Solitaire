#pragma once

#include <stdint.h>

class Clock
{
public:
	Clock();
	virtual ~Clock();

	bool NeverBeenReset() const;
	void Reset();

	double GetCurrentTimeMilliseconds(bool reset = false);
	double GetCurrentTimeSeconds(bool reset = false);

private:
	uint64_t GetCurrentSystemTime() const;
	uint64_t GetElapsedTime(bool reset = false);

	uint64_t timeBase;
};