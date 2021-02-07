#ifndef _SENSOR_JSON_H
#define _SENSOR_JSON_H

#include "sensor.h"

class readoutBuffer;

class sensorJSON : public sensor
{
private:
	readoutBuffer* _rBuffer;  // File buffer, to avoid reading a JSON file multiple times.
	std::string _jsonFilename;

public:
	sensorJSON(logger* root, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, const std::string &jsonFile, const std::vector<std::string>* jsonKeys, bool isCounter, double factor, double offset, uint64_t minimumRestPeriod, readoutBuffer* buffer);

	sensor_type type() const;

	void setJSONfilename(const std::string &jsonFilename);
	bool measure(uint64_t currentTimestamp);
};

#endif