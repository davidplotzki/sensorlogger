#ifndef _TINKERFORGE_H
#define _TINKERFORGE_H

#include <cstdint>
#include <string>
#include <sstream>

#include "ip_connection.h"
#include "brick_master.h"
#include "bricklet_air_quality.h"
#include "bricklet_ambient_light.h"
#include "bricklet_ambient_light_v2.h"
#include "bricklet_ambient_light_v3.h"
#include "bricklet_analog_in.h"
#include "bricklet_analog_in_v2.h"
#include "bricklet_analog_in_v3.h"
#include "bricklet_temperature.h"
#include "bricklet_temperature_v2.h"
#include "bricklet_humidity.h"
#include "bricklet_humidity_v2.h"
#include "bricklet_barometer.h"
#include "bricklet_barometer_v2.h"
#include "bricklet_co2.h"
#include "bricklet_co2_v2.h"
#include "bricklet_current12.h"
#include "bricklet_current25.h"
#include "bricklet_distance_ir.h"
#include "bricklet_distance_ir_v2.h"
#include "bricklet_distance_us.h"
#include "bricklet_distance_us_v2.h"
#include "bricklet_dust_detector.h"
#include "bricklet_energy_monitor.h"
#include "bricklet_industrial_digital_in_4.h"
#include "bricklet_industrial_digital_in_4_v2.h"
#include "bricklet_industrial_dual_0_20ma.h"
#include "bricklet_industrial_dual_0_20ma_v2.h"
#include "bricklet_industrial_dual_analog_in.h"
#include "bricklet_industrial_dual_analog_in_v2.h"
#include "bricklet_io4.h"
#include "bricklet_io4_v2.h"
#include "bricklet_io16.h"
#include "bricklet_io16_v2.h"
#include "bricklet_laser_range_finder.h"
#include "bricklet_laser_range_finder_v2.h"
#include "bricklet_line.h"
#include "bricklet_load_cell.h"
#include "bricklet_load_cell_v2.h"
#include "bricklet_moisture.h"
#include "bricklet_particulate_matter.h"
#include "bricklet_sound_intensity.h"
#include "bricklet_sound_pressure_level.h"
#include "bricklet_ptc.h"
#include "bricklet_ptc_v2.h"
#include "bricklet_temperature_ir.h"
#include "bricklet_temperature_ir_v2.h"
#include "bricklet_uv_light.h"
#include "bricklet_uv_light_v2.h"
#include "bricklet_voltage.h"
#include "bricklet_voltage_current.h"
#include "bricklet_voltage_current_v2.h"


class logger;
class sensorTinkerforge;

std::string getTFConnectionErrorText(int e);
std::string getDeviceType_name(uint16_t device_identifier);
std::string getDeviceType_nice(uint16_t device_identifier);
void enumerateTFSensors(const char *uid, const char *connected_uid, char position, uint8_t hardware_version[3], uint8_t firmware_version[3], uint16_t device_identifier, uint8_t enumeration_type, logger* _root);


class tinkerforge
{
private:
	std::string _host;
	unsigned short _port;
	uint32_t _timeout;

	logger* _root;

public:
	IPConnection *_ipcon;

	tinkerforge(logger* root);
	tinkerforge(logger* root, const std::string &host, unsigned short port);
	~tinkerforge();

	void setHost(const std::string &host);
	void setPort(unsigned short port);

	std::string getHost() const;
	unsigned short getPort() const;

	bool disconnect();
	bool disconnect_and_prepare();
	bool reconnect();
};


/* A callback class to store all callback properties for a sensorTinkerforge object.
   Which callbacks will be registered (and how) will be figured out later by the
   tinkerforge TinkerMan, because Sensors with the same UID but different thresholds/periods
   might be defined. */

class tinkerforge_callback
{
protected:
	sensorTinkerforge* _s;
	tinkerforge*       _tinkerMan;

	bool               _value_has_to_change;
	double             _min;
	double             _max;

public:
	tinkerforge_callback();
	virtual ~tinkerforge_callback();

	bool         valueHasToChange() const;
	double       min() const;
	double       max() const;

	void setValueHasToChange(bool value_has_to_change);
	void setMin(double min);
	void setMax(double max);

	virtual void reset() = 0;
	virtual void registerCallback() = 0;
};


class tinkerforge_callback_io4 : public tinkerforge_callback
{
private:
	IO4 *_io4;

public:
	tinkerforge_callback_io4(sensorTinkerforge* s, tinkerforge* tinkerMan);
	~tinkerforge_callback_io4();

	void reset();
	void registerCallback();
};


class tinkerforge_callback_io4_v2 : public tinkerforge_callback
{
private:
	IO4V2 *_io4_v2;

public:
	tinkerforge_callback_io4_v2(sensorTinkerforge* s, tinkerforge* tinkerMan);
	~tinkerforge_callback_io4_v2();

	void reset();
	void registerCallback();
};


class tinkerforge_callback_io16 : public tinkerforge_callback
{
private:
	IO16 *_io16;

public:
	tinkerforge_callback_io16(sensorTinkerforge* s, tinkerforge* tinkerMan);
	~tinkerforge_callback_io16();

	void reset();
	void registerCallback();
};

class tinkerforge_callback_io16_v2 : public tinkerforge_callback
{
private:
	IO16V2 *_io16_v2;

public:
	tinkerforge_callback_io16_v2(sensorTinkerforge* s, tinkerforge* tinkerMan);
	~tinkerforge_callback_io16_v2();

	void reset();
	void registerCallback();
};


// Callback functions
void callback_io4(uint8_t interrupt_mask, uint8_t value_mask, sensorTinkerforge* cb);  // IO-4
void callback_io16(char port, uint8_t interrupt_mask, uint8_t value_mask, sensorTinkerforge* s); // IO-16
void callback_io_v2(uint8_t channel, bool changed, bool value, sensorTinkerforge* s); // IO-4/16 2.0


#endif