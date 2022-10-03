#ifndef _MQTTBROKER_H
#define _MQTTBROKER_H

#ifdef OPTION_MQTT

#include <sstream>
#include "mqtt/async_client.h"

class logger;

class mqttBroker : public virtual mqtt::callback, public virtual mqtt::iaction_listener
{
private:
	std::string _host;
	unsigned    _port;
	int         _qos;
	bool        _retained;  // Should messaged be retained by the broker?
	bool        _publish_enabled;
	bool        _subscribe_enabled;
	std::string _topic_domain;
	
	std::string _connected_topic;
	std::string _connected_payload;

	std::string _lwt_topic;
	std::string _lwt_payload;

	bool        _isConnected;

	logger*     _root;

	std::string _clientID;

	mqtt::will_options    _lwt;
	mqtt::connect_options _connOpts;
	mqtt::async_client*   _mqttClient;

	void generateClientID();
	void reconnect();

	bool isValidTopic(const std::string& topic) const;

	// MQTT callbacks:
	void on_failure(const mqtt::token& tok) override;
	void on_success(const mqtt::token& tok) override;
	void connected(const std::string& cause) override;
	void connection_lost(const std::string& cause) override;
	void message_arrived(mqtt::const_message_ptr msg) override;
	void delivery_complete(mqtt::delivery_token_ptr token) override;

public:
	mqttBroker(logger* root);
	~mqttBroker();

	void setHost(const std::string &host);
	void setPort(unsigned port);
	void setQoS(int qos);
	void setRetained(bool retained);
	void enablePublish(bool enabled);
	void enableSubscribe(bool enabled);
	void setTopicDomain(const std::string& topic_domain);
	void setLWTtopic(const std::string& lwttopic);
	void setLWTpayload(const std::string& lwtpayload);
	void setConnectedTopic(const std::string& connectedtopic);
	void setConnectedPayload(const std::string& connectedpayload);

	std::string getHost() const;
	unsigned    getPort() const;
	int         getQoS() const;
	bool        doRetain() const;

	void connectToMQTTBroker();
	void publish(const std::string &topic, const std::string &payload, bool enforce);
};

#else

class mqttBroker {};

#endif
#endif