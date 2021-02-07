#include "counter.h"
#include "measurements.h"

cycleCounter::cycleCounter(const uint64_t currentTimestamp)
{
	reset(currentTimestamp);
}

cycleCounter::~cycleCounter()
{

}

uint64_t cycleCounter::getStartTimestamp() const
{
	return _timestamp_countsSince;
}

void cycleCounter::count(const uint64_t currentTimestamp)
{
	uint64_t timeDistance = timeDiff(currentTimestamp, _timestamp_lastCount);

	++_counts;
	_timestamp_lastCount = currentTimestamp;

	if((_maxTimeDistance == 0) || (timeDistance > _maxTimeDistance))
	{
		_maxTimeDistance = timeDistance;
	}

	if((_minTimeDistance == 0) || (timeDistance < _minTimeDistance))
	{
		_minTimeDistance = timeDistance;
	}
}

void cycleCounter::finish(const uint64_t currentTimestamp)
{
	_timestamp_finished = currentTimestamp;
}

void cycleCounter::reset(const uint64_t currentTimestamp)
{
	_timestamp_lastCount = 0;
	_timestamp_countsSince = currentTimestamp;
	_counts = 0;

	_minTimeDistance = 0;
	_maxTimeDistance = 0;
}

uint64_t cycleCounter::counts() const
{
	return _counts;
}

double cycleCounter::frequency() const
{
	if(_counts > 0)
	{
		uint64_t periodicLength = timeDiff(_timestamp_countsSince, _timestamp_finished) / 1000;   // in s

		if(periodicLength > 0)
		{
			return static_cast<double>(_counts) / static_cast<double>(periodicLength);
		}
	}

	return 0;
}

double cycleCounter::frequency_min() const
{
	if(_maxTimeDistance > 0)
	{
		return 1000.0 / static_cast<double>(_maxTimeDistance);
	}

	return 0;
}

double cycleCounter::frequency_max() const
{
	if(_minTimeDistance > 0)
	{
		return 1000.0 / static_cast<double>(_minTimeDistance);
	}

	return 0;
}



counter::counter(const uint64_t currentTimestamp)
{
	_nCyclesToStore = 1;
	reset(currentTimestamp);
}

counter::~counter()
{
	for(size_t i=0; i<_counters.size(); ++i)
	{
		delete _counters.at(i);
	}

	_counters.clear();
}

void counter::count(const uint64_t currentTimestamp)
{
	_currentCounter->count(currentTimestamp);
}

void counter::accumulateCyclesToStore(size_t cyclesToStore)
{
	if(_nCyclesToStore > 0) // otherwise, store counts infinitely
	{
		if(cyclesToStore == 0)  // store infinitely
		{
			_nCyclesToStore = cyclesToStore;
			return;
		}

		if(cyclesToStore > _nCyclesToStore)
		{
			_nCyclesToStore = cyclesToStore;
		}
	}
}

void counter::startNewCycle(const uint64_t currentTimestamp)
{
	if(_nCyclesToStore > 0)  // otherwise never reset the current counter
	{
		if(currentTimestamp > _currentCounter->getStartTimestamp())  // only if new cycle will be younger than current cycle
		{
			_currentCounter->finish(currentTimestamp);
			_currentCounter = new cycleCounter(currentTimestamp);

			// Delete old counters:
			while(_counters.size() >= _nCyclesToStore)
			{
				delete _counters.at(0);
				_counters.erase(_counters.begin());
			}

			_counters.push_back(_currentCounter);
		}
	}
}

void counter::reset(const uint64_t currentTimestamp)
{
	for(size_t i=0; i<_counters.size(); ++i)
	{
		delete _counters.at(i);
	}

	_counters.clear();
	_currentCounter = new cycleCounter(currentTimestamp);
	_counters.push_back(_currentCounter);
}

uint64_t counter::counts(size_t nCycles) const
{
	if(nCycles == 1)
	{
		return _currentCounter->counts();
	}
	else if(_counters.size() > 0)
	{
		if(nCycles == 0)  // get result for all stored counters
			nCycles = _counters.size();

		size_t i = _counters.size();
		uint64_t sum = 0;

		do
		{
			--i;
			--nCycles;
			sum += _counters.at(i)->counts();
		} while(i>0 && nCycles>0);

		return sum;
	}

	return 0;
}

double counter::frequency(size_t nCycles, const uint64_t currentTimestamp) const
{
	_currentCounter->finish(currentTimestamp);

	if(nCycles == 1)
	{
		return _currentCounter->frequency();
	}
	else if(_counters.size() > 0)
	{
		if(nCycles == 0)  // get result for all stored counters
			nCycles = _counters.size();

		size_t i = _counters.size();
		uint64_t sum = 0;
		uint64_t startTime = 0;
		do
		{
			--i;
			--nCycles;
			sum += _counters.at(i)->counts();
			startTime = _counters.at(i)->getStartTimestamp();
		} while(i>0 && nCycles>0);

		uint64_t periodicLength = timeDiff(startTime, currentTimestamp) / 1000;   // in s
		return static_cast<double>(sum) / static_cast<double>(periodicLength);
	}

	return 0;
}

double counter::frequency_min(size_t nCycles) const
{
	if(nCycles == 1)
	{
		return _currentCounter->frequency_min();
	}
	else if(_counters.size() > 0)
	{
		if(nCycles == 0)  // get result for all stored counters
			nCycles = _counters.size();

		size_t i = _counters.size();
		double min = _currentCounter->frequency_min();

		do
		{
			--i;
			--nCycles;
			if(min > _counters.at(i)->frequency_min())
			{
				min = _counters.at(i)->frequency_min();
			}
		} while(i>0 && nCycles>0);

		return min;
	}

	return 0;
}

double counter::frequency_max(size_t nCycles) const
{
	if(nCycles == 1)
	{
		return _currentCounter->frequency_max();
	}
	else if(_counters.size() > 0)
	{
		if(nCycles == 0)  // get result for all stored counters
			nCycles = _counters.size();

		size_t i = _counters.size();
		double max = _currentCounter->frequency_max();

		do
		{
			--i;
			--nCycles;
			if(max < _counters.at(i)->frequency_max())
			{
				max = _counters.at(i)->frequency_max();
			}
		} while(i>0 && nCycles>0);

		return max;
	}

	return 0;
}