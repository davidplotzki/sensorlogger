#include "sensor_tinkerforge.h"

#include "logger.h"
#include "measurements.h"

sensorTinkerforge::sensorTinkerforge(logger* root, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, tinkerforge* tinkerManager, const std::string uid, trigger_event triggerEvent, bool isCounter, uint8_t channel, char ioPort, uint32_t debounceTime, double factor, double offset, uint64_t minimumRestPeriod)
{
	setPointerToLogger(root);
	_isInitialized = false;
	_tinkerMan = tinkerManager;
	_tinkerforgeCallback = NULL;

	setSensorID(sensorID);
	setUID(uid);
	setChannel(channel);
	setIOPort(ioPort);
	setTriggerEvent(triggerEvent);
	setCounter(isCounter);
	setDebounceTime(debounceTime);
	setDeviceType(0);

	setFactor(factor);
	setOffset(offset);
	setMinimumRestPeriod(minimumRestPeriod);

	setMQTTPublishTopic(mqttPublishTopic);
	setHomematicPublishISE(homematicPublishISE);
}

sensorTinkerforge::~sensorTinkerforge()
{

}

sensor_type sensorTinkerforge::type() const
{
	return sensor_tinkerforge;
}

std::string sensorTinkerforge::getUID() const
{
	return _uid;
}

unsigned sensorTinkerforge::getDeviceType() const
{
	return _deviceType;
}

uint16_t sensorTinkerforge::getBitMask() const
{
	return _bitMask;
}

uint8_t sensorTinkerforge::getChannel() const
{
	return _channel;
}

char sensorTinkerforge::getIOPort() const
{
	return _ioPort;
}

uint32_t sensorTinkerforge::getDebounceTime() const
{
	return _debounceTime;
}


void sensorTinkerforge::setUID(const std::string &uid)
{
	_uid = uid;
}

void sensorTinkerforge::setDeviceType(unsigned deviceType)
{
	_deviceType = deviceType;
	_isInitialized = true;
}

void sensorTinkerforge::setChannel(uint8_t channel)
{
	_channel = channel;

	uint16_t one = 1;
	_bitMask = (one << _channel);
}

void sensorTinkerforge::setIOPort(char ioPort)
{
	_ioPort = 'a';

	if((ioPort == 'B') || (ioPort == 'b'))
		_ioPort = 'b';
}

void sensorTinkerforge::setDebounceTime(uint32_t debounceTime)
{
	_debounceTime = debounceTime;
}

void sensorTinkerforge::registerCallback()
{
	if(getTriggerEvent() != periodic)
	{
		if(_tinkerforgeCallback != NULL)
			delete _tinkerforgeCallback;

		if(ipcon_get_connection_state(_tinkerMan->_ipcon) == IPCON_CONNECTION_STATE_CONNECTED)
		{
			try
			{
				if(_deviceType == IO4_DEVICE_IDENTIFIER)
					_tinkerforgeCallback = new tinkerforge_callback_io4(this, _tinkerMan);
				else if(_deviceType == IO4_V2_DEVICE_IDENTIFIER)
					_tinkerforgeCallback = new tinkerforge_callback_io4_v2(this, _tinkerMan);
				else if(_deviceType == IO16_DEVICE_IDENTIFIER)
					_tinkerforgeCallback = new tinkerforge_callback_io16(this, _tinkerMan);
				else if(_deviceType == IO16_V2_DEVICE_IDENTIFIER)
					_tinkerforgeCallback = new tinkerforge_callback_io16_v2(this, _tinkerMan);
				else
				{
					std::stringstream ss;
					ss << "Cannot register callback for unknown Bricklet \'" << getUID() << "\' (";
					ss << getDeviceType_name(getDeviceType());
					ss << ").";
					_root->message(ss.str(), true);
					return;
				}

				// Before registering the callback, do a first measurement (poll) of the current state,
				// just in case this is supposed to be broadcast via MQTT.
				// Reset the counters, because we didn't actually count anything.
				try {
					poll(_root->currentTimestamp());
					resetCounters();
				} catch(int e) {
					if(e == E_TIMEOUT)
					{
						// Wait a little bit and try again...
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						poll(_root->currentTimestamp());
						resetCounters();
					}
					else throw e;
				}

				try	{
					_tinkerforgeCallback->registerCallback();
				} catch(int e)
				{
					if(e == E_TIMEOUT)
					{
						// Wait a little bit and try again...
						std::this_thread::sleep_for(std::chrono::milliseconds(5000));
						_tinkerforgeCallback->registerCallback();
					}
					else throw e;
				}
			}
			catch(int e)
			{
				std::stringstream ss;
				ss << "Error registering callback for Bricklet \'" << getUID() << "\' (";
				ss << getDeviceType_name(getDeviceType());
				ss << "): " << getTFConnectionErrorText(e);
				_root->message(ss.str(), true);
			}
		}
	}
}

void sensorTinkerforge::failWithReadError(int tf_error_code)
{
	std::stringstream ss;
	ss << "Read Error: Bricklet \'" << getUID() << "\' (" << getDeviceType_name(getDeviceType()) << "). ";
	ss << "Tinkerforge Error Code " << tf_error_code << ": " << getTFConnectionErrorText(tf_error_code) << ".";
	_root->message(ss.str(), true);

	addReadFailure();
}

bool sensorTinkerforge::poll(uint64_t currentTimestamp)
{
	if(_tinkerMan->reconnect())
	{
		if(_isInitialized)
		{
			switch(_deviceType)
			{
				case(AIR_QUALITY_DEVICE_IDENTIFIER):
				{
					AirQuality aq;
					air_quality_create(&aq, _uid.c_str(), _tinkerMan->_ipcon);
					
					int32_t iaq_index, temperature, humidity, air_pressure;
					uint8_t iaq_index_accuracy;

					int result = air_quality_get_all_values(&aq, &iaq_index, &iaq_index_accuracy, &temperature, &humidity, &air_pressure);

					air_quality_destroy(&aq);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue;
						double value = 0;

						switch(getChannel())
						{
							case(1): // Temperature
								ivalue = static_cast<int>(temperature);
								value = static_cast<double>(ivalue)/100.0;
								break;
							case(2): // Humidity
								ivalue = static_cast<int>(humidity);
								value = static_cast<double>(ivalue)/100.0;
								break;
							case(3): // Air pressure
								ivalue = static_cast<int>(air_pressure);
								value = static_cast<double>(ivalue)/100.0;
								break;
							default: // IAQ index
								ivalue = static_cast<int>(iaq_index);
								value = static_cast<double>(ivalue);
								break;
						}  
							
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(CO2_DEVICE_IDENTIFIER):
				{
					CO2 co2;
					co2_create(&co2, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t co2_concentration;
					int result = co2_get_co2_concentration(&co2, &co2_concentration);
					co2_destroy(&co2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(co2_concentration);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(CO2_V2_DEVICE_IDENTIFIER):
				{
					CO2V2 co2;
					co2_v2_create(&co2, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t co2_concentration, humidity;
					int16_t temperature;

					int result = co2_v2_get_all_values(&co2, &co2_concentration, &temperature, &humidity);

					co2_v2_destroy(&co2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue;
						double value = 0;

						switch(getChannel())
						{
							case(1): // Temperature
								ivalue = static_cast<int>(temperature);
								value = static_cast<double>(ivalue)/100.0;
								break;
							case(2): // Humidity
								ivalue = static_cast<int>(humidity);
								value = static_cast<double>(ivalue)/100.0;
								break;
							default: // CO2 concentration
								ivalue = static_cast<int>(co2_concentration);
								value = static_cast<double>(ivalue);
								break;
						}

						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(CURRENT12_DEVICE_IDENTIFIER):
				{
					Current12 c;
					current12_create(&c, _uid.c_str(), _tinkerMan->_ipcon);
					
					int16_t current;
					int result = current12_get_current(&c, &current);

					current12_destroy(&c);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(current);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(CURRENT25_DEVICE_IDENTIFIER):
				{
					Current25 c;
					current25_create(&c, _uid.c_str(), _tinkerMan->_ipcon);
					
					int16_t current;
					int result = current25_get_current(&c, &current);

					current25_destroy(&c);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(current);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(DISTANCE_IR_DEVICE_IDENTIFIER):
				{
					DistanceIR dir;
					distance_ir_create(&dir, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t distance;
					int result = distance_ir_get_distance(&dir, &distance);

					distance_ir_destroy(&dir);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(distance);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(DISTANCE_IR_V2_DEVICE_IDENTIFIER):
				{
					DistanceIRV2 dir;
					distance_ir_v2_create(&dir, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t distance;
					int result = distance_ir_v2_get_distance(&dir, &distance);

					distance_ir_v2_destroy(&dir);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(distance);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(DISTANCE_US_DEVICE_IDENTIFIER):
				{
					DistanceUS dus;
					distance_us_create(&dus, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t distance;
					int result = distance_us_get_distance_value(&dus, &distance);

					distance_us_destroy(&dus);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(distance);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(DISTANCE_US_V2_DEVICE_IDENTIFIER):
				{
					DistanceUSV2 dus;
					distance_us_v2_create(&dus, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t distance;
					int result = distance_us_v2_get_distance(&dus, &distance);

					distance_us_v2_destroy(&dus);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(distance);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(DUST_DETECTOR_DEVICE_IDENTIFIER):
				{
					DustDetector dd;
					dust_detector_create(&dd, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t dust_density;
					int result = dust_detector_get_dust_density(&dd, &dust_density);

					dust_detector_destroy(&dd);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(dust_density);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(ENERGY_MONITOR_DEVICE_IDENTIFIER):
				{
					EnergyMonitor em;
					energy_monitor_create(&em, _uid.c_str(), _tinkerMan->_ipcon);
					
					int32_t voltage, current, energy, real_power, apparent_power, reactive_power;
    				uint16_t power_factor, frequency;
					int result = energy_monitor_get_energy_data(&em, &voltage, &current, &energy, &real_power, &apparent_power, &reactive_power, &power_factor, &frequency);

					energy_monitor_destroy(&em);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue;
						double value = 0;

						switch(getChannel())
						{
							case(1): // Current
								ivalue = static_cast<int>(current);
								value = static_cast<double>(ivalue) / 100.0;
								break;
							case(2): // Energy
								ivalue = static_cast<int>(energy);
								value = static_cast<double>(ivalue) / 100.0;
								break;
							case(3): // Real Power
								ivalue = static_cast<int>(real_power);
								value = static_cast<double>(ivalue) / 100.0;
								break;
							case(4): // Apparent Power
								ivalue = static_cast<int>(apparent_power);
								value = static_cast<double>(ivalue) / 100.0;
								break;
							case(5): // Reactive Power
								ivalue = static_cast<int>(reactive_power);
								value = static_cast<double>(ivalue) / 100.0;
								break;
							case(6): // Power Factor
								ivalue = static_cast<int>(power_factor);
								value = static_cast<double>(ivalue) / 1000.0;
								break;
							case(7): // Frequency
								ivalue = static_cast<int>(frequency);
								value = static_cast<double>(ivalue) / 100.0;
								break;
							default: // Voltage
								ivalue = static_cast<int>(voltage);
								value = static_cast<double>(ivalue) / 100.0;
								break;
						}

						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(INDUSTRIAL_DUAL_0_20MA_DEVICE_IDENTIFIER):
				{
					IndustrialDual020mA id020;
					industrial_dual_0_20ma_create(&id020, _uid.c_str(), _tinkerMan->_ipcon);
					
					int result;
					int32_t current;
					if(getChannel() == 1)
						result = industrial_dual_0_20ma_get_current(&id020, 1, &current);
					else
						result = industrial_dual_0_20ma_get_current(&id020, 0, &current);

					industrial_dual_0_20ma_destroy(&id020);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(current);
						double value = static_cast<double>(ivalue)/1000000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(INDUSTRIAL_DUAL_0_20MA_V2_DEVICE_IDENTIFIER):
				{
					IndustrialDual020mAV2 id020;
					industrial_dual_0_20ma_v2_create(&id020, _uid.c_str(), _tinkerMan->_ipcon);
					
					int result;
					int32_t current;
					if(getChannel() == 1)
						result = industrial_dual_0_20ma_v2_get_current(&id020, 1, &current);
					else
						result = industrial_dual_0_20ma_v2_get_current(&id020, 0, &current);

					industrial_dual_0_20ma_v2_destroy(&id020);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(current);
						double value = static_cast<double>(ivalue)/1000000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(INDUSTRIAL_DUAL_ANALOG_IN_DEVICE_IDENTIFIER):
				{
					IndustrialDualAnalogIn idai;
					industrial_dual_analog_in_create(&idai, _uid.c_str(), _tinkerMan->_ipcon);
					
					int result;
					int32_t voltage;
					if(getChannel() == 1)
						result = industrial_dual_analog_in_get_voltage(&idai, 1, &voltage);
					else
						result = industrial_dual_analog_in_get_voltage(&idai, 0, &voltage);

					industrial_dual_analog_in_destroy(&idai);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(voltage);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(INDUSTRIAL_DUAL_ANALOG_IN_V2_DEVICE_IDENTIFIER):
				{
					IndustrialDualAnalogInV2 idai;
					industrial_dual_analog_in_v2_create(&idai, _uid.c_str(), _tinkerMan->_ipcon);
					
					int result;
					int32_t voltage;
					if(getChannel() == 1)
						result = industrial_dual_analog_in_v2_get_voltage(&idai, 1, &voltage);
					else
						result = industrial_dual_analog_in_v2_get_voltage(&idai, 0, &voltage);

					industrial_dual_analog_in_v2_destroy(&idai);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(voltage);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(LASER_RANGE_FINDER_DEVICE_IDENTIFIER):
				{
					LaserRangeFinder lrf;
					laser_range_finder_create(&lrf, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t distance;

					laser_range_finder_enable_laser(&lrf);
    				std::this_thread::sleep_for(std::chrono::milliseconds(300));

					int result = laser_range_finder_get_distance(&lrf, &distance);
					laser_range_finder_disable_laser(&lrf);

					laser_range_finder_destroy(&lrf);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(distance);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(LASER_RANGE_FINDER_V2_DEVICE_IDENTIFIER):
				{
					LaserRangeFinderV2 lrf;
					laser_range_finder_v2_create(&lrf, _uid.c_str(), _tinkerMan->_ipcon);

					int16_t distance;

					laser_range_finder_v2_set_enable(&lrf, true);
    				std::this_thread::sleep_for(std::chrono::milliseconds(300));

					int result = laser_range_finder_v2_get_distance(&lrf, &distance);
					laser_range_finder_v2_set_enable(&lrf, false);

					laser_range_finder_v2_destroy(&lrf);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(distance);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(LINE_DEVICE_IDENTIFIER):
				{
					Line l;
					line_create(&l, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t reflectivity;
					int result = line_get_reflectivity(&l, &reflectivity);
					line_destroy(&l);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(reflectivity);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(LOAD_CELL_DEVICE_IDENTIFIER):
				{
					LoadCell lc;
					load_cell_create(&lc, _uid.c_str(), _tinkerMan->_ipcon);

					int32_t weight;
					int result = load_cell_get_weight(&lc, &weight);
					load_cell_destroy(&lc);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(weight);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(LOAD_CELL_V2_DEVICE_IDENTIFIER):
				{
					LoadCellV2 lc;
					load_cell_v2_create(&lc, _uid.c_str(), _tinkerMan->_ipcon);

					int32_t weight;
					int result = load_cell_v2_get_weight(&lc, &weight);
					load_cell_v2_destroy(&lc);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(weight);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(PARTICULATE_MATTER_DEVICE_IDENTIFIER):
				{
					ParticulateMatter pm;
					particulate_matter_create(&pm, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t pm10, pm25, pm100;
					int result = particulate_matter_get_pm_concentration(&pm, &pm10, &pm25, &pm100);
					particulate_matter_destroy(&pm);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue;
						double value = 0;

						switch(getChannel())
						{
							case(2):
							case(25): // PM25
								ivalue = static_cast<int>(pm25);
								value = static_cast<double>(ivalue);
								break;
							case(3):
							case(100): // PM100
								ivalue = static_cast<int>(pm100);
								value = static_cast<double>(ivalue);
								break;
							default: // PM10
								ivalue = static_cast<int>(pm10);
								value = static_cast<double>(ivalue);
								break;
						}  
							
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(SOUND_INTENSITY_DEVICE_IDENTIFIER):
				{
					SoundIntensity si;
					sound_intensity_create(&si, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t intensity;
					int result = sound_intensity_get_intensity(&si, &intensity);

					sound_intensity_destroy(&si);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(intensity);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(SOUND_PRESSURE_LEVEL_DEVICE_IDENTIFIER):
				{
					SoundPressureLevel spl;
					sound_pressure_level_create(&spl, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t decibel;
					int result = sound_pressure_level_get_decibel(&spl, &decibel);

					sound_pressure_level_destroy(&spl);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(decibel);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(TEMPERATURE_IR_DEVICE_IDENTIFIER):
				{
					TemperatureIR tir;
					temperature_ir_create(&tir, _uid.c_str(), _tinkerMan->_ipcon);
					
					int16_t temperature;
					int result;

					if(getChannel() == 1)
						result = temperature_ir_get_object_temperature(&tir, &temperature);
					else
						result = temperature_ir_get_ambient_temperature(&tir, &temperature);

					temperature_ir_destroy(&tir);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(temperature);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(TEMPERATURE_IR_V2_DEVICE_IDENTIFIER):
				{
					TemperatureIRV2 tir;
					temperature_ir_v2_create(&tir, _uid.c_str(), _tinkerMan->_ipcon);
					
					int16_t temperature;
					int result;

					if(getChannel() == 1)
						result = temperature_ir_v2_get_object_temperature(&tir, &temperature);
					else
						result = temperature_ir_v2_get_ambient_temperature(&tir, &temperature);

					temperature_ir_v2_destroy(&tir);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(temperature);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(UV_LIGHT_DEVICE_IDENTIFIER):
				{
					UVLight uvl;
					uv_light_create(&uvl, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint32_t uv_light;
					int result = uv_light_get_uv_light(&uvl, &uv_light);

					uv_light_destroy(&uvl);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(uv_light);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(UV_LIGHT_V2_DEVICE_IDENTIFIER):
				{
					UVLightV2 uvl;
					uv_light_v2_create(&uvl, _uid.c_str(), _tinkerMan->_ipcon);
					
					int32_t uv_light;
					int result;
					if(getChannel() == 1)  // UV-B
						result = uv_light_v2_get_uvb(&uvl, &uv_light);
					else if(getChannel() == 2)  // UV-Index
						result = uv_light_v2_get_uvi(&uvl, &uv_light);
					else // UV-A
						result = uv_light_v2_get_uva(&uvl, &uv_light);

					uv_light_v2_destroy(&uvl);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(uv_light);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(VOLTAGE_DEVICE_IDENTIFIER):
				{
					Voltage v;
					voltage_create(&v, _uid.c_str(), _tinkerMan->_ipcon);
					
					uint16_t voltage;
					int result = voltage_get_voltage(&v, &voltage);

					voltage_destroy(&v);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(voltage);
						double value = static_cast<double>(ivalue)/100.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(VOLTAGE_CURRENT_DEVICE_IDENTIFIER):
				{
					VoltageCurrent vc;
					voltage_current_create(&vc, _uid.c_str(), _tinkerMan->_ipcon);
					
					int32_t meas;
					int result;
					if(getChannel() == 1)  // current
						result = voltage_current_get_current(&vc, &meas);
					else // voltage
						result = voltage_current_get_voltage(&vc, &meas);

					voltage_current_destroy(&vc);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(meas);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(VOLTAGE_CURRENT_V2_DEVICE_IDENTIFIER):
				{
					VoltageCurrentV2 vc;
					voltage_current_v2_create(&vc, _uid.c_str(), _tinkerMan->_ipcon);
					
					int32_t meas;
					int result;
					if(getChannel() == 1)  // current
						result = voltage_current_v2_get_current(&vc, &meas);
					else // voltage
						result = voltage_current_v2_get_voltage(&vc, &meas);

					voltage_current_v2_destroy(&vc);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(meas);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(TEMPERATURE_DEVICE_IDENTIFIER):
				{
					Temperature t;
					temperature_create(&t, _uid.c_str(), _tinkerMan->_ipcon);
					
					int16_t T;
					int result = temperature_get_temperature(&t, &T);

					temperature_destroy(&t);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(T);
						double value = static_cast<double>(ivalue)/100.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(TEMPERATURE_V2_DEVICE_IDENTIFIER):
				{
					TemperatureV2 t2;
					temperature_v2_create(&t2, _uid.c_str(), _tinkerMan->_ipcon);
					
					int16_t T2;
					int result = temperature_v2_get_temperature(&t2, &T2);

					temperature_v2_destroy(&t2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(T2);
						double value = static_cast<double>(ivalue)/100.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(ANALOG_IN_DEVICE_IDENTIFIER):
				{
					AnalogIn ai;
					analog_in_create(&ai, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t V;
					int result = analog_in_get_voltage(&ai, &V);

					analog_in_destroy(&ai);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(V);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(ANALOG_IN_V2_DEVICE_IDENTIFIER):
				{
					AnalogInV2 ai2;
					analog_in_v2_create(&ai2, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t V2;
					int result = analog_in_v2_get_voltage(&ai2, &V2);

					analog_in_v2_destroy(&ai2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(V2);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(ANALOG_IN_V3_DEVICE_IDENTIFIER):
				{
					AnalogInV3 ai3;
					analog_in_v3_create(&ai3, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t V3;
					int result = analog_in_v3_get_voltage(&ai3, &V3);

					analog_in_v3_destroy(&ai3);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(V3);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(HUMIDITY_DEVICE_IDENTIFIER):
				{
					Humidity h;
					humidity_create(&h, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t H;
					int result = humidity_get_humidity(&h, &H);

					humidity_destroy(&h);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(H);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(HUMIDITY_V2_DEVICE_IDENTIFIER):
				{
					HumidityV2 h2;
					humidity_v2_create(&h2, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t H2;
					int result = humidity_v2_get_humidity(&h2, &H2);

					humidity_v2_destroy(&h2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(H2);
						double value = static_cast<double>(ivalue)/100.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(BAROMETER_DEVICE_IDENTIFIER):
				{
					Barometer b;
					barometer_create(&b, _uid.c_str(), _tinkerMan->_ipcon); 

					int32_t air_pressure;
					int result = barometer_get_air_pressure(&b, &air_pressure);

					barometer_destroy(&b);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(air_pressure);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(BAROMETER_V2_DEVICE_IDENTIFIER):
				{
					BarometerV2 b2;
					barometer_v2_create(&b2, _uid.c_str(), _tinkerMan->_ipcon); 

					int32_t air_pressure2;
					int result = barometer_v2_get_air_pressure(&b2, &air_pressure2);

					barometer_v2_destroy(&b2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(air_pressure2);
						double value = static_cast<double>(ivalue)/1000.0;
						addRawMeasurement(value);
					}

					return true;
					break;
				}

				case(AMBIENT_LIGHT_DEVICE_IDENTIFIER):
				{
					AmbientLight al;
					ambient_light_create(&al, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t light;
					int result = ambient_light_get_illuminance(&al, &light);

					ambient_light_destroy(&al);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(light);
						double value = static_cast<double>(ivalue)/10.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(AMBIENT_LIGHT_V2_DEVICE_IDENTIFIER):
				{
					AmbientLightV2 al2;
					ambient_light_v2_create(&al2, _uid.c_str(), _tinkerMan->_ipcon);

					uint32_t light2;
					int result = ambient_light_v2_get_illuminance(&al2, &light2);

					ambient_light_v2_destroy(&al2);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(light2);
						double value = static_cast<double>(ivalue)/100.0 ;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(AMBIENT_LIGHT_V3_DEVICE_IDENTIFIER):
				{
					AmbientLightV3 al3;
					ambient_light_v3_create(&al3, _uid.c_str(), _tinkerMan->_ipcon);

					uint32_t light3;
					int result = ambient_light_v3_get_illuminance(&al3, &light3);

					ambient_light_v3_destroy(&al3);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(light3);
						double value = static_cast<double>(ivalue)/100.0;
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(MOISTURE_DEVICE_IDENTIFIER):
				{
					Moisture m;
					moisture_create(&m, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t moisture;
					int result = moisture_get_moisture_value(&m, &moisture);

					moisture_destroy(&m);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = static_cast<int>(moisture);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(PTC_DEVICE_IDENTIFIER):
				{
				    PTC ptc;
				    ptc_create(&ptc, _uid.c_str(), _tinkerMan->_ipcon);

				    bool ret_connected = false;
					ptc_is_sensor_connected(&ptc, &ret_connected);
					if(ret_connected)
					{
						int32_t temperature;
						int result = ptc_get_temperature(&ptc, &temperature);
						ptc_destroy(&ptc);

						if(result < 0)
						{
							failWithReadError(result);
							return false;
						}
						else
						{
							int ivalue = static_cast<int>(temperature);
							double value = static_cast<double>(ivalue)/100.0;
							addRawMeasurement(value);
						}
					}
					else
					{
						ptc_destroy(&ptc);
						_root->message("PTC Temperature Bricklet \'"+_uid+"\': no sensor connected.", true);
						addReadFailure();
						return false;
					}
					
					return true;
					break;
				}

				case(PTC_V2_DEVICE_IDENTIFIER):
				{
				    PTCV2 ptc2;
				    ptc_v2_create(&ptc2, _uid.c_str(), _tinkerMan->_ipcon);

				    bool ret_connected2 = false;
					ptc_v2_is_sensor_connected(&ptc2, &ret_connected2);
					if(ret_connected2)
					{
						int32_t temperature2;
						int result = ptc_v2_get_temperature(&ptc2, &temperature2);
						ptc_v2_destroy(&ptc2);

						if(result < 0)
						{
							failWithReadError(result);
							return false;
						}
						else
						{
							int ivalue = static_cast<int>(temperature2);
							double value = static_cast<double>(ivalue)/100.0;
							addRawMeasurement(value);
						}
					}
					else
					{
						ptc_v2_destroy(&ptc2);
						_root->message("PTC Temperature 2.0 Bricklet \'"+_uid+"\': no sensor connected.", true);
						return false;
					}
					
					return true;
					break;
				}

				case(INDUSTRIAL_DIGITAL_IN_4_DEVICE_IDENTIFIER):
				{
					IndustrialDigitalIn4 idi4;
				    industrial_digital_in_4_create(&idi4, _uid.c_str(), _tinkerMan->_ipcon);

					uint16_t value_mask;
					int result = industrial_digital_in_4_get_value(&idi4, &value_mask);
					industrial_digital_in_4_destroy(&idi4);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = ((value_mask & _bitMask) > 0);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(INDUSTRIAL_DIGITAL_IN_4_V2_DEVICE_IDENTIFIER):
				{
				    if(_channel < 4)
				    {
				    	IndustrialDigitalIn4V2 idi4;
				    	industrial_digital_in_4_v2_create(&idi4, _uid.c_str(), _tinkerMan->_ipcon);

						bool values[4];
						int result = industrial_digital_in_4_v2_get_value(&idi4, values);
						industrial_digital_in_4_v2_destroy(&idi4);

						if(result < 0)
						{
							failWithReadError(result);
							return false;
						}
						else
						{
							int ivalue = 0;
							if(values[_channel])
								ivalue = 1;

							double value = static_cast<double>(ivalue);
							addRawMeasurement(value);
						}
					}
					
					return true;
					break;
				}

				case(IO4_DEVICE_IDENTIFIER):
				{
					IO4 io;
				    io4_create(&io, _uid.c_str(), _tinkerMan->_ipcon);

					uint8_t value_mask;
					int result = io4_get_value(&io, &value_mask);
					io4_destroy(&io);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = ((value_mask & _bitMask) > 0);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(IO4_V2_DEVICE_IDENTIFIER):
				{
				    if(_channel < 4)
				    {
				    	IO4V2 io;
				    	io4_v2_create(&io, _uid.c_str(), _tinkerMan->_ipcon);

						bool values[4];
						int result = io4_v2_get_value(&io, values);
						io4_v2_destroy(&io);

						if(result < 0)
						{
							failWithReadError(result);
							return false;
						}
						else
						{
							int ivalue = 0;
							if(values[_channel])
								ivalue = 1;

							double value = static_cast<double>(ivalue);
							addRawMeasurement(value);
						}
					}
					
					return true;
					break;
				}

				case(IO16_DEVICE_IDENTIFIER):
				{
					IO16 io;
				    io16_create(&io, _uid.c_str(), _tinkerMan->_ipcon);

					uint8_t value_mask;
					int result = io16_get_port(&io, _ioPort, &value_mask);
					io16_destroy(&io);

					if(result < 0)
					{
						failWithReadError(result);
						return false;
					}
					else
					{
						int ivalue = ((value_mask & _bitMask) > 0);
						double value = static_cast<double>(ivalue);
						addRawMeasurement(value);
					}
					
					return true;
					break;
				}

				case(IO16_V2_DEVICE_IDENTIFIER):
				{
				    if(_channel < 16)
				    {
				    	IO16V2 io;
				    	io16_v2_create(&io, _uid.c_str(), _tinkerMan->_ipcon);

						bool values[16];
						int result = io16_v2_get_value(&io, values);
						io16_v2_destroy(&io);

						if(result < 0)
						{
							failWithReadError(result);
							return false;
						}
						else
						{
							int ivalue = 0;
							if(values[_channel])
								ivalue = 1;

							double value = static_cast<double>(ivalue);
							addRawMeasurement(value);
						}
					}
										
					return true;
					break;
				}
			}
		}
		else
		{
			std::stringstream ss;
			ss << "Tinkerforge sensor \'"<<_uid<<"\' is not known. The brick daemon did not assign any known device identifier to this bricklet. Assigned identifier is: "<<_deviceType<<".";
			_root->message(ss.str(), true);

			addReadFailure();
		}
	}

	return false;
}

bool sensorTinkerforge::measure(uint64_t currentTimestamp)
{
	if(getTriggerEvent() != periodic)  // callback-triggered sensor
	{
		_tinkerMan->reconnect();
		return true;
	}
	else  // periodic poll measurement
	{
		if(timeDiff(_timestamp_lastMeasurement, currentTimestamp) >= _minimumRestPeriod)
		{
			clean(currentTimestamp);
			if(poll(currentTimestamp))
			{
				return true;
			}

			// If polling failed, set last measurement time
			// to now to prevent excessive polling.
			_timestamp_lastMeasurement = currentTimestamp;
		}
	}

	return false;
}