#include "sensor_homematic.h"

#include "logger.h"
#include "homematic.h"
#include "measurements.h"

sensorHomematic::sensorHomematic(logger* root, homematic* hmPtr, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, const std::string &homematicSubscribeISE, bool isCounter, double factor, double offset, uint64_t minimumRestPeriod)
{
	setPointerToLogger(root);
	_homematic = hmPtr;
	setSensorID(sensorID);
	setTriggerEvent(periodic);
	setFactor(factor);
	setOffset(offset);
	setCounter(isCounter);
	setMinimumRestPeriod(minimumRestPeriod);
	setMQTTPublishTopic(mqttPublishTopic);
	setHomematicPublishISE(homematicPublishISE);
	setISE(homematicSubscribeISE);
}

sensor_type sensorHomematic::type() const
{
	return sensor_homematic;
}

std::string sensorHomematic::getISE()
{
	return _iseID;
}

void sensorHomematic::setISE(const std::string &ise)
{
	_iseID = ise;
}

bool sensorHomematic::measure(uint64_t currentTimestamp)
{
	if(timeDiff(_timestamp_lastMeasurement, currentTimestamp) >= _minimumRestPeriod)
	{
		clean(currentTimestamp);

		if(_homematic != NULL)
		{
			try
			{
				std::string valueString = _homematic->getValue(_iseID);

				double value = 0;

				if(valueString.size() > 0)
					value = atof(valueString.c_str());

				return addRawMeasurement(value);
			}
			catch(int e)
			{
				addReadFailure();
				_timestamp_lastMeasurement = currentTimestamp;

				// homematic::getValue already reports errors.
				// Only report other errors here...
				if(e != E_CANNOT_READ_HM_SYSVAR)
					_root->message("Error reading sensor: " + _sensorID, true);
			}
		}
	}

	return false;
}