#include "column.h"

#include "logbook.h"
#include "sensor.h"
#include "counter.h"
#include "measurements.h"

column::column(sensor* s, logbook* lb, const std::string &title, const std::string &unit, uint64_t evaluationPeriod, operation op, double confidenceAbsolute, double confidenceSigma, const std::string &mqttPublishTopic, const std::string &homematicPublishISE, double countFactor)
{
	_sensor = s;
	_rootLogbook = lb;

	/* We can share counters between all logbook columns that refer to
	   the same sensor for columns with the same cycle time. */
	_pulseCounter = _rootLogbook->getSharedCounterForSensor(_sensor);

	if(_pulseCounter == NULL)
		_pulseCounter = s->newCounter();

	setTitle(title);
	setUnit(unit);
	setOperation(op);
	setEvaluationPeriod(evaluationPeriod);
	setConfidenceAbsolute(confidenceAbsolute);
	setConfidenceSigma(confidenceSigma);
	setMQTTPublishTopic(mqttPublishTopic);
	setHomematicPublishISE(homematicPublishISE);
	setCountFactor(countFactor);
}

std::string column::getTitle() const
{
	return _title;
}

std::string column::getUnit() const
{
	return _unit;
}

operation column::getOperation() const
{
	return _op;
}

uint64_t column::getEvaluationPeriod() const
{
	return _evaluationPeriod;
}

double column::getConfidenceAbsolute() const
{
	return _confidenceAbsolute;
}

double column::getConfidenceSigma() const
{
	return _confidenceSigma;
}

std::string column::getMQTTPublishTopic() const
{
	return _mqttPublishTopic;
}

std::string column::getHomematicPublishISE() const
{
	return _homematicPublishISE;
}

double column::getCountFactor() const
{
	return _countFactor;
}

counter* column::getCounter()
{
	return _pulseCounter;
}

sensor* column::getSensor()
{
	return _sensor;
}

void column::setTitle(const std::string &title)
{
	_title = title;
}

void column::setUnit(const std::string &unit)
{
	_unit = unit;
}

void column::setOperation(operation op)
{
	_op = op;
}

void column::setEvaluationPeriod(uint64_t evaluationPeriod)
{
	_evaluationPeriod = evaluationPeriod;
	if(_evaluationPeriod == 0)
	{
		_evaluationPeriod = _rootLogbook->getCycleTime();
	}
	_sensor->accumulateMaxTimeToKeep(_evaluationPeriod);
	calculateColumnCycles();
}

void column::calculateColumnCycles()
{
	uint64_t nCycles = _evaluationPeriod / _rootLogbook->getCycleTime();

	_nCycles = static_cast<size_t>(nCycles);
	_pulseCounter->accumulateCyclesToStore(_nCycles);
}

void column::setConfidenceAbsolute(double confidenceAbsolute)
{
	_confidenceAbsolute = confidenceAbsolute;
}

void column::setConfidenceSigma(double confidenceSigma)
{
	_confidenceSigma = confidenceSigma;
}

void column::setMQTTPublishTopic(const std::string &mqttPublishTopic)
{
	_mqttPublishTopic = mqttPublishTopic;
}

void column::setHomematicPublishISE(const std::string &homematicPublishISE)
{
	_homematicPublishISE = homematicPublishISE;
}

void column::setCountFactor(double countFactor)
{
	_countFactor = countFactor;
}

std::string column::getValue(uint64_t startTimestamp, uint64_t currentTimestamp) const
{
	if(_sensor != NULL)
	{
		double value = 0;
		std::vector<double>* values = NULL;
		if(!(_op == freq || _op==freq_min || _op==freq_max || _op==count))
		{
			values = _sensor->valuesInConfidence(startTimestamp, _confidenceAbsolute, _confidenceSigma);
		}

		switch(_op)
		{
			case(mean):         value = measurementFunctions::mean(values); break;
			case(median):       value = measurementFunctions::median(values); break;
			case(max):          value = measurementFunctions::maximum(values); break;
			case(min):          value = measurementFunctions::minimum(values); break;
			case(sum):          value = measurementFunctions::sum(values); break;
			case(stdDevMean):   value = measurementFunctions::stdDevMean(values); break;
			case(stdDevMedian): value = measurementFunctions::stdDevMedian(values); break;
			case(count):
				value = _countFactor * static_cast<double>(_pulseCounter->counts(_nCycles));
				break;
			case(freq):
				value = _countFactor * _pulseCounter->frequency(_nCycles, currentTimestamp);
				break;
			case(freq_min):
				value = _countFactor * _pulseCounter->frequency_min(_nCycles);
				break;
			case(freq_max):
				value = _countFactor * _pulseCounter->frequency_max(_nCycles);
				break;
		}

		if(!(_op == freq || _op==freq_min || _op==freq_max || _op==count))
		{
			if(values != NULL)
				delete values;
		}

		std::stringstream ss;
		ss << value;
		return ss.str();
	}

	throw E_NO_VALUES_FOR_COLUMN;
}

void column::startNewCycle(const uint64_t currentTimestamp)
{
	_pulseCounter->startNewCycle(currentTimestamp);
}
