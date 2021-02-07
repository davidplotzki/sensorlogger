#include "sensor.h"

#include "counter.h"
#include "logger.h"
#include "measurements.h"
#include "json.h"

sensor::sensor()
{
	resetReadFailures();
	_keepValuesFor_ms = 0;
	setCounter(false);
	setTriggerEvent(periodic);

	_m = new measurements();

	_lastValuePublished = false;
}

sensor::~sensor()
{
	delete _m;
	for(size_t i=0; i<_counters.size(); ++i)
	{
		delete _counters.at(i);
	}
}

void sensor::clean(uint64_t currentTimestamp)
{
	_m->clean(currentTimestamp - _keepValuesFor_ms);
}

void sensor::accumulateMaxTimeToKeep(uint64_t timeToKeep)
{
	if(_keepValuesFor_ms < timeToKeep)
		_keepValuesFor_ms = timeToKeep;
}

counter* sensor::newCounter()
{
	counter* c = new counter(_root->currentTimestamp());
	_counters.push_back(c);
	return c;
}

void sensor::setPointerToLogger(logger* root)
{
	_root = root;
}

void sensor::setSensorID(const std::string &sensorID)
{
	_sensorID = sensorID;
}

void sensor::setCounter(bool c)
{
	_isCounter = c;
}

void sensor::setTriggerEvent(trigger_event e)
{
	_triggerEvent = e;
}

void sensor::setFactor(double factor)
{
	_factor = factor;
}

void sensor::setOffset(double offset)
{
	_offset = offset;
}

void sensor::setMinimumRestPeriod(uint64_t minRestPeriod)
{
	_minimumRestPeriod = minRestPeriod;
	_m->setMinimumRestPeriod(_minimumRestPeriod);
}

void sensor::setMQTTPublishTopic(const std::string &mqttPublishTopic)
{
	_mqttPublishTopic = mqttPublishTopic;
}

void sensor::setHomematicPublishISE(const std::string &homematicPublishISE)
{
	_homematicPublishISE = homematicPublishISE;
}

void sensor::addJSONkey(const std::string &key)
{
	if(key.size() > 0)
		_jsonKey.push_back(key);
}

void sensor::setJSONkeys(const std::vector<std::string>* keys)
{
	for(size_t i=0; i<keys->size(); ++i)
		_jsonKey.push_back(keys->at(i));
}

size_t sensor::nJSONkeys() const
{
	return _jsonKey.size();
}

const std::string& sensor::jsonKey(size_t i) const
{
	if(i < _jsonKey.size())
	{
		return _jsonKey.at(i);
	}

	throw E_NAME_NOT_FOUND;
}

logger* sensor::ptLogger()
{
	return _root;
}

std::string sensor::getSensorID() const
{
	return _sensorID;
}

bool sensor::isCounter() const
{
	return _isCounter;
}

trigger_event sensor::getTriggerEvent() const
{
	return _triggerEvent;
}

double sensor::getFactor() const
{
	return _factor;
}

double sensor::getOffset() const
{
	return _offset;
}

uint64_t sensor::getMinimumRestPeriod() const
{
	return _minimumRestPeriod;
}

std::string sensor::getMQTTPublishTopic() const
{
	return _mqttPublishTopic;
}

uint64_t sensor::getReadFailures() const
{
	return _nReadFailures;
}

void sensor::addReadFailure()
{
	uint64_t currentTimestamp = _root->currentTimestamp();

	if(timeDiff(_timestamp_lastMeasurement, currentTimestamp) >= _minimumRestPeriod)
	{
		++_nReadFailures;
		
		_timestamp_lastMeasurement = currentTimestamp;
		clean(currentTimestamp);
	}
}

void sensor::resetReadFailures()
{
	_nReadFailures = 0;
}

void sensor::resetCounters()
{
	uint64_t currentTimestamp = _root->currentTimestamp();

	for(size_t i=0; i<_counters.size(); ++i)
		_counters.at(i)->reset(currentTimestamp);
}

/*
void sensor::count()
{
	uint64_t currentTimestamp = _root->currentTimestamp();
	count(currentTimestamp);
}
*/

void sensor::count(uint64_t currentTimestamp)
{
	// Increase count for all counters:
	for(size_t i=0; i<_counters.size(); ++i)
		_counters.at(i)->count(currentTimestamp);
}

size_t sensor::nMeasurements() const
{
	return _m->nMeasurements();
}

bool sensor::addRawMeasurement(double value)
{
	uint64_t currentTimestamp = _root->currentTimestamp();
	resetReadFailures();

	if(timeDiff(_timestamp_lastMeasurement, currentTimestamp) >= _minimumRestPeriod)
	{
		// Round to full multiple of the sensor's rest period:
		uint64_t currentTimeslot = currentTimestamp - (currentTimestamp % _minimumRestPeriod);

		_timestamp_lastMeasurement = currentTimeslot;
		clean(currentTimeslot);

		count(currentTimeslot);

		double convertedValue = _factor * (value + _offset);

		if(isCounter())
		{
			_m->setOnlyValue(convertedValue, currentTimeslot);
		}
		else
		{
			_m->addValue(convertedValue, currentTimeslot);
		}
		
		_lastValuePublished = false;

		return true;
	}

	return false;
}

std::vector<double>* sensor::valuesInConfidence(uint64_t startTimestamp, double absolute, double nSigma) const
{
	return _m->valuesInConfidence(startTimestamp, absolute, nSigma);
}

void sensor::reset()
{
	_m->clear();
	_nReadFailures = 0;
}

double sensor::getLastValue() const
{
	return _m->getLastValue();
}

void sensor::publishLastEvent()
{
	if(!_lastValuePublished)
	{
		std::stringstream ssPayload;
		ssPayload << getLastValue();
		std::string payload = ssPayload.str();

		if(_mqttPublishTopic.size() > 0)
		{
			try
			{
				_root->mqttPublish(_mqttPublishTopic, payload);
			}
			catch(int e)
			{

			}
		}

		if(_homematicPublishISE.size() > 0)
		{
			try
			{
				_root->homematicPublish(_homematicPublishISE, payload);
			}
			catch(int e)
			{

			}
		}

		_lastValuePublished = true;
	}
}