#ifdef OPTION_MQTT

#include "mqttbroker.h"

#include "sensorlogger.h"
#include "logger.h"
#include "sensor.h"
#include "sensor_mqtt.h"
#include "json.h"

mqttBroker::mqttBroker(logger* root)
{
	_root = root;
	_host = "";
	_port = 0;
	_qos = 1;
	_retained          = false;   // Should messaged be retained by the broker?
	_publish_enabled   = true;
	_subscribe_enabled = true;
	_topic_domain      = "";

	_mqttClient = NULL;
	_isConnected = false;
}

mqttBroker::~mqttBroker()
{
	if(_mqttClient != NULL)
	{
		try {
			_mqttClient->disconnect()->wait();
		}
		catch (const mqtt::exception& exc) {
			std::stringstream ss;
			ss << exc.what();
			_root->error(ss.str());
		}

		delete _mqttClient;
	}
}

void mqttBroker::setHost(const std::string &host)
{
	_host = host;
}

void mqttBroker::setPort(unsigned port)
{
	_port = port;
}

void mqttBroker::setRetained(bool retained)
{
	_retained = retained;
	_lwt.set_retained(_retained);
}

void mqttBroker::enablePublish(bool enabled)
{
	_publish_enabled = enabled;
}

void mqttBroker::enableSubscribe(bool enabled)
{
	_subscribe_enabled = enabled;
}

void mqttBroker::setTopicDomain(const std::string& topic_domain)
{
	_topic_domain = topic_domain;
}

bool mqttBroker::isValidTopic(const std::string& topic) const
{
	if(_topic_domain.size() > 0)
	{
		if(_topic_domain.size() > topic.size())
		{
			return false;
		}
		else
		{
			// Does topic start with domain topic?
			if(topic.substr(0, _topic_domain.size()) == _topic_domain)
			{
				return true;
			}
			else
			{
				return false;
			}
		}
	}
	else
	{
		// all topics are valid
		return true;
	}

	return false;
}

void mqttBroker::setQoS(int qos)
{
	_qos = qos;
	_lwt.set_qos(_retained);
}

void mqttBroker::setLWTtopic(const std::string& lwttopic)
{
	_lwt_topic = lwttopic;
	_lwt.set_topic(_lwt_topic);
}

void mqttBroker::setLWTpayload(const std::string& lwtpayload)
{
	_lwt_payload = lwtpayload;
	_lwt.set_payload(_lwt_payload);
}

void mqttBroker::setConnectedTopic(const std::string& connectedtopic)
{
	_connected_topic = connectedtopic;
}

void mqttBroker::setConnectedPayload(const std::string& connectedpayload)
{
	_connected_payload = connectedpayload;
}


void mqttBroker::generateClientID()
{
	std::stringstream cliID;
	cliID << "Sensorlogger_" << _root->currentTimestamp();
	_clientID = cliID.str();

	_root->info("MQTT Client ID: "+_clientID);
}


std::string mqttBroker::getHost() const
{
	return _host;
}
unsigned mqttBroker::getPort() const
{
	return _port;
}
int mqttBroker::getQoS() const
{
	return _qos;
}
bool mqttBroker::doRetain() const
{
	return _retained;
}


void mqttBroker::reconnect()
{
	std::this_thread::sleep_for(std::chrono::milliseconds(2500));
	try {
		_mqttClient->connect(_connOpts, nullptr, *this);
	}
	catch (const mqtt::exception& exc) {
		std::stringstream ss;
		ss << exc.what();
		_root->error(ss.str());
		return;
	}
}

// Re-connection failure
void mqttBroker::on_failure(const mqtt::token& tok)
{
	std::stringstream ss;
	ss << "Failed to connect to MQTT Broker at " << _host << ":" << _port;
	_root->error(ss.str());

	reconnect();
}

// (Re)connection success
// Either this or connected() can be used for callbacks.
void mqttBroker::on_success(const mqtt::token& tok)
{
	std::stringstream ss;
	ss << "Connected to MQTT Broker at " << _host << ":" << _port << ".";
	_root->info(ss.str());
}

// (Re)connection success
void mqttBroker::connected(const std::string& cause)
{
	_isConnected = true;

	if(!_connected_topic.empty())
		publish(_connected_topic, _connected_payload, true);


	// Subscribe to topics:
	if(_subscribe_enabled)
	{
		for(size_t i=0; i<_root->nSensors(); ++i)
		{
			try
			{
				sensor* s = _root->getSensor(i);
				if(s->type() == sensor_mqtt)
				{
					sensorMQTT *smqtt = dynamic_cast<sensorMQTT*>(s);
					std::string subscribeTopic = smqtt->getMQTTSubscribeTopic();

					if(isValidTopic(subscribeTopic))
					{
						_mqttClient->subscribe(subscribeTopic, _qos, nullptr, *smqtt);
					}
				}
			} catch(int e) {}
		}
	}
}

// Callback for when the connection is lost.
// This will initiate the attempt to manually reconnect.
void mqttBroker::connection_lost(const std::string& cause)
{
	_isConnected = false;

	std::stringstream ss;
	ss << "Connection to MQTT Broker lost: " << _host << ":" << _port << ".";
	if (!cause.empty())
		ss << " Cause: " << cause;

	_root->error(ss.str());
	_root->info("Reconnecting to MQTT Broker...");

	reconnect();
}

// Callback for when a message arrives.
void mqttBroker::message_arrived(mqtt::const_message_ptr msg)
{
	std::string topic = msg->get_topic();
	std::string payload = msg->to_string();

	for(size_t i=0; i<_root->nSensors(); ++i)
	{
		sensor* s = _root->getSensor(i);
		if(s->type() == sensor_mqtt)
		{
			sensorMQTT* smqtt = dynamic_cast<sensorMQTT*>(s);

			if(topic == smqtt->getMQTTSubscribeTopic())
			{
				// Are we supposed to find a key in this JSON object?
				if(smqtt->nJSONkeys() > 0)
				{
					try
					{
						json jsonPayload;
						jsonPayload.setContent(payload);
						jsonPayload.parse();

						jsonNode* node = jsonPayload.root();
						for(size_t key=0; key<smqtt->nJSONkeys(); ++key)
						{
							try
							{ 
								node = node->element(smqtt->jsonKey(key));
							}
							catch(int e)
							{
								std::stringstream ss;
								ss << "Error finding value for JSON key \'" << smqtt->jsonKey(key) << "\' in MQTT message. Topic: \'" << topic << "\', Payload: '" << payload << "\'.";
								_root->error(ss.str());
								throw e;
							}
						}

						double value = node->value()->getDouble();
						if(smqtt->addRawMeasurement(value))
						{
							smqtt->publishLastEvent();
						}
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "Error parsing JSON payload in MQTT message. Topic: \'" << topic << "\', Payload: '" << payload << "\'.";
						_root->error(ss.str());
					}
				}
				else
				{
					try
					{
						double value = atof(payload.c_str());
						if(smqtt->addRawMeasurement(value))
						{
							smqtt->publishLastEvent();
						}
						
					} catch(int e) {}
				}
			}
		}
	}
}

void mqttBroker::delivery_complete(mqtt::delivery_token_ptr token)
{

}

void mqttBroker::connectToMQTTBroker()
{
	if(_host.size() > 0)
	{
		generateClientID();

		_connOpts.set_keep_alive_interval(MQTT_KEEPALIVE_INTERVAL);
		_connOpts.set_clean_session(true);
		
		if(!_lwt_topic.empty())
			_connOpts.set_will(_lwt);

		std::stringstream serverAddress;
		serverAddress << "tcp://" << _host << ":" << _port;
		_mqttClient = new mqtt::async_client(serverAddress.str(), _clientID);

		_mqttClient->set_callback(*this);

		try {
			std::stringstream ss;
			ss << "Connecting to MQTT Broker at " << _host << ":" << _port << "...";
			_root->info(ss.str());

			_mqttClient->connect(_connOpts, nullptr, *this);
		}
		catch (const mqtt::exception&) {
			std::stringstream ss;
			ss << "Error connecting to MQTT Broker at " << _host << ":" << _port << ".";
			_root->error(ss.str());
		}
	}
}

void mqttBroker::publish(const std::string &topic, const std::string &payload, bool enforce)
{
	if(_publish_enabled || enforce)
	{
		if(_isConnected)
		{
			if(isValidTopic(topic) || enforce)
			{
				_mqttClient->publish(topic, payload.c_str(), payload.size(), _qos, _retained);
			}
		}
	}
}

#endif