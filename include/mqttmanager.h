#ifndef _MQTTMANAGER_H
#define _MQTTMANAGER_H

// Manages several MQTT brokers.

#include <vector>
#include <string>

class mqttBroker;

class mqttManager
{
private:
	std::vector<mqttBroker*> _brokers;

public:
	mqttManager();
	~mqttManager();

	void addBroker(mqttBroker* broker);

	void connectToMQTTBrokers();
	void publish(const std::string &topic, const std::string &payload);

};

#endif