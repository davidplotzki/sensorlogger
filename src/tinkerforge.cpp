#include "tinkerforge.h"

#include "logger.h"
#include "sensor_tinkerforge.h"
#include "sensorlogger.h"

tinkerforge::tinkerforge(logger* root)
{
	_root = root;
	_ipcon = NULL;
	_timeout = DEFAULT_TINKERFORGE_TIMEOUT;
	disconnect_and_prepare();
}

tinkerforge::tinkerforge(logger*root, const std::string &host, unsigned short port)
{
	_root = root;
	_ipcon = NULL;
	_timeout = DEFAULT_TINKERFORGE_TIMEOUT;
	disconnect_and_prepare();

	setHost(host);
	setPort(port);
}

tinkerforge::~tinkerforge()
{
	ipcon_disconnect(_ipcon);
	ipcon_destroy(_ipcon);
	delete _ipcon;
}

void tinkerforge::setHost(const std::string &host)
{
	_host = host;
}

void tinkerforge::setPort(unsigned short port)
{
	_port = port;
}

std::string tinkerforge::getHost() const
{
	return _host;
}

unsigned short tinkerforge::getPort() const
{
	return _port;
}

bool tinkerforge::disconnect()
{
	if(_ipcon != NULL)
	{
		ipcon_disconnect(_ipcon);
		ipcon_destroy(_ipcon);
		delete _ipcon;
	}

	return true;
}

bool tinkerforge::disconnect_and_prepare()
{
	if(disconnect())
	{
		_ipcon = new IPConnection();
		ipcon_create(_ipcon);
		ipcon_set_timeout(_ipcon, _timeout);
		ipcon_set_auto_reconnect(_ipcon, false);
	}

	return true;
}

bool tinkerforge::reconnect()
{
	if(_host.size() > 0)
	{
		if(ipcon_get_connection_state(_ipcon) == IPCON_CONNECTION_STATE_DISCONNECTED)
		{
			disconnect_and_prepare();

			// Connect to brickd
			if(ipcon_connect(_ipcon, _host.c_str(), _port) < 0)
			{
				std::stringstream ss;
				ss << "Unable to connect to Tinkerforge Brick Daemon at " << _host << ":" << _port << ".";
				_root->message(ss.str(), true);
				disconnect_and_prepare();

				return false;
			}
			else
			{
				std::stringstream ss;
				ss << "Successfully connected to Tinkerforge Brick Daemon at " << _host << ":" << _port << ".";
				_root->message(ss.str(), false);
				
				_root->message("Enumerating Tinkerforge Bricklets...", false);
				ipcon_register_callback(_ipcon, IPCON_CALLBACK_ENUMERATE, (void (*)(void))enumerateTFSensors, _root);
				ipcon_enumerate(_ipcon);
				std::this_thread::sleep_for(std::chrono::milliseconds(10000));
				_root->message("Enumeration of Tinkerforge Bricklets done.", false);

				/*
				_root->message("Registering Tinkerforge callbacks...", false);
				for(size_t i=0; i<_root->nSensors(); ++i)
				{
					if(_root->getSensor(i)->type() == sensor_tinkerforge)
					{
						sensorTinkerforge* tfSensor = dynamic_cast<sensorTinkerforge*>(_root->getSensor(i));

						if(tfSensor->getTriggerEvent() != periodic)
						{
							std::stringstream cbss;
							cbss << "  Register callback for Bricklet \'" << tfSensor->getUID() << "\' (" << getDeviceType_name(tfSensor->getDeviceType()) << "), debounce period: "<< tfSensor->getDebounceTime() << " ms.";
							_root->message(cbss.str(), false);

							tfSensor->registerCallback();
						}
					}
				}
				_root->message("Registration of Tinkerforge callbacks done.", false);
				*/

				return true;
			}
		}
		else if(ipcon_get_connection_state(_ipcon) == IPCON_CONNECTION_STATE_PENDING)
		{
			std::stringstream ss_state;
			ss_state << "Connecting to Tinkerforge Brick Daemon at " << _host << ":" << _port << "...";
			_root->message(ss_state.str(), false);

			return false;
		}

		return true;
	}

	return false;
}

std::string getTFConnectionErrorText(int e)
{
	switch(e)
	{
		case(E_TIMEOUT):
			return "Timeout";
			break;
		case(E_NO_STREAM_SOCKET):
			return "No Stream Socket";
			break;
		case(E_HOSTNAME_INVALID):
			return "Invalid Hostname";
			break;
		case(E_NO_CONNECT):
			return "No Connection";
			break;
		case(E_NO_THREAD):
			return "No Thread";
			break;
		case(E_ALREADY_CONNECTED):
			return "Already Connected";
			break;
		case(E_NOT_CONNECTED):
			return "Not Connected";
			break;
		case(E_INVALID_PARAMETER):
			return "Invalid Parameter";
			break;
		case(E_NOT_SUPPORTED):
			return "Not Supported";
			break;
		case(E_UNKNOWN_ERROR_CODE):
			return "Unknown Error Code";
			break;
		case(E_STREAM_OUT_OF_SYNC):
			return "Stream out of Sync";
			break;
		case(E_INVALID_UID):
			return "Invalid UID";
			break;
		case(E_NON_ASCII_CHAR_IN_SECRET):
			return "Non-ASCII character in secret";
			break;
		case(E_WRONG_DEVICE_TYPE):
			return "Wrong Device Type";
			break;
		case(E_DEVICE_REPLACED):
			return "Device Replaced";
			break;
		case(E_WRONG_RESPONSE_LENGTH):
			return "Wrong Response Length";
			break;
	}

	return "Unknown Error";
}

std::string getDeviceType_name(uint16_t device_identifier)
{
	switch(device_identifier)
	{
		case(TEMPERATURE_DEVICE_IDENTIFIER):
			return TEMPERATURE_DEVICE_DISPLAY_NAME; break;
		case(TEMPERATURE_V2_DEVICE_IDENTIFIER):
			return TEMPERATURE_V2_DEVICE_DISPLAY_NAME; break;
		case(IO4_DEVICE_IDENTIFIER):
			return IO4_DEVICE_DISPLAY_NAME; break;
		case(IO4_V2_DEVICE_IDENTIFIER):
			return IO4_V2_DEVICE_DISPLAY_NAME; break;
		case(IO16_DEVICE_IDENTIFIER):
			return IO16_DEVICE_DISPLAY_NAME; break;
		case(IO16_V2_DEVICE_IDENTIFIER):
			return IO16_V2_DEVICE_DISPLAY_NAME; break;    
 		case(HUMIDITY_DEVICE_IDENTIFIER):
			return HUMIDITY_DEVICE_DISPLAY_NAME; break;
		case(HUMIDITY_V2_DEVICE_IDENTIFIER):
			return HUMIDITY_V2_DEVICE_DISPLAY_NAME; break;
		case(BAROMETER_DEVICE_IDENTIFIER):
			return BAROMETER_DEVICE_DISPLAY_NAME; break;
		case(BAROMETER_V2_DEVICE_IDENTIFIER):
			return BAROMETER_V2_DEVICE_DISPLAY_NAME; break;
		case(ANALOG_IN_DEVICE_IDENTIFIER):
			return ANALOG_IN_DEVICE_DISPLAY_NAME; break;
		case(ANALOG_IN_V2_DEVICE_IDENTIFIER):
			return ANALOG_IN_V2_DEVICE_DISPLAY_NAME; break;
		case(ANALOG_IN_V3_DEVICE_IDENTIFIER):
			return ANALOG_IN_V3_DEVICE_DISPLAY_NAME; break;
		case(AMBIENT_LIGHT_DEVICE_IDENTIFIER):
			return AMBIENT_LIGHT_DEVICE_DISPLAY_NAME; break;
		case(AMBIENT_LIGHT_V2_DEVICE_IDENTIFIER):
			return AMBIENT_LIGHT_V2_DEVICE_DISPLAY_NAME; break;
		case(AMBIENT_LIGHT_V3_DEVICE_IDENTIFIER):
			return AMBIENT_LIGHT_V3_DEVICE_DISPLAY_NAME; break;
		case(PTC_DEVICE_IDENTIFIER):
			return PTC_DEVICE_DISPLAY_NAME; break;
		case(PTC_V2_DEVICE_IDENTIFIER):
			return PTC_V2_DEVICE_DISPLAY_NAME; break;
		case(MOISTURE_DEVICE_IDENTIFIER):
			return MOISTURE_DEVICE_DISPLAY_NAME; break;
		case(AIR_QUALITY_DEVICE_IDENTIFIER):
			return AIR_QUALITY_DEVICE_DISPLAY_NAME; break;
		case(CO2_DEVICE_IDENTIFIER):
			return CO2_DEVICE_DISPLAY_NAME; break;
		case(CO2_V2_DEVICE_IDENTIFIER):
			return CO2_V2_DEVICE_DISPLAY_NAME; break;
		case(CURRENT12_DEVICE_IDENTIFIER):
			return CURRENT12_DEVICE_DISPLAY_NAME; break;
		case(CURRENT25_DEVICE_IDENTIFIER):
			return CURRENT25_DEVICE_DISPLAY_NAME; break;
		case(DISTANCE_IR_DEVICE_IDENTIFIER):
			return DISTANCE_IR_DEVICE_DISPLAY_NAME; break;
		case(DISTANCE_IR_V2_DEVICE_IDENTIFIER):
			return DISTANCE_IR_V2_DEVICE_DISPLAY_NAME; break;
		case(DISTANCE_US_DEVICE_IDENTIFIER):
			return DISTANCE_US_DEVICE_DISPLAY_NAME; break;
		case(DISTANCE_US_V2_DEVICE_IDENTIFIER):
			return DISTANCE_US_V2_DEVICE_DISPLAY_NAME; break;
		case(DUST_DETECTOR_DEVICE_IDENTIFIER):
			return DUST_DETECTOR_DEVICE_DISPLAY_NAME; break;
		case(ENERGY_MONITOR_DEVICE_IDENTIFIER):
			return ENERGY_MONITOR_DEVICE_DISPLAY_NAME; break;
		case(INDUSTRIAL_DIGITAL_IN_4_DEVICE_IDENTIFIER):
			return INDUSTRIAL_DIGITAL_IN_4_DEVICE_DISPLAY_NAME; break;
		case(INDUSTRIAL_DIGITAL_IN_4_V2_DEVICE_IDENTIFIER):
			return INDUSTRIAL_DIGITAL_IN_4_V2_DEVICE_DISPLAY_NAME; break;
		case(INDUSTRIAL_DUAL_0_20MA_DEVICE_IDENTIFIER):
			return INDUSTRIAL_DUAL_0_20MA_DEVICE_DISPLAY_NAME; break;
		case(INDUSTRIAL_DUAL_0_20MA_V2_DEVICE_IDENTIFIER):
			return INDUSTRIAL_DUAL_0_20MA_V2_DEVICE_DISPLAY_NAME; break;
		case(INDUSTRIAL_DUAL_ANALOG_IN_DEVICE_IDENTIFIER):
			return INDUSTRIAL_DUAL_ANALOG_IN_DEVICE_DISPLAY_NAME; break;
		case(INDUSTRIAL_DUAL_ANALOG_IN_V2_DEVICE_IDENTIFIER):
			return INDUSTRIAL_DUAL_ANALOG_IN_V2_DEVICE_DISPLAY_NAME; break;
		case(LASER_RANGE_FINDER_DEVICE_IDENTIFIER):
			return LASER_RANGE_FINDER_DEVICE_DISPLAY_NAME; break;
		case(LASER_RANGE_FINDER_V2_DEVICE_IDENTIFIER):
			return LASER_RANGE_FINDER_V2_DEVICE_DISPLAY_NAME; break;
		case(LINE_DEVICE_IDENTIFIER):
			return LINE_DEVICE_DISPLAY_NAME; break;
		case(LOAD_CELL_DEVICE_IDENTIFIER):
			return LOAD_CELL_DEVICE_DISPLAY_NAME; break;
		case(LOAD_CELL_V2_DEVICE_IDENTIFIER):
			return LOAD_CELL_V2_DEVICE_DISPLAY_NAME; break;
		case(PARTICULATE_MATTER_DEVICE_IDENTIFIER):
			return PARTICULATE_MATTER_DEVICE_DISPLAY_NAME; break;
		case(SOUND_INTENSITY_DEVICE_IDENTIFIER):
			return SOUND_INTENSITY_DEVICE_DISPLAY_NAME; break;
		case(SOUND_PRESSURE_LEVEL_DEVICE_IDENTIFIER):
			return SOUND_PRESSURE_LEVEL_DEVICE_DISPLAY_NAME; break;
		case(TEMPERATURE_IR_DEVICE_IDENTIFIER):
			return TEMPERATURE_IR_DEVICE_DISPLAY_NAME; break;
		case(TEMPERATURE_IR_V2_DEVICE_IDENTIFIER):
			return TEMPERATURE_IR_V2_DEVICE_DISPLAY_NAME; break;
		case(UV_LIGHT_DEVICE_IDENTIFIER):
			return UV_LIGHT_DEVICE_DISPLAY_NAME; break;
		case(UV_LIGHT_V2_DEVICE_IDENTIFIER):
			return UV_LIGHT_V2_DEVICE_DISPLAY_NAME; break;
		case(VOLTAGE_DEVICE_IDENTIFIER):
			return VOLTAGE_DEVICE_DISPLAY_NAME; break;
		case(VOLTAGE_CURRENT_DEVICE_IDENTIFIER):
			return VOLTAGE_CURRENT_DEVICE_DISPLAY_NAME; break;
		case(VOLTAGE_CURRENT_V2_DEVICE_IDENTIFIER):
			return VOLTAGE_CURRENT_V2_DEVICE_DISPLAY_NAME; break;
	}

	return "Unknown";
}

std::string getDeviceType_nice(uint16_t device_identifier)
{
	std::stringstream ss;
	ss << device_identifier << " (" << getDeviceType_name(device_identifier) << ")";
	return ss.str();
}



void enumerateTFSensors(const char *uid, const char *connected_uid, char position, uint8_t hardware_version[3], uint8_t firmware_version[3], uint16_t device_identifier, uint8_t enumeration_type, logger* _root)
{
	if(device_identifier > 20)
	{
		std::stringstream ss;
		ss<<"Found Bricklet \'"<<std::string(uid)<<"\' with device identifier "<<getDeviceType_nice(device_identifier) <<".";
		_root->message(ss.str(), false);

		for(size_t s=0; s<_root->nSensors(); ++s)
		{
			// Check if this is a Tinkerforge sensor:
			if(_root->getSensor(s)->type() == sensor_tinkerforge)
			{
				sensorTinkerforge* tfsensor = dynamic_cast<sensorTinkerforge*>(_root->getSensor(s));

				if(tfsensor->getUID() == std::string(uid))
				{
					tfsensor->setDeviceType(static_cast<unsigned>(device_identifier));
					if(tfsensor->getTriggerEvent() != periodic)
					{
						std::stringstream cbss;
						cbss << "  Register callback for Bricklet \'" << tfsensor->getUID() << "\' (" << getDeviceType_name(tfsensor->getDeviceType()) << "), debounce period: "<< tfsensor->getDebounceTime() << " ms.";
						_root->message(cbss.str(), false);

						tfsensor->registerCallback();
					}
				}
			}
		}
	}
}
