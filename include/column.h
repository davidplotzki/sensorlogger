#ifndef _COLUMN_H
#define _COLUMN_H

#define E_NO_VALUES_FOR_COLUMN  7002

#include <string>
#include <sstream>
#include <vector>

enum operation {mean, median, max, min, sum, count, freq, freq_min, freq_max, stdDevMean, stdDevMedian};

class logbook;
class sensor;
class counter;

class column
{
private:
	sensor*       _sensor;
	counter*      _pulseCounter;
	std::string   _title;
	std::string   _unit;
	uint64_t      _evaluationPeriod;
	operation     _op;
	double        _confidenceSigma;
	double        _confidenceAbsolute;
	std::string   _mqttPublishTopic;
	std::string   _homematicPublishISE;
	double        _countFactor;

	logbook*      _rootLogbook;

	// Calculated from evaluation period / logbook cycle time:
	size_t        _nCycles;  // How many logbook cycles does the evaluation period represent?

public:
	column(sensor* s, logbook* lb, const std::string &title, const std::string &unit, uint64_t evaluationPeriod, operation op, double confidenceAbsolute, double confidenceSigma, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, double countFactor);

	std::string getTitle() const;
	std::string getUnit() const;
	operation getOperation() const;
	uint64_t getEvaluationPeriod() const;
	double getConfidenceAbsolute() const;
	double getConfidenceSigma() const;
	std::string getMQTTPublishTopic() const;
	std::string getHomematicPublishISE() const;
	double getCountFactor() const;

	counter* getCounter();
	sensor*  getSensor();

	void setTitle(const std::string &title);
	void setUnit(const std::string &unit);
	void setOperation(operation op);
	void setEvaluationPeriod(uint64_t evaluationPeriod);
	void calculateColumnCycles();
	void setConfidenceAbsolute(double confidenceAbsolute);
	void setConfidenceSigma(double confidenceSigma);
	void setMQTTPublishTopic(const std::string &mqttPublishTopic);
	void setHomematicPublishISE(const std::string &homematicPublishISE);
	void setCountFactor(double countFactor);

	std::string getValue(uint64_t startTimestamp, uint64_t currentTimestamp) const;
	void startNewCycle(const uint64_t currentTimestamp);
};


#endif