#include "logger.h"

#include "sensorlogger.h"
#include "readoutbuffer.h"
#include "tinkerforge.h"
#include "mqttbroker.h"
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

	_tfDaemon   = new tinkerforge(this);
	_mqttBroker = new mqttBroker(this);
	_homematic  = new homematic(this);

	curl_global_init(CURL_GLOBAL_DEFAULT);
	_curl = curl_easy_init();

	_rBuffer = new readoutBuffer(this);
}

logger::~logger()
{
	for(size_t i=0; i<_sensors.size(); ++i)
		delete _sensors.at(i);

	if(_rBuffer != NULL)
		delete _rBuffer;
	
	if(_tfDaemon != NULL)
		delete _tfDaemon;

	if(_mqttBroker != NULL)
		delete _mqttBroker;

	if(_homematic != NULL)
		delete _homematic;

	if(_curl)
		curl_easy_cleanup(_curl);

	curl_global_cleanup();
}

void logger::message(const std::string &m, bool isError)
{
	if(m != _lastLogfileMessage)
	{
		_lastLogfileMessage = m;

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
			int second = timeinfo->tm_sec;

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
			
			message("No logfile given under general/logfile. Logging into file is disabled.", false);
		}

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

		// Tinkerforge configuration
		if(configFile.existAndNotNull("tinkerforge"))
		{
			if(configFile.element("tinkerforge")->existAndNotNull("host"))
			{
				try {
					_tfDaemon->setHost(configFile.element("tinkerforge")->element("host")->value()->getString());
				} catch(int e) { }

				try {
					_tfDaemon->setPort(static_cast<unsigned short>(configFile.element("tinkerforge")->element("port")->value()->getInt()));
				} catch(int e) { }
			}
		}

		try {
			_max_tinkerforge_read_failures = configFile.element("tinkerforge")->element("max_bricklet_read_failures")->value()->getInt();
		} catch(int e) {
			_max_tinkerforge_read_failures = DEFAULT_MAX_BRICKLET_READ_FAILURES;
		}

		try {
			_max_brickd_restart_attempts = configFile.element("tinkerforge")->element("max_brickd_restart_attempts")->value()->getInt();
		} catch(int e) {
			_max_brickd_restart_attempts = DEFAULT_MAX_BRICKD_RESTART_ATTEMPTS;
		}

		try {
			_cmd_readFailures = configFile.element("tinkerforge")->element("brickd_restart_command")->value()->getString();
		} catch(int e) {
			_cmd_readFailures = "";
		}

		try {
			_cmd_readFailures_critical = configFile.element("tinkerforge")->element("system_restart_command")->value()->getString();
		} catch(int e) {
			_cmd_readFailures_critical = "";
		}


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
						uint8_t channel = 0;
						char ioPort = 'a';
						bool isCounter = false;
						trigger_event triggerEvent = periodic;
						uint32_t io_debounce_ms = DEFAULT_DEBOUNCE_TIME;

						double sensorFactor = 1.0;
						double sensorOffset = 0.0;
						uint64_t sensorMinimumRestPeriod = DEFAULT_MEASUREMENT_INTERVAL;

						try {
							sensorID = s->element("sensor_id")->value()->getString();
						} catch(int e) {
							std::stringstream ss;
							ss << "Sensor #" << (i+1) <<": sensor_id not found in config file.";
							message(ss.str(), true);
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
									ss << "Error: Not a valid io_port : \'" << sioPort <<"\'. The io_port for an IO Bricklet must be either \'a\' or \'b\'. Default to \'a\'.";
									message(ss.str(), true);
									ioPort = 'a';
								}
							}
							else
							{
								std::stringstream ss;
								ss << "Error: Not a valid io_port : \'" << sioPort <<"\'. The io_port for an IO Bricklet must be either \'a\' or \'b\'. Default to \'a\'.";
								message(ss.str(), true);
							}
						} catch(int e) {}

						try {
							isCounter = s->element("counter")->value()->getBool();
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
								message("Error: Unknown trigger type: \'"+triggerText+"\'", true);
							}
						} catch(int e) {}

						try {
							io_debounce_ms = static_cast<uint32_t>(s->element("io_debounce")->durationInMS());
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
								message(ss.str(), false);
							}
						}



						if(sensorJSONfile.size() > 0)  // Create a JSON sensor
						{
							sensorJSON* jsonSensor = new sensorJSON(this, sensorID, mqttPublishTopic, homematicPublishISE, sensorJSONfile, &sensorJSONkeys, isCounter, sensorFactor, sensorOffset, sensorMinimumRestPeriod, _rBuffer);
							_sensors.push_back(jsonSensor);
						}
						else if(tinkerforge_uid.size() > 0)
						{
							sensorTinkerforge* tfSensor = new sensorTinkerforge(this, sensorID, mqttPublishTopic, homematicPublishISE, _tfDaemon, tinkerforge_uid, triggerEvent, isCounter, channel, ioPort, io_debounce_ms, sensorFactor, sensorOffset, sensorMinimumRestPeriod);
							_sensors.push_back(tfSensor);
						}
						else if(mqttSubscribeTopic.size() > 0)
						{
							sensorMQTT* mqttSensor = new sensorMQTT(this, sensorID, mqttPublishTopic, homematicPublishISE, mqttSubscribeTopic, &sensorJSONkeys, isCounter, sensorFactor, sensorOffset, sensorMinimumRestPeriod);
							_sensors.push_back(mqttSensor);
						}
						else if(homematicSubscribeISE.size() > 0)
						{
							sensorHomematic* homematicSensor = new sensorHomematic(this, _homematic, sensorID, mqttPublishTopic, homematicPublishISE, homematicSubscribeISE, isCounter, sensorFactor, sensorOffset, sensorMinimumRestPeriod);
							_sensors.push_back(homematicSensor);
						}
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "Error in configuration for sensor #" << (i+1);
						message(ss.str(), true);
					}
				}
			}
		} catch(int e) {
			message("No valid sensor configuration found.", true);
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
							message(ss.str(), true);
						}

						uint64_t cycleTime = DEFAULT_CYCLETIME;
						try	{
							cycleTime = static_cast<uint64_t>(currentLog->element("cycle_time")->durationInMS());
						} catch(int e) {
							std::stringstream ss;
							ss << "Error in configuration for logbook #" << (i+1) << ": cannot get cycle_time. Default to " << cycleTime<<".";
							message(ss.str(), true);
						}

						unsigned maxEntries = DEFAULT_MAX_ENTRIES;
						try	{
							maxEntries = currentLog->element("max_entries")->value()->getInt();
						} catch(int e) {
							std::stringstream ss;
							ss << "Error in configuration for logbook #" << (i+1) << ": cannot get max_entries. Default to " << maxEntries<<".";
							message(ss.str(), true);
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
										message(ss.str(), true);
										throw e;
									}

									try	{
										colSensor = getSensor(colSensorID);
									} catch(int e) {
										std::stringstream ss;
										ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": sensor \'" << colSensorID << "\' not found.";
										message(ss.str(), true);
										throw e;
									}
									
									std::string title = "";
									try {
										title = currentCol->element("title")->value()->getString();
									} catch(int e) {
										std::stringstream ss;
										ss << "Warning in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": no title found.";
										message(ss.str(), true);
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
											message(ss.str(), true);
										}
									} catch(int e) {
										std::stringstream ss;
										ss << "Error in configuration for logbook #" << (i+1) << ", column #" << (c+1) <<": no operation found.";
										message(ss.str(), true);
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
									message(ss.str(), true);
								}
								

							}
						} catch(int e) {
							std::stringstream ss;
							ss << "Error in configuration for logbook #" << (i+1) << ": error parsing column information.";
							message(ss.str(), true);
						}
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "Error in configuration for logbook #" << (i+1);
						message(ss.str(), true);
					}
				}
			}
			catch(int e)
			{
				message("Error in configuration for the logbooks.", true);
			}
		}

		// MQTT Configuration:
		if(configFile.existAndNotNull("mqtt"))
		{
			if(configFile.element("mqtt")->existAndNotNull("host"))
			{
				try
				{
					std::string mqttHost = configFile.element("mqtt")->element("host")->value()->getString();
					int mqttPort = configFile.element("mqtt")->element("port")->value()->getInt();

					int mqttQoS = 1;
					try
					{
						mqttQoS = configFile.element("mqtt")->element("qos")->value()->getInt();
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "No MQTT QoS defined. Default to " << mqttQoS << ".";
						message(ss.str(), false);
					}

					bool mqttRetained = false;
					try
					{
						mqttRetained = configFile.element("mqtt")->element("retained")->value()->getBool();
					}
					catch(int e)
					{
						std::stringstream ss;
						ss << "No MQTT \'retained\' setting defined. Default to false.";
						message(ss.str(), false);
					}

					std::string mqtt_connected_topic;
					std::string mqtt_connected_payload;
					std::string mqtt_lwt_topic;
					std::string mqtt_lwt_payload;

					try
					{
						mqtt_connected_topic = configFile.element("mqtt")->element("connected_topic")->value()->getString();
					}
					catch(int e) { }

					try
					{
						mqtt_connected_payload = configFile.element("mqtt")->element("connected_message")->value()->getString();
					}
					catch(int e) { }

					try
					{
						mqtt_lwt_topic = configFile.element("mqtt")->element("lwt_topic")->value()->getString();
					}
					catch(int e) { }

					try
					{
						mqtt_lwt_payload = configFile.element("mqtt")->element("lwt_message")->value()->getString();
					}
					catch(int e) { }


					if(mqttHost.size() > 0)
					{
						_mqttBroker->setHost(mqttHost);
						_mqttBroker->setPort(mqttPort);
						_mqttBroker->setQoS(mqttQoS);
						_mqttBroker->setRetained(mqttRetained);
						_mqttBroker->setConnectedTopic(mqtt_connected_topic);
						_mqttBroker->setConnectedPayload(mqtt_connected_payload);
						_mqttBroker->setLWTtopic(mqtt_lwt_topic);
						_mqttBroker->setLWTpayload(mqtt_lwt_payload);
					}
				}
				catch(int e)
				{
					message("Error in MQTT configuration.", true);
				}
			}
		}
	}
	catch(int e)
	{
		message("Error parsing config file: " + configJSON, true);
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
	if(_curl)
	{
		std::string response;

		curl_easy_setopt(_curl, CURLOPT_URL, url.c_str());
		curl_easy_setopt(_curl, CURLOPT_FOLLOWLOCATION, 1L);
		curl_easy_setopt(_curl, CURLOPT_WRITEFUNCTION, receiveHTTP);
		curl_easy_setopt(_curl, CURLOPT_WRITEDATA, &response);

		CURLcode res = curl_easy_perform(_curl);
		if(res != CURLE_OK)
		{
			std::string easyReadError = curl_easy_strerror(res);
			message("HTTP(S) request failed: " + easyReadError, true);

			throw E_HTTP_REQUEST_FAILED;
		}

		return response;
	}

	throw E_HTTP_REQUEST_FAILED;
}

void logger::setUpConnections()
{
	_mqttBroker->connectToMQTTBroker();
}

void logger::executeSystemCommand(const std::string &command)
{
	message("Execute command: "+command, false);

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
			message("Error executing the command.", true);

		if(output.size() > 0)
			message(output, false);
	}
	else
		message("Error executing the command.", true);
}

void logger::mqttPublish(const std::string &topic, const std::string &payload)
{
	if(_mqttBroker != NULL)
		_mqttBroker->publish(topic, payload);
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
			_logbooks.at(i)->write(_mqttBroker, _homematic);
		} catch(int e)
		{
			std::cerr<<"Error "<<e<<" when writing logbook "<<(i+1)<<"."<<std::endl;
		}
	}

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
						message("Too many Tinkerforge Bricklet read failures:", true);
					}

					std::stringstream ss;
					ss << "  Bricklet \'" << s->getUID() << "\' ("<< getDeviceType_name(s->getDeviceType()) << "): " << s->getReadFailures() << " read failures.";
					message(ss.str(), true);

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
				message("Trying to reboot the entire system...", true);
				executeSystemCommand(_cmd_readFailures_critical);
				std::this_thread::sleep_for(std::chrono::seconds(2));
			}

			return;
		}

		if(_cmd_readFailures.size() > 0)
		{
			message("Trying to restart Brick Daemon...", true);
			executeSystemCommand(_cmd_readFailures);
			++_brickd_restart_attempt_counter;
			std::this_thread::sleep_for(std::chrono::seconds(30));
		}
	}	
}