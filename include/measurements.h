#ifndef _MEASUREMENTS_H
#define _MEASUREMENTS_H

#define E_NO_MEASUREMENTS    1000

#include <cstdint>
#include <vector>
#include <cmath>
#include <algorithm>

uint64_t timeDiff(uint64_t timestamp1, uint64_t timestamp2);

class measurements
{
private:
	std::vector<double> _values;
	std::vector<uint64_t> _timestamps;  // microtime in milliseconds

	uint64_t _minimumRestPeriod;  // min. time between two measurements, in ms

public:
	measurements();
	~measurements();

	void setMinimumRestPeriod(uint64_t ms);
	void addValue(double value, uint64_t timestamp);
	void setOnlyValue(double value, uint64_t timestamp);  // Counters only keep the last value.
	void clean(uint64_t earliest_timestamp_to_keep);
	void clear();

	size_t nMeasurements() const;
	double getLastValue() const;

	std::vector<double>* valuesInConfidence(uint64_t startTimestamp, double absolute, double nSigma) const;
};

namespace measurementFunctions
{
	double sum(const std::vector<double>* values);
	double mean(const std::vector<double>* values);
	double median(const std::vector<double>* values);
	double maximum(const std::vector<double>* values);
	double minimum(const std::vector<double>* values);
	double stdDevMean(const std::vector<double>* values);
	double stdDevMedian(const std::vector<double>* values);
}

#endif