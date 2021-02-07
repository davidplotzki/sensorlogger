#ifndef _SENSOR_HOMEMATIC_H
#define _SENSOR_HOMEMATIC_H

#include "sensor.h"

class homematic;

class sensorHomematic : public sensor
{
private:
	std::string _iseID;
	homematic* _homematic;

public:
	sensorHomematic(logger* root, homematic* hmPtr, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, const std::string &homematicSubscribeISE, bool isCounter, double factor, double offset, uint64_t minimumRestPeriod);

	sensor_type type() const;
	std::string getISE();
	void setISE(const std::string &ise);

	bool measure(uint64_t currentTimestamp);
};

#endif