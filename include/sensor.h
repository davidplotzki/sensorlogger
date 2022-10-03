#ifndef _SENSOR_H
#define _SENSOR_H

#include <cstdio>
#include <vector>
#include <string>
#include <sstream>

enum sensor_type {sensor_json, sensor_tinkerforge, sensor_mqtt, sensor_homematic};
enum trigger_event {periodic, high, low, high_or_low, mqttSubscribe};

class measurements;
class counter;
class logger;

class sensor
{
private:
	//void count();
	void count(uint64_t currentTimestamp);
	bool isCounter() const;

protected:
	measurements* _m;  // List of measurements, with value and time stamp
	std::vector<counter*> _counters;  // List of impulse counters, managed mostly by logbook columns.

	std::string   _sensorID;
	bool          _isCounter;
	trigger_event _triggerEvent;
	double        _factor;
	double        _offset;
	uint64_t      _nReadFailures;      // number of read failures
	uint64_t      _minimumRestPeriod;  // in ms
	uint64_t	  _retry_time;
	std::vector<std::string> _jsonKey;
	std::string   _mqttPublishTopic;
	std::string   _homematicPublishISE;
	bool          _lastValuePublished;

	uint64_t      _keepValuesFor_ms;   // defined by the maximum logbook cycle time

	uint64_t      _timestamp_lastMeasurement;

	logger* _root;

public:
	sensor();
	virtual ~sensor();

	virtual sensor_type type() const = 0;

	std::string info() const;

	void clean(uint64_t currentTimestamp);

	void accumulateMaxTimeToKeep(uint64_t timeToKeep);
	counter* newCounter();

	void setPointerToLogger(logger* root);
	void setSensorID(const std::string &sensorID);
	void setCounter(bool c);
	void setTriggerEvent(trigger_event e);
	void setFactor(double factor);
	void setOffset(double offset);
	void setMinimumRestPeriod(uint64_t minRestPeriod);
	void setRetryTime(uint64_t retryTime);
	void setMQTTPublishTopic(const std::string &mqttPublishTopic);
	void setHomematicPublishISE(const std::string &homematicPublishISE);

	void addJSONkey(const std::string &key);
	void setJSONkeys(const std::vector<std::string>* keys);

	size_t nJSONkeys() const;
	const std::string& jsonKey(size_t i) const;

	logger* ptLogger();
	std::string getSensorID() const;
	//
	trigger_event getTriggerEvent() const;
	double getFactor() const;
	double getOffset() const;
	uint64_t getMinimumRestPeriod() const;
	uint64_t getRetryTime() const;
	std::string getMQTTPublishTopic() const;

	uint64_t getReadFailures() const;
	void addReadFailure();
	void resetReadFailures();

	void resetCounters();

	size_t nMeasurements() const;
	bool addRawMeasurement(double value);
	
	std::vector<double>* valuesInConfidence(uint64_t startTimestamp, double absolute, double nSigma) const;

	void reset();
	virtual bool measure(uint64_t currentTimestamp) = 0;
	double getLastValue() const;

	void publishLastEvent();   // to MQTT and HomeMatic
};

#endif