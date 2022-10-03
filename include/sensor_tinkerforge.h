#ifndef _SENSOR_TINKERFORGE_H
#define _SENSOR_TINKERFORGE_H

#ifdef OPTION_TINKERFORGE

#include "sensor.h"
#include "tinkerforge.h"

class sensorTinkerforge : public sensor
{
private:
	void failWithReadError(int tf_error_code);

	std::string _uid;
	unsigned    _deviceType;
	uint8_t     _channel;  // To select input channel
	uint16_t    _bitMask;  // Input channel mask
	char        _ioPort;   // Port 'a' or 'b' of the IO16 bricklet
	unsigned    _debounceTime;

	bool        _isInitialized;

	tinkerforge*          _tinkerMan;
	tinkerforge_callback* _tinkerforgeCallback;

public:
	sensorTinkerforge(logger* root, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, tinkerforge* tinkerManager, const std::string uid, trigger_event triggerEvent, bool isCounter, uint8_t channel, char ioPort, uint32_t debounceTime, double factor, double offset, uint64_t minimumRestPeriod, uint64_t retryTime);

	~sensorTinkerforge();
	
	sensor_type type() const;

	std::string getUID() const;
	std::string getMasterBrickUID() const;
	unsigned    getDeviceType() const;
	uint16_t    getBitMask() const;
	uint8_t     getChannel() const;
	uint32_t    getDebounceTime() const;
	char        getIOPort() const;

	void setUID(const std::string &uid);
	void setMasterBrickUID(const std::string &masterBrickUID);
	void setDeviceType(unsigned deviceType);
	void setChannel(uint8_t channel);
	void setIOPort(char ioPort);
	void setDebounceTime(uint32_t debounceTime);

	void registerCallback();
	bool poll(uint64_t currentTimestamp);
	bool measure(uint64_t currentTimestamp);
};


#endif
#endif