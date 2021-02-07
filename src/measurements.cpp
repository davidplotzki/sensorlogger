#include "measurements.h"
#include "sensorlogger.h"

uint64_t timeDiff(uint64_t timestamp1, uint64_t timestamp2)
{
	if(timestamp1 > timestamp2)
		return timestamp1 - timestamp2;
	else
		return timestamp2 - timestamp1;
}


measurements::measurements()
{
	_minimumRestPeriod = 0;
}

measurements::~measurements()
{
	clear();
}

void measurements::setMinimumRestPeriod(uint64_t ms)
{
	_minimumRestPeriod = ms;
}

void measurements::addValue(double value, uint64_t timestamp)
{
	// Obey minimum rest time:
	uint64_t lastTimeStamp = 0;
	if(_timestamps.size() > 0)
		lastTimeStamp = _timestamps.at(_timestamps.size()-1);

	if(timeDiff(timestamp, lastTimeStamp) >= _minimumRestPeriod)
	{
		_values.push_back(value);
		_timestamps.push_back(timestamp);
	}

	// Keep only up to MAX_MEASUREMENTS values in memory to avoid overflow:
	while(_values.size() > MAX_MEASUREMENTS)
	{
		_values.erase(_values.begin());
		_timestamps.erase(_timestamps.begin());
	}
}

void measurements::setOnlyValue(double value, uint64_t timestamp)
{
	if(_values.size() > 1)
	{
		clear();
	}

	if(_values.size() == 0)
	{
		_values.push_back(value);
		_timestamps.push_back(timestamp);
	}
	else if(_values.size() == 1)
	{
		_values.at(0) = value;
		_timestamps.at(0) = timestamp;
	}
}

void measurements::clean(uint64_t earliest_timestamp_to_keep)
{
	for(int i=0; i<static_cast<int>(_values.size()); ++i)
	{
		if(_timestamps.at(i) < earliest_timestamp_to_keep)
		{
			_values.erase(_values.begin()+i);
			_timestamps.erase(_timestamps.begin()+i);
			--i;
		}
	}
}

void measurements::clear()
{
	_values.clear();
	_timestamps.clear();
}

size_t measurements::nMeasurements() const
{
	return _values.size();
}

double measurements::getLastValue() const
{
	if(_values.size() > 0)
	{
		return _values.at(_values.size()-1);
	}

	throw E_NO_MEASUREMENTS;
}

std::vector<double>* measurements::valuesInConfidence(uint64_t startTimestamp, double absolute, double nSigma) const
{
	// Create a new vector with everything that is in time range:
	std::vector<double>* valuesToKeep = new std::vector<double>();
	for(size_t i=0; i<_values.size(); ++i)
	{
		if(_timestamps.at(i) > startTimestamp)  // (startTimestamp, newest]
		{
			valuesToKeep->push_back(_values.at(i));
		}
	}

	absolute = abs(absolute);

	// Delete any values that are not in the given sigma range:
	if((absolute > 0) || (nSigma > 0))
	{
		double m = measurementFunctions::median(valuesToKeep);
		double lowerLimit;
		double upperLimit;

		if(nSigma > 0)
		{
			double sigma = measurementFunctions::stdDevMedian(valuesToKeep);
			absolute = nSigma * sigma;
		}

		lowerLimit = m - absolute;
		upperLimit = m + absolute;

		for(int i=0; i<static_cast<int>(valuesToKeep->size()); ++i)
		{
			if((valuesToKeep->at(i) < lowerLimit) || (valuesToKeep->at(i) > upperLimit))
			{
				valuesToKeep->erase(valuesToKeep->begin()+i);
				--i;
			}
		}

		if(valuesToKeep->size() == 0)
		{
			// The confidence sigma was probably too high, and a calculation artifact removed all values.
			// Let's assume that the median value now represents the whole distribution.
			valuesToKeep->push_back(m);
		}
	}

	return valuesToKeep;
}

double measurementFunctions::sum(const std::vector<double>* values)
{
	if(values->size() > 0)
	{
		double s = 0;
		for(size_t i=0; i<values->size(); ++i)
			s += values->at(i);

		return s;
	}
	
	return 0;
}

double measurementFunctions::mean(const std::vector<double>* values)
{
	if(values->size() > 0)
	{	
		return measurementFunctions::sum(values) / static_cast<double>(values->size());
	}
	else
	{
		throw E_NO_MEASUREMENTS;
	}
}

double measurementFunctions::median(const std::vector<double>* values)
{
	if(values->size() > 0)
	{	
		std::vector<double> sortedValues = *values;
		std::sort(sortedValues.begin(), sortedValues.end());

		if((sortedValues.size() % 2) == 0)  // even number of values
		{
			size_t pos1 = sortedValues.size() / 2;
			size_t pos0 = pos1 - 1;

			double mean = (sortedValues.at(pos0) + sortedValues.at(pos1)) / 2.0;
			return mean;
		}
		else
		{
			return sortedValues.at(sortedValues.size() / 2);
		}
	}
	else
	{
		throw E_NO_MEASUREMENTS;
	}
}

double measurementFunctions::maximum(const std::vector<double>* values)
{
	if(values->size() > 0)
	{	
		return *std::max_element(values->begin(), values->end());
	}
	else
	{
		throw E_NO_MEASUREMENTS;
	}
}

double measurementFunctions::minimum(const std::vector<double>* values)
{
	if(values->size() > 0)
	{	
		return *std::min_element(values->begin(), values->end());
	}
	else
	{
		throw E_NO_MEASUREMENTS;
	}
}

double measurementFunctions::stdDevMean(const std::vector<double>* values)
{
	// Standard deviation around mean value
	if(values->size() > 1)
	{
		double m = measurementFunctions::mean(values);
		double sum = 0;
		for(size_t i=0; i<values->size(); ++i)
			sum += pow(values->at(i) - m, 2.0);

		sum /= static_cast<double>(values->size());
		return sqrt(sum);
	}

	return 0;
}

double measurementFunctions::stdDevMedian(const std::vector<double>* values)
{
	// Standard deviation around median value
	if(values->size() > 1)
	{
		double m = measurementFunctions::median(values);
		double sum = 0;
		for(size_t i=0; i<values->size(); ++i)
			sum += pow(values->at(i) - m, 2.0);
	
		sum /= static_cast<double>(values->size());
		return sqrt(sum);
	}

	return 0;
}