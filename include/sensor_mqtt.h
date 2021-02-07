#ifndef _SENSOR_MQTT_H
#define _SENSOR_MQTT_H

#include "mqtt/async_client.h"
#include "sensor.h"

class sensorMQTT : public sensor, public virtual mqtt::callback, public virtual mqtt::iaction_listener
{
private:
	std::string _mqttSubscribeTopic;

	// The setter is private because it must check for feedback loops,
	// if the sensor publishes under the same topic.
	void setMQTTSubscribeTopic(const std::string &mqttSubscribeTopic);

	void on_failure(const mqtt::token& tok) override;
	void on_success(const mqtt::token& tok) override;

public:
	sensorMQTT(logger* root, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, const std::string &mqttSubscribeTopic, const std::vector<std::string>* jsonKeys, bool isCounter, double factor, double offset, uint64_t minimumRestPeriod);

	sensor_type type() const;
	std::string getMQTTSubscribeTopic() const;

	bool measure(uint64_t currentTimestamp);
};

#endif