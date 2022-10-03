#include "logger.h"

#include "sensorlogger.h"
#include "readoutbuffer.h"
#include "tinkerforge.h"
#include "mqttbroker.h"
#include "mqttmanager.h"
#include "homematic.h"
#include "sensor.h"
#include "sensor_json.h"
#include "sensor_mqtt.h"
#include "sensor_homematic.h"
#include "sensor_tinkerforge.h"
#include "column.h"
#include "logbook.h"
#include "json.h"

size_t receiveHTTP(void* buffer, size_t size, size_t nmemb, void* userp)
{
	static_cast<std::string*>(userp)->append(static_cast<char*>(buffer), size*nmemb);

	return size*nmemb;
}

logger::logger()
{
	_max_tinkerforge_read_failures  = DEFAULT_MAX_BRICKLET_READ_FAILURES;
	_max_brickd_restart_attempts    = DEFAULT_MAX_BRICKD_RESTART_ATTEMPTS;
	_brickd_restart_attempt_counter = 0;

	_verbose = true;
	_logLevel = loglevel_info;

	_default_rest_period = DEFAULT_MEASUREMENT_INTERVAL;
	_default_retry_time  = DEFAULT_RETRY_TIME;

	#ifdef OPTION_TINKERFORGE
		_tfDaemon    = new tinkerforge(this);
	#endif

	_mqttManager = new mqttManager();
	_homematic   = new homematic(this);

	#ifdef OPTION_CURL
		curl_global_init(CURL_GLOBAL_DEFAULT);
		_curl = curl_easy_init();
	#endif
	_http_timeout = DEFAULT_HTTP_TIMEOUT;

	_rBuffer = new readoutBuffer(this);
}

logger::~logger()
{
	for(size_t i=0; i<_sensors.size(); ++i)
		delete _sensors.at(i);

	if(_mqttManager != NULL)
		delete _mqttManager;

	if(_rBuffer != NULL)
		delete _rBuffer;
	
	#ifdef OPTION_TINKERFORGE
		if(_tfDaemon != NULL)
			delete _tfDaemon;
	#endif

	if(_homematic != NULL)
		delete _homematic;

	#ifdef OPTION_CURL
		if(_curl)
			curl_easy_cleanup(_curl);

		curl_global_cleanup();
	#endif
}

void logger::debug(const std::string &debug_message)
{
	if(_logLevel <= loglevel_debug)
	{
		message("Debug: " + debug_message, false);
	}
}

void logger::info(const std::string &info_message)
{
	if(_logLevel <= loglevel_info)
	{
		message(info_message, false);
	}
}

void logger::warning(const std::string &warning_message)
{
	if(_logLevel <= loglevel_warning)
	{
		message("Warning: " + warning_message, false);
	}
}

void logger::error(const std::string &error_message)
{
	if(_logLevel <= loglevel_error)
	{
		message("ERROR: " + error_message, true);
	}
}

void logger::message(const std::string &m, bool isError)
{
	if(m != _lastLogfileMessage)
	{
		_lastLogfileMessage = ""; //m;

		if(_verbose)
		{
			if(isError)
				std::cerr << m << std::endl;
			else
				std::cout << m << std::endl;
		}

		if(_logFilename.length() > 0)
		{
			time_t rawtime;
			struct tm * timeinfo;
			time(&rawtime);
			timeinfo = localtime(&rawtime);

			int year    = timeinfo->tm_year + 1900;
			int day     = timeinfo->tm_mday;
			int month   = timeinfo->tm_mon + 1;
			int hour    = timeinfo->tm_hour;
			int minute 	= timeinfo->tm_min;
			int second  = timeinfo->tm_sec;

			std::ofstream outfile;
			outfile.open(_logFilename, std::ios_base::app);
			if(outfile.is_open())
			{
				outfile << std::setfill('0') << std::setw(2) << year << "-";
				outfile << std::setfill('0') << std::setw(2) << month << "-";
				outfile << std::setfill('0') << std::setw(2) << day <<" ";
				outfile << std::setfill('0') << std::setw(2) << hour << ":";
				outfile << std::setfill('0') << std::setw(2) << minute << ":";
				outfile << std::setfill('0') << std::setw(2) << second << " > ";
				outfile << m << std::endl;
				outfile.close();
			}
			else
			{
				std::cerr << "Cannot open logfile to write: " << _logFilename << std::endl;
			}			
		}
	}
}

std::string logger::logLevelString()
{
	switch(_logLevel)
	{
		case(loglevel_debug): return "debug"; break;
		case(loglevel_info): return "info"; break;
		case(loglevel_warning): return "warning"; break;
		case(loglevel_error): return "error"; break;
	}

	return "unknown";
}

void logger::welcomeMessage(const std::string &configJSON)
{
	message("---", false);
	message("Sensorlogger, Ver. " + std::string(SENSORLOGGER_VERSION) + " (" + SENSORLOGGER_VERSION_DATE + ")", false);
	message("  Configuration: " + configJSON, false);
	message("  Logfile: " + logfileState(), false);
}

void logger::loadConfig(const std::string &configJSON)
{
	json configFile;
	try
	{
		configFile.parseFile(configJSON);

		// General configuration
		try	{
			_logFilename = configFile.element("general")->element("logfile")->value()->getString();
			welcomeMessage(configJSON);
		}
		catch(int e) {
			_logFilename = "";
			welcomeMessage(configJSON);
			
			info("No logfile given under general/logfile. Logging into file is disabled.");
		}

		try	{
			std::string json_loglevel = configFile.element("general")->element("loglevel")->value()->getString();

			if(json_loglevel == "debug")
			{
				_logLevel = loglevel_debug;
			}
			else if (json_loglevel == "info")
			{
				_logLevel = loglevel_info;
			}
			else if (json_loglevel == "warning")
			{
				_logLevel = loglevel_warning;
			}
			else if (json_loglevel == "error")
			{
				_logLevel = loglevel_error;
			}
		}
		catch(int e) {
			debug("loglevel not found in config file.");
		}
		debug("Log Level: " + logLevelString());

		try	{
			_http_timeout = static_cast<long>(configFile.element("general")->element("http_timeout")->value()->getInt());
		}
		catch(int e) {
			_http_timeout = DEFAULT_HTTP_TIMEOUT;
		}
		debug("HTTP Timeout: " + std::to_string(_http_timeout) + " s");

		try	{
			_default_rest_period = configFile.element("general")->element("default_rest_period")->durationInMS();
		}
		catch(int e) {
			_default_rest_period = DEFAULT_MEASUREMENT_INTERVAL;
		}
		debug("Default rest period: " + std::to_string(_default_rest_period) + " ms");

		try	{
			_default_retry_time = configFile.element("general")->element("default_retry_time")->durationInMS();
		}
		catch(int e) {
			_default_retry_time = DEFAULT_RETRY_TIME;
		}
		debug("Default retry time: " + std::to_string(_default_retry_time) + " ms");

		// Homematic configuration
		if(configFile.existAndNotNull("homematic"))
		{
			if(configFile.element("homematic")->existAndNotNull("xmlapi_url"))
			{
				try {
					_homematic->setXMLAPI_URL(configFile.element("homematic")->element("xmlapi_url")->value()->getString());
				} catch(int e) { }
			}
		}
		debug("Homematic xmlapi_url: " + _homematic->getXMLAPI_URL());

		// Tinkerforge configuration
		if(configFile.existAndNotNull("tinkerforge"))
		{
			if(configFile.element("tinkerforge")->existAndNotNull("host"))
			{
				#ifdef OPTION_TINKERFORGE
					try {
						_tfDaemon->setHost(configFile.element("tinkerforge")->element("host")->value()->getString());
					} catch(int e) { }

					try {
						_tfDaemon->setPort(static_cast<unsigned short>(configFile.element("tinkerforge")->element("port")->value()->getInt()));
					} catch(int e) { }
				#else
					error("Cannot set up Tinkerforge connection. This version of Sensorlogger was compiled without Tinkerforge support.");
				#endif
			}
		}

		#ifdef OPTION_TINKERFORGE
			debug("Tinkerforge Host: " + _tfDaemon->getHost());
			debug("Tinkerforge Port: " + std::to_string(_tfDaemon->getPort()));

			try {
				_max_tinkerforge_read_failures = configFile.element("tinkerforge")->element("max_bricklet_read_failures")->value()->getInt();
			} catch(int e) {
				_max_tinkerforge_read_failures = DEFAULT_MAX_BRICKLET_READ_FAILURES;
			}
			debug("Tinkerforge max_bricklet_read_failures: " + std::to_string(_max_tinkerforge_read_failures));

			try {
				_max_brickd_restart_attempts = configFile.element("tinkerforge")->element("max_brickd_restart_attempts")->value()->getInt();
			} catch(int e) {
				_max_brickd_restart_attempts = DEFAULT_MAX_BRICKD_RESTART_ATTEMPTS;
			}
			debug("Tinkerforge max_brickd_restart_attempts: " + std::to_string(_max_brickd_restart_attempts));

			try {
				_cmd_readFailures = configFile.element("tinkerforge")->element("brickd_restart_command")->value()->getString();
			} catch(int e) {
				_cmd_readFailures = "";
			}
			debug("Tinkerforge brickd_restart_command: " + _cmd_readFailures);

			try {
				_cmd_readFailures_critical = configFile.element("tinkerforge")->element("system_restart_command")->value()->getString();
			} catch(int e) {
				_cmd_readFailures_critical = "";
			}
			debug("Tinkerforge system_restart_command: " + _cmd_readFailures_critical);
		#endif


		// Sensor configuration
		try {
			jsonNode* sensorNode = configFile.element("sensors");
			if(sensorNode->nElements() > 0)
			{
				for(size_t i=0; i<sensorNode->nElements(); ++i)
				{
					try
					{
						jsonNode *s = sensorNode->element(i);
						std::string sensorID;

						std::string sensorJSONfile;
						std::vector<std::string> sensorJSONkeys;

						std::string mqttPublishTopic = "";
						std::string mqttSubscribeTopic = "";

						std::string homematicPublishISE = "";
						std::string homematicSubscribeISE = "";

						std::string tinkerforge_uid;
						#ifdef OPTION_TINKERFORGE
							uint8_t channel = 0;
							char ioPort = 'a';
							trigger_event triggerEvent = periodic;
							uint32_t io_debounce_ms = DEFAULT_DEBOUNCE_TIME;
						#endif

						bool isCounter = false;

						double sensorFactor = 1.0;
						double sensorOffset = 0.0;
						uint64_t sensorMinimumRestPeriod = _default_rest_period;
						uint64_t sensorRetryTime = _default_retry_time;

						try {
							sensorID = s->element("sensor_id")->value()->getString();
						} catch(int e) {
							std::stringstream ss;
							ss << "Sensor #" << (i+1) <<": sensor_id not found in config file.";
							error(ss.str());
							continue;
						}

						try {
							sensorJSONfile = s->element("json_file")->value()->getString();
						} catch(int e) {}

						try {
							if(s->element("json_key")->nElements() == 0)
							{
								std::string sensorJSONkey = s->element("json_key")->value()->getString();
								sensorJSONkeys.push_back(sensorJSONkey);
							}
							else // nested JSON key
							{
								for(size_t j=0; j<s->element("json_key")->nElements(); ++j)
								{
									std::string sensorJSONkey = s->element("json_key")->element(j)->value()->getString();
									sensorJSONkeys.push_back(sensorJSONkey);
								}								
							}							
						} catch(int e) {}

						try {
							mqttPublishTopic = s->element("mqtt_publish")->value()->getString();
						} catch(int e) {}

						try {
							mqttSubscribeTopic = s->element("mqtt_subscribe")->value()->getString();
						} catch(int e) {}

						try {
							homematicPublishISE = s->element("homematic_publish")->value()->getString();
						} catch(int e) {}

						try {
							homematicSubscribeISE = s->element("homematic_subscribe")->value()->getString();
						} catch(int e) {}

						try {
							tinkerforge_uid = s->element("tinkerforge_uid")->value()->getString();
						} catch(int e) {}

						#ifdef OPTION_TINKERFORGE
							try {
								channel = static_cast<uint8_t>(s->element("channel")->value()->getInt());
							} catch(int e) {}

							try {
								std::string sioPort = s->element("io_port")->value()->getString();
								if(sioPort.size() == 1)
								{
									ioPort = sioPort.at(0);
									if(!(ioPort == 'A' || ioPort == 'a' || ioPort == 'B' || ioPort == 'b'))
									{
										std::stringstream ss;
										ss << "Not a valid io_port : \'" << sioPort <<"\'. The io_port for an IO Bricklet must be either \'a\' or \'b\'. Default to \'a\'.";
										error(ss.str());
										ioPort = 'a';
									}
								}
								else
								{
									std::stringstream ss;
									ss << "Not a valid io_port : \'" << sioPort <<"\'. The io_port for an IO Bricklet must be either \'a\' or \'b\'. Default to \'a\'.";
									error(ss.str());
								}
							} catch(int e) {}

							try {
								std::string triggerText = s->element("trigger")->value()->getString();
								if(triggerText == "periodic")
									triggerEvent = periodic;
								else if(triggerText == "high")
									triggerEvent = high;
								else if(triggerText == "low")
									triggerEvent = low;
								else if(triggerText == "high_or_low")
									triggerEvent = high_or_low;
								else if(triggerText == "high_and_low")
									triggerEvent = high_or_low;
								else
								{
									error("Unknown trigger type: \'"+triggerText+"\'");
								}
							} catch(int e) {}

							try {
								io_debounce_ms = static_cast<uint32_t>(s->element("io_debounce")->durationInMS());
							} catch(int e) {}
						#endif

						try {
							isCounter = s->element("counter")->value()->getBool();
						} catch(int e) {}

						try {
							sensorFactor = s->element("factor")->value()->getDouble();
						} catch(int e) {}

						try {
							sensorOffset = s->element("offset")->value()->getDouble();
						} catch(int e) {}

						try {
							sensorMinimumRestPeriod = s->element("rest_period")->durationInMS();
						}
						catch(int e)
						{
							if(tinkerforge_uid != "")
							{
								std::stringstream ss;
								ss << "Sensor " << (i+1) << " (" << sensorID << ") has no valid minimum rest period. Default to " << sensorMinimumRestPeriod << "ms.";
								warning(ss.str());
							}
						}

						try {
							sensorRetryTime = s->element("retry_time")->durationInMS();
						}
						catch(int e)
						{
						}

						if(sensorJSONfile.size() > 0)  // Create a JSON sensor
						{
							sensorJSON* jsonSensor = new sensorJSON(this, sensorID, mqttPublishTopic, homematicPublishISE, sensorJSONfile, &sensorJSONkeys, isCounter, sensorFactor, sensorOffset, sensorMinimumRestPeriod, sensorRetryTime, _rBuffer);
							_sensors.push_back(jsonSensor);
						}
						else if(tinkerforge_uid.size() > 0)
						{
							#ifdef OPTION_TINKERFORGE
								sensorTinkerforge* tfSensor = new sensorTinkerforge(this, sensorID, mqttPublishTopic, homematicPublishISE, _tfDaemon, tinkerforge_uid, triggerEvent, isCounter, channel, ioPort, io_debounce_ms, sensorFactor, sensorOffset, sensorMinimumRestPeriod, sensorRetryTime);
								_sensors.push_back(tfSensor);
							#else
								error("Cannot add Tinkerforge sensor. This version of Sensorlogger was compiled without support for Tinkerforge.");
							#endif
						}
						else if(mqttSubscribeTopic.size() > 0)
						{
							#ifdef OPTION_MQTT
								sensorMQTT* mqttSensor = new sensorMQTT(this, sensorID, mqttPublishTopic, homematicPublishISE, mqttSubscribeTopic, &sensorJSONkeys, isCounter, sensorFactor, sensorOffset, sensorMinimumRestPeriod, sensorRetryTime);
								_sensors.push_back(mqttSensor);
							#else
								error("Cannot add MQTT sensor. This version of Sensorlogger was compiled without support for MQTT.");
							#endif
						}
						else if(homematicSubscribeISE.size() > 0)
						{
							sensorHomematic* homematicSensor = new sensorHomematic(this, _homematic, sensorID, mqttPublishTopic, homematicPublishISE, homematicSubscribeISE, isCounter, sensorFactor, sensorOffset, sensorMinimumRestPeriod, sensorRetryTime);
							_sensors.push_back(homematicSensor);
						}
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "Error in configuration for sensor #" << (i+1);
						error(ss.str());
					}
				}
			}
		} catch(int e) {
			error("No valid sensor configuration found.");
		}

		// Logging configuration
		if(configFile.existAndNotNull("logbooks"))
		{
			try
			{
				jsonNode* logs = configFile.element("logbooks");
				size_t nLogs = logs->nElements();
				for(size_t i=0; i<nLogs; ++i)
				{
					try
					{
						jsonNode *currentLog = logs->element(i);

						std::string filename = "";
						try	{
							filename = currentLog->element("filename")->value()->getString();
						} catch(int e) {
							std::stringstream ss;
							ss << "Warning for configuration for logbook #" << (i+1) << ": no filename specified.";
							error(ss.str());
						}

						uint64_t cycleTime = DEFAULT_CYCLETIME;
						try	{
							cycleTime = static_cast<uint64_t>(currentLog->element("cycle_time")->durationInMS());
						} catch(int e) {
							std::stringstream ss;
							ss << "Error in configuration for logbook #" << (i+1) << ": cannot get cycle_time. Default to " << cycleTime<<".";
							error(ss.str());
						}

						unsigned maxEntries = DEFAULT_MAX_ENTRIES;
						try	{
							maxEntries = currentLog->element("max_entries")->value()->getInt();
						} catch(int e) {
							std::stringstream ss;
							ss << "Error in configuration for logbook #" << (i+1) << ": cannot get max_entries. Default to " << maxEntries<<".";
							error(ss.str());
						}

						std::string missingDataToken = "-";
						try	{
							missingDataToken = currentLog->element("missing_data")->value()->getString();
						} catch(int e) { }

						// Create new logbook:
						logbook* l = new logbook(this, filename, cycleTime, maxEntries, missingDataToken);
						_logbooks.push_back(l);

						// Add logbook columns:
						try {
							jsonNode* cols = currentLog->element("columns");
							size_t nCols = cols->nElements();
							for(size_t c=0; c<nCols; ++c)
							{
								try
								{
									jsonNode* currentCol = cols->element(c);

									sensor* colSensor = NULL;
									std::string colSensorID;

									try {
										colSensorID = currentCol->element("sensor_id")->value()->getString();
									} catch(int e) {
										std::stringstream ss;
										ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": sensor_id not found.";
										error(ss.str());
										throw e;
									}

									try	{
										colSensor = getSensor(colSensorID);
									} catch(int e) {
										std::stringstream ss;
										ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": sensor \'" << colSensorID << "\' not found.";
										error(ss.str());
										throw e;
									}
									
									std::string title = "";
									try {
										title = currentCol->element("title")->value()->getString();
									} catch(int e) {
										std::stringstream ss;
										ss << "Warning in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": no title found.";
										error(ss.str());
									}

									std::string unit = "";
									try {
										unit = currentCol->element("unit")->value()->getString();
									} catch(int e) {}

									uint64_t evaluationPeriod = 0;
									try {
										evaluationPeriod = currentCol->element("evaluation_period")->durationInMS();
									} catch(int e) {}

									operation colOp = mean;
									try {
										std::string colOpString = currentCol->element("operation")->value()->getString();
										if(colOpString == "mean")               {colOp = mean;}
										else if(colOpString == "median")        {colOp = median;}
										else if(colOpString == "max")           {colOp = max;}
										else if(colOpString == "min")           {colOp = min;}
										else if(colOpString == "sum")           {colOp = sum;}
										else if(colOpString == "count")         {colOp = count;}
										else if(colOpString == "freq")          {colOp = freq;}
										else if(colOpString == "freq_min")      {colOp = freq_min;}
										else if(colOpString == "freq_max")      {colOp = freq_max;}
										else if(colOpString == "stddev")        {colOp = stdDevMean;}
										else if(colOpString == "stddev_mean")   {colOp = stdDevMean;}
										else if(colOpString == "stddev_median") {colOp = stdDevMedian;}
										else
										{
											std::stringstream ss;
											ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": \'" << colOpString << "\' is not a valid operation.";
											error(ss.str());
										}
									} catch(int e) {
										std::stringstream ss;
										ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": no operation found.";
										error(ss.str());
									}

									double confidenceAbsolute = 0;
									try {
										confidenceAbsolute = currentCol->element("confidence_absolute")->value()->getDouble();
									} catch(int e) {}

									double confidenceSigma = 0;
									try {
										confidenceSigma = currentCol->element("confidence_sigma")->value()->getDouble();
									} catch(int e) {}

									double countFactor = 1;
									try {
										countFactor = currentCol->element("count_factor")->value()->getDouble();
									} catch(int e) {}

									std::string colMqttPublishTopic = "";
									try {
										colMqttPublishTopic = currentCol->element("mqtt_publish")->value()->getString();
									} catch(int e) {}

									std::string colHomematicPublishISE = "";
									try {
										colHomematicPublishISE = currentCol->element("homematic_publish")->value()->getString();
									} catch(int e) {}

									column* newColumn = new column(colSensor, l, title, unit, evaluationPeriod, colOp, confidenceAbsolute, confidenceSigma, colMqttPublishTopic, colHomematicPublishISE, countFactor);
									l->addColumn(newColumn);
								}
								catch(int e)
								{
									std::stringstream ss;
									ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<".";
									error(ss.str());
								}
								

							}
						} catch(int e) {
							std::stringstream ss;
							ss << "Error in configuration for logbook #" << (i+1) << ": error parsing column information.";
							error(ss.str());
						}
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "Error in configuration for logbook #" << (i+1);
						error(ss.str());
					}
				}
			}
			catch(int e)
			{
				error("Error in configuration for the logbooks.");
			}
		}

		// MQTT Configuration:
		if(configFile.existAndNotNull("mqtt"))
		{
			 jsonNode* mqttNode = configFile.element("mqtt");
			 size_t nMqttBrokers = 0;

			 if(mqttNode->existAndNotNull("host"))
			 {
			 	// Only one MQTT broker seems to be defined.
			 	nMqttBrokers = 1;
			 }
			 else
			 {
			 	// This might be a JSON array that defines multiple MQTT brokers.
			 	// Start with the first element in the supposed array.
			 	mqttNode = configFile.element("mqtt")->element(0);
			 	nMqttBrokers = configFile.element("mqtt")->nElements();
			 }

			 for(size_t m=0; m<nMqttBrokers; ++m)
			 {
				if(mqttNode->existAndNotNull("host"))
				{
					try
					{
						#ifdef OPTION_MQTT
							std::string mqttHost = mqttNode->element("host")->value()->getString();
							int mqttPort = mqttNode->element("port")->value()->getInt();

							int mqttQoS = 1;
							try
							{
								mqttQoS = mqttNode->element("qos")->value()->getInt();
							}
							catch(int e)
							{
								std::stringstream ss;
								ss << "No MQTT QoS defined. Default to " << mqttQoS << ".";
								info(ss.str());
							}

							bool mqttRetained = false;
							try
							{
								mqttRetained = mqttNode->element("retained")->value()->getBool();
							}
							catch(int e)
							{
								std::stringstream ss;
								ss << "No MQTT \'retained\' setting defined. Default to false.";
								info(ss.str());
							}

							bool mqttPublishEnabled = true;
							try
							{
								mqttPublishEnabled = mqttNode->element("enable_publish")->value()->getBool();
							}
							catch(int e)
							{
								
							}

							bool mqttSubscribeEnabled = true;
							try
							{
								mqttSubscribeEnabled = mqttNode->element("enable_subscribe")->value()->getBool();
							}
							catch(int e)
							{
								
							}

							std::string mqtt_topic_domain;
							std::string mqtt_connected_topic;
							std::string mqtt_connected_payload;
							std::string mqtt_lwt_topic;
							std::string mqtt_lwt_payload;

							try
							{
								mqtt_topic_domain = mqttNode->element("topic_domain")->value()->getString();
							}
							catch(int e) { }

							try
							{
								mqtt_connected_topic = mqttNode->element("connected_topic")->value()->getString();
							}
							catch(int e) { }

							try
							{
								mqtt_connected_payload = mqttNode->element("connected_message")->value()->getString();
							}
							catch(int e) { }

							try
							{
								mqtt_lwt_topic = mqttNode->element("lwt_topic")->value()->getString();
							}
							catch(int e) { }

							try
							{
								mqtt_lwt_payload = mqttNode->element("lwt_message")->value()->getString();
							}
							catch(int e) { }

							if(mqttHost.size() > 0)
							{
								mqttBroker *b = new mqttBroker(this);
								b->setHost(mqttHost);
								b->setPort(mqttPort);
								b->setQoS(mqttQoS);
								b->setRetained(mqttRetained);
								b->setConnectedTopic(mqtt_connected_topic);
								b->setConnectedPayload(mqtt_connected_payload);
								b->setLWTtopic(mqtt_lwt_topic);
								b->setLWTpayload(mqtt_lwt_payload);
								b->enablePublish(mqttPublishEnabled);
								b->enableSubscribe(mqttSubscribeEnabled);
								b->setTopicDomain(mqtt_topic_domain);

								_mqttManager->addBroker(b);
							}
						#else
							error("Cannot set up MQTT broker. This version of Sensorlogger was compiled without support for MQTT.");
						#endif
					}
					catch(int e)
					{
						error("MQTT configuration contains errors.");
					}
				}

				// Try the next array element:
				if((m+1) < nMqttBrokers)
					mqttNode = configFile.element("mqtt")->element(m+1);
				else
					break;
			}
		}
	}
	catch(int e)
	{
		error("Parsing config file failed: " + configJSON);
		throw E_JSON_READ;
	}
}

std::string logger::getLogFilename() const
{
	return _logFilename;
}

std::string logger::logfileState() const
{
	if(_logFilename == "")
		return "Logging disabled.";
	else
		return _logFilename;
}

uint64_t logger::getDefaultRetryTime() const
{
	return _default_retry_time;
}

size_t logger::nSensors() const
{
	return _sensors.size();
}

sensor* logger::getSensor(std::string sensorID)
{
	for(size_t i=0; i<_sensors.size(); ++i)
	{
		if(_sensors.at(i)->getSensorID() == sensorID)
			return _sensors.at(i);
	}

	throw E_SENSORID_NOT_FOUND;
}

sensor* logger::getSensor(size_t n)
{
	if(n < _sensors.size())
		return _sensors.at(n);

	throw E_SENSOR_DOES_NOT_EXIST;
}

uint64_t logger::currentTimestamp() const
{
	const auto now = std::chrono::system_clock::now();
	uint64_t timestamp = static_cast<uint64_t>(std::chrono::duration_cast<std::chrono::milliseconds>(now.time_since_epoch()).count());

	return timestamp;
}

std::string logger::httpRequest(const std::string url)
{
	#ifdef OPTION_CURL
		if(_curl)
		{
			std::string response;

			curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
			curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
			curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, receiveHTTP);
			curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &response);
			curl_easy_setopt(_curl, CURLOPT_TIMEOUT, _http_timeout);

			// Do not download more than 10 MB = 10485760 Byte:
			curl_easy_setopt(_curl, CURLOPT_MAXFILESIZE, DEFAULT_HTTP_MAXFILESIZE);

			// Maximum redirects:
			curl_easy_setopt(_curl, CURLOPT_MAXREDIRS, DEFAULT_HTTP_MAXREDIRS);

			CURLcode res = curl_easy_perform(_curl);
			if(res != CURLE_OK)
			{
				std::string easyReadError = curl_easy_strerror(res);
				error("HTTP(S) request failed: " + easyReadError);

				throw E_HTTP_REQUEST_FAILED;
			}

			return response;
		}

		throw E_HTTP_REQUEST_FAILED;
	#else
		error("Cannot make HTTP(S) request. This version of Sensorlogger was compiled without support for libcurl.");
		return "";
	#endif
}

void logger::setUpConnections()
{
	_mqttManager->connectToMQTTBrokers();
}

void logger::executeSystemCommand(const std::string &command)
{
	info("Execute system command: "+command);

	FILE *fp;

	// Redirect errors to stdout:
	std::string errorRedirectedCommand = command + " 2>&1";

	fp = popen(errorRedirectedCommand.c_str(), "r");
	if(fp != NULL)
	{
		char line[4096];
		std::string output;
		while (fgets(line, 4096, fp) != NULL)
			output += line;

		int status = pclose(fp);
		if(status == -1)
			error("Executing the command failed.");

		if(output.size() > 0)
			info(output);
	}
	else
		error("Executing the command failed.");
}

void logger::mqttPublish(const std::string &topic, const std::string &payload)
{
	if(_mqttManager != NULL)
		_mqttManager->publish(topic, payload);
}

void logger::homematicPublish(const std::string &iseID, const std::string &payload) const
{
	if(_homematic != NULL)
		_homematic->publish(iseID, payload);
}

void logger::trigger()
{
	uint64_t current = currentTimestamp();
	_rBuffer->cleanUp(current);

	for(size_t i=0; i<_sensors.size(); ++i)
	{
		sensor* s = _sensors.at(i);
		try
		{
			if(s->measure(current))
			{
				try
				{
					s->publishLastEvent();
				}
				catch(int i)
				{
				}
			}
		} catch(int e)
		{
			std::cerr<<"Error "<<e<<" when measuring at sensor #"<<(i+1)<<"."<<std::endl;
		}
	}

	for(size_t i=0; i<_logbooks.size(); ++i)
	{
		try
		{
			_logbooks.at(i)->write(_mqttManager, _homematic);
		} catch(int e)
		{
			std::cerr<<"Error "<<e<<" when writing logbook "<<(i+1)<<"."<<std::endl;
		}
	}

	#ifdef OPTION_TINKERFORGE
		// Check if a restart of the Tinkerforge Brick Daemon might be necessary:
		size_t nSensorsFailedTooMuch = 0;
		for(size_t i=0; i<_sensors.size(); ++i)
		{
			if(_sensors.at(i)->type() == sensor_tinkerforge)
			{
				sensorTinkerforge* s = dynamic_cast<sensorTinkerforge*>(_sensors.at(i));
				if(s->getReadFailures() > 0)
				{
					if(s->getReadFailures() >= _max_tinkerforge_read_failures)
					{
						if(nSensorsFailedTooMuch == 0)
						{
							error("Too many Tinkerforge Bricklet read failures:");
						}

						std::stringstream ss;
						ss << "  Bricklet \'" << s->getUID() << "\' ("<< getDeviceType_name(s->getDeviceType()) << "): " << s->getReadFailures() << " read failures.";
						error(ss.str());

						++nSensorsFailedTooMuch;
					}
				}
			}
		}

		if(nSensorsFailedTooMuch > 0)
		{
			// Reset all sensor read failures:
			for(size_t i=0; i<_sensors.size(); ++i)
				_sensors.at(i)->resetReadFailures();

			if(_brickd_restart_attempt_counter >= _max_brickd_restart_attempts)
			{
				_brickd_restart_attempt_counter = 0;
				
				if(_cmd_readFailures_critical.size() > 0)
				{
					error("Trying to reboot the entire system...");
					executeSystemCommand(_cmd_readFailures_critical);
					std::this_thread::sleep_for(std::chrono::seconds(2));
				}

				return;
			}

			if(_cmd_readFailures.size() > 0)
			{
				error("Trying to restart Brick Daemon...");
				executeSystemCommand(_cmd_readFailures);
				++_brickd_restart_attempt_counter;
				std::this_thread::sleep_for(std::chrono::seconds(30));
			}
		}
	#endif
}