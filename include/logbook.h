#ifndef _LOG_H
#define _LOG_H

#include <cstdio>
#include <vector>
#include <string>
#include <iostream>
#include <ctime>
#include <chrono>
#include <iomanip>
#include <sstream>
#include <fstream>

class sensor;
class counter;
class mqttBroker;
class homematic;
class logbook;
class logger;
class column;

#define E_COLUMN_DOES_NOT_EXIST 5001

class logbook
{
private:
	std::string          _filename;
	uint64_t             _cycleTime;  // in ms
	unsigned             _maxEntries;
	std::vector<column*> _cols;
	logger*              _root;
	std::string          _missingDataToken;

	uint64_t _timestamp_next_logentry;
	uint64_t _timestamp_last_logentry_written;

public:
	logbook(logger* root, const std::string &filename, uint64_t cycleTime, unsigned maxEntries, const std::string &missingDataToken);
	~logbook();

	std::string getFilename() const;
	uint64_t getCycleTime() const;
	unsigned getMaxEntries() const;
	column* getCol(size_t pos);
	counter* getSharedCounterForSensor(sensor* s);

	void setFilename(const std::string &filename);
	void setCycleTime(uint64_t cycleTime);
	void setMaxEntries(unsigned maxEntries);
	void setMissingDataToken(const std::string &missingDataToken);
	void addColumn(column* col);

	void setTimestampForNextLogEntry();

	// Writes to file and possibly publishes column values to MQTT broker.
	void write(mqttBroker* broker, homematic* hm);
};

#endif