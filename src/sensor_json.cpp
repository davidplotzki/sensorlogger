#include "sensor_json.h"

#include "json.h"
#include "readoutbuffer.h"
#include "logger.h"
#include "measurements.h"

sensorJSON::sensorJSON(logger* root, const std::string &sensorID, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, const std::string &jsonFile, const std::vector<std::string>* jsonKeys, bool isCounter, double factor, double offset, uint64_t minimumRestPeriod, readoutBuffer* buffer)
{
	setPointerToLogger(root);
	setSensorID(sensorID);
	setJSONfilename(jsonFile);
	setJSONkeys(jsonKeys);
	setFactor(factor);
	setOffset(offset);
	setCounter(isCounter);
	setMinimumRestPeriod(minimumRestPeriod);
	setMQTTPublishTopic(mqttPublishTopic);
	setHomematicPublishISE(homematicPublishISE);

	_rBuffer = buffer;
}

sensor_type sensorJSON::type() const
{
	return sensor_json;
}

void sensorJSON::setJSONfilename(const std::string &jsonFilename)
{
	_jsonFilename = jsonFilename;
}

bool sensorJSON::measure(uint64_t currentTimestamp)
{
	if(timeDiff(_timestamp_lastMeasurement, currentTimestamp) >= _minimumRestPeriod)
	{
		clean(currentTimestamp);

		try
		{
			if(_jsonKey.size() > 0)
			{
				json j;
				j.setContent(_rBuffer->getFileContents(_jsonFilename, currentTimestamp));
				j.parse();

				jsonNode* node = j.root();
				for(size_t i=0; i<_jsonKey.size(); ++i)
				{
					node = node->element(_jsonKey.at(i));
				}

				double value = node->value()->getDouble();
				return addRawMeasurement(value);
			}
		}
		catch(int e)
		{
			_timestamp_lastMeasurement = currentTimestamp;
			addReadFailure();
			_root->message("Error reading sensor: " + _sensorID, true);
		}
	}

	return false;
}