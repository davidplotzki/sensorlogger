#ifndef _COUNTER_H
#define _COUNTER_H

#include <cstdint>
#include <cstdio>
#include <vector>

/* Counter for one logbook cycle */
class cycleCounter
{
private:
	uint64_t _counts;
	uint64_t _timestamp_countsSince;
	uint64_t _timestamp_finished;
	uint64_t _minTimeDistance;
	uint64_t _maxTimeDistance;
	uint64_t _timestamp_lastCount;

public:
	cycleCounter(const uint64_t currentTimestamp);
	~cycleCounter();

	uint64_t getStartTimestamp() const;

	void count(const uint64_t currentTimestamp);
	void finish(const uint64_t currentTimestamp);
	void reset(const uint64_t currentTimestamp);

	uint64_t counts() const;
	double frequency() const;
	double frequency_min() const;
	double frequency_max() const;

};

/* Counter manager class; enables floating counters. */
class counter
{
private:
	size_t _nCyclesToStore;

	std::vector<cycleCounter*> _counters;
	cycleCounter* _currentCounter;

public:
	counter(const uint64_t currentTimestamp);
	~counter();

	void accumulateCyclesToStore(size_t cyclesToStore);
	void count(const uint64_t currentTimestamp);
	void startNewCycle(const uint64_t currentTimestamp);
	void reset(const uint64_t currentTimestamp);

	uint64_t counts(size_t nCycles) const;
	double frequency(size_t nCycles, const uint64_t currentTimestamp) const;
	double frequency_min(size_t nCycles) const;
	double frequency_max(size_t nCycles) const;
};

#endif