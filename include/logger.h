#ifndef _LOGGER_H
#define _LOGGER_H

#define E_SENSORID_NOT_FOUND    6001
#define E_SENSOR_DOES_NOT_EXIST 6002
#define E_JSON_READ             6003
#define E_HTTP_REQUEST_FAILED   6004

#include <cstdio>
#include <iostream>
#include <iomanip>
#include <sstream>
#include <string>
#include <vector>
#include <thread>
#include <curl/curl.h>

class readoutBuffer;
class tinkerforge;
class mqttBroker;
class homematic;
class sensor;
class logbook;

class logger
{
private:
	std::string _logFilename;
	std::string _lastLogfileMessage;

	readoutBuffer* _rBuffer;
	bool _verbose;

	unsigned _brickd_restart_attempt_counter;
	unsigned _max_tinkerforge_read_failures;
	unsigned _max_brickd_restart_attempts;
	std::string _cmd_readFailures;
	std::string _cmd_readFailures_critical;

	std::vector<sensor*> _sensors;
	std::vector<logbook*> _logbooks;

	tinkerforge* _tfDaemon;
	mqttBroker* _mqttBroker;
	homematic* _homematic;

	CURL* _curl;

public:
	logger();
	~logger();
	void message(const std::string &m, bool isError);
	void loadConfig(const std::string &configJSON);

	void welcomeMessage(const std::string &configJSON);
	std::string getLogFilename() const;
	std::string logfileState() const;

	size_t nSensors() const;
	sensor* getSensor(std::string sensorID);
	sensor* getSensor(size_t n);

	uint64_t currentTimestamp() const;

	std::string httpRequest(const std::string url);

	void setUpConnections();
	void executeSystemCommand(const std::string &command);

	void mqttPublish(const std::string &topic, const std::string &payload);
	void homematicPublish(const std::string &iseID, const std::string &payload) const;

	void trigger();  // Check if measurement is necessary, and run it if so.
};

#endif