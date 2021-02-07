#include "sensor_mqtt.h"

#include "logger.h"

sensorMQTT::sensorMQTT(logger* root, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, const std::string &mqttSubscribeTopic, const std::vector<std::string>* jsonKeys, bool isCounter, double factor, double offset, uint64_t minimumRestPeriod)
{
	setPointerToLogger(root);
	setSensorID(sensorID);
	setTriggerEvent(mqttSubscribe);
	setJSONkeys(jsonKeys);
	setFactor(factor);
	setOffset(offset);
	setCounter(isCounter);
	setMinimumRestPeriod(minimumRestPeriod);
	setMQTTPublishTopic(mqttPublishTopic);
	setMQTTSubscribeTopic(mqttSubscribeTopic);
	setHomematicPublishISE(homematicPublishISE);
}

sensor_type sensorMQTT::type() const
{
	return sensor_mqtt;
}

std::string sensorMQTT::getMQTTSubscribeTopic() const
{
	return _mqttSubscribeTopic;
}

void sensorMQTT::setMQTTSubscribeTopic(const std::string &mqttSubscribeTopic)
{
	_mqttSubscribeTopic = mqttSubscribeTopic;

	if(_mqttPublishTopic == _mqttSubscribeTopic)
	{
		_mqttPublishTopic = "";
	}
}

bool sensorMQTT::measure(uint64_t currentTimestamp)
{
	return false;
}

void sensorMQTT::on_failure(const mqtt::token& tok)
{
	std::stringstream ss;
	ss << "MQTT Subscription " << _mqttSubscribeTopic << "failed.";
	_root->message(ss.str(), true);
}

void sensorMQTT::on_success(const mqtt::token& tok)
{
	std::stringstream ss;
	ss << "Subscribed to MQTT topic: " << _mqttSubscribeTopic;
	_root->message(ss.str(), false);
}
