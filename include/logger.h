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

#ifdef OPTION_CURL
	#include <curl/curl.h>
#endif

enum loglevel {
	loglevel_debug   = 0,
	loglevel_info    = 1,
	loglevel_warning = 2,
	loglevel_error   = 3
};

class readoutBuffer;
class tinkerforge;
class mqttBroker;
class mqttManager;
class homematic;
class sensor;
class logbook;

class logger
{
private:
	std::string _logFilename;
	std::string _lastLogfileMessage;
	int _logLevel;

	uint64_t _default_rest_period;
	uint64_t _default_retry_time;

	readoutBuffer* _rBuffer;
	bool _verbose;

	unsigned _brickd_restart_attempt_counter;
	unsigned _max_tinkerforge_read_failures;
	unsigned _max_brickd_restart_attempts;
	std::string _cmd_readFailures;
	std::string _cmd_readFailures_critical;

	std::vector<sensor*> _sensors;
	std::vector<logbook*> _logbooks;

	#ifdef OPTION_TINKERFORGE
		tinkerforge* _tfDaemon;
	#endif

	mqttManager* _mqttManager;
	homematic* _homematic;

	#ifdef OPTION_CURL
		CURL* _curl;
	#endif

	long _http_timeout;

public:
	logger();
	~logger();

	void info(const std::string &info_message);
	void warning(const std::string &warning_message);
	void error(const std::string &error_message);
	void debug(const std::string &debug_message);
	void message(const std::string &m, bool isError);
	std::string logLevelString();
	void welcomeMessage(const std::string &configJSON);

	void loadConfig(const std::string &configJSON);

	std::string getLogFilename() const;
	std::string logfileState() const;

	uint64_t getDefaultRetryTime() const;

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