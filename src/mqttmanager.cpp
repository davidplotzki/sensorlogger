#include "mqttmanager.h"

#include "mqttbroker.h"

mqttManager::mqttManager()
{

}

mqttManager::~mqttManager()
{
	for(size_t i=0; i<_brokers.size(); ++i)
		delete _brokers.at(i);
}

void mqttManager::addBroker(mqttBroker* broker)
{
	_brokers.push_back(broker);
}

void mqttManager::connectToMQTTBrokers()
{
	for(size_t i=0; i<_brokers.size(); ++i)
		_brokers.at(i)->connectToMQTTBroker();
}

void mqttManager::publish(const std::string &topic, const std::string &payload)
{
	for(size_t i=0; i<_brokers.size(); ++i)
		_brokers.at(i)->publish(topic, payload, false);
}