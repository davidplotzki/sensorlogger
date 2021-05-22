#include "logbook.h"

#include "column.h"
#include "measurements.h" 
#include "counter.h"
#include "logger.h"
#include "mqttmanager.h"
#include "homematic.h"
#include "sensor.h"

logbook::logbook(logger* root, const std::string &filename, uint64_t cycleTime, unsigned maxEntries, const std::string &missingDataToken)
{
	_root = root;

	setFilename(filename);
	setCycleTime(cycleTime);
	setMaxEntries(maxEntries);
	setMissingDataToken(missingDataToken);

	setTimestampForNextLogEntry();
	_timestamp_last_logentry_written = _root->currentTimestamp();
}

logbook::~logbook()
{
	for(size_t i=0; i<_cols.size(); ++i)
		delete _cols.at(i);
}

std::string logbook::getFilename() const
{
	return _filename;
}

uint64_t logbook::getCycleTime() const
{
	return _cycleTime;
}

unsigned logbook::getMaxEntries() const
{
	return _maxEntries;
}

column* logbook::getCol(size_t pos)
{
	if(pos < _cols.size())
		return _cols.at(pos);
	else
		throw E_COLUMN_DOES_NOT_EXIST;
}

counter* logbook::getSharedCounterForSensor(sensor* s)
{
	for(size_t i=0; i<_cols.size(); ++i)
	{
		if(_cols.at(i)->getSensor() == s)
		{
			return _cols.at(i)->getCounter();
		}
	}

	return NULL;
}

void logbook::setFilename(const std::string &filename)
{
	_filename = filename;
}

void logbook::setCycleTime(uint64_t cycleTime)
{
	_cycleTime = cycleTime;

	// Update cycles to keep for each column:
	for(size_t i=0; i<_cols.size(); ++i)
	{
		_cols.at(i)->calculateColumnCycles();
	}
}

void logbook::setMaxEntries(unsigned maxEntries)
{
	_maxEntries = maxEntries;
}

void logbook::setMissingDataToken(const std::string &missingDataToken)
{
	_missingDataToken = missingDataToken;
}

void logbook::addColumn(column* col)
{
	_cols.push_back(col);
}

void logbook::setTimestampForNextLogEntry()
{
	uint64_t current  = _root->currentTimestamp();
	uint64_t lastLogentry = current - (current % _cycleTime);
	_timestamp_next_logentry = lastLogentry + _cycleTime;
}


void logbook::write(mqttManager* brokers, homematic* hm)
{
	uint64_t currentTimestamp = _root->currentTimestamp();

	if(currentTimestamp >= _timestamp_next_logentry)
	{
		// Round to full multiple of the logbook's cycle time:
		uint64_t currentTimeSlot = currentTimestamp - (currentTimestamp % _cycleTime);

		// Generate column values for this logbook entry,
		// and publish them to MQTT and HomeMatic if necessary.
		std::vector<std::string> columnValues;

		for(size_t i=0; i<_cols.size(); ++i)
		{
			std::string colValue = _missingDataToken;
			try	{
				uint64_t startTimestamp = currentTimeSlot - _cols.at(i)->getEvaluationPeriod();
				colValue = _cols.at(i)->getValue(startTimestamp, currentTimeSlot);
			} catch(int e) { }

			columnValues.push_back(colValue);

			if(colValue != _missingDataToken)
			{
				// Publish this result to MQTT Broker?
				if(_cols.at(i)->getMQTTPublishTopic().size() > 0)
				{
					try
					{
						brokers->publish(_cols.at(i)->getMQTTPublishTopic(), colValue);
					}
					catch(int e)
					{

					}
				}

				// Publish this result to Homematic?
				if(_cols.at(i)->getHomematicPublishISE().size() > 0)
				{
					try
					{
						hm->publish(_cols.at(i)->getHomematicPublishISE(), colValue);
					}
					catch(int e)
					{

					}
				}
			}
		}

		// If a logbook file is supposed to be written:
		if(_filename.size() > 0)
		{
			// Get current time:
			std::time_t now = std::chrono::system_clock::to_time_t(std::chrono::system_clock::now());
			struct tm *timeinfo;
			timeinfo = std::localtime(&now);
			
			int y	   = timeinfo->tm_year + 1900;
			int d 	   = timeinfo->tm_mday;
			int m	   = timeinfo->tm_mon + 1;
			int h	   = timeinfo->tm_hour;
			int minute = timeinfo->tm_min;
			int s      = timeinfo->tm_sec;

			// Read existing log values:
			std::ifstream existingLog;
			existingLog.open(_filename.c_str());

			std::vector<std::string> loglines;

			if(existingLog.is_open())
			{
				std::string line;

				while(!existingLog.eof())
				{
					getline(existingLog, line);
					if(line.length() > 0)
					{
						if(line[0] != '#')
						{
							loglines.push_back(line);
						}
					}
				}

				existingLog.close();
			}

			// Delete old lines:
			while(loglines.size() >= _maxEntries)
				loglines.erase(loglines.begin());

			// Create logbook header:
			std::stringstream headLine;
			headLine << "# Time             ";
			for(unsigned i=0; i<_cols.size(); ++i)
			{
				headLine << "\t";
				headLine << _cols.at(i)->getTitle();

				if(_cols.at(i)->getUnit() != "")
				{
					headLine << " [";
					headLine << _cols.at(i)->getUnit();
					headLine << "]";
				}
			}
			headLine << "\n";

			// Write header and existing lines:
			std::ofstream logwrite;
			logwrite.open(_filename.c_str());

			if(logwrite.is_open())
			{
				logwrite << headLine.str();

				for(unsigned i=0; i<loglines.size(); ++i)
				{
					logwrite << loglines.at(i) << std::endl;
				}


				// Create new line for current cycle:
				std::stringstream dateStream;
				dateStream << y;
				dateStream << "-" << std::setfill('0') << std::setw(2) << m;
				dateStream << "-" << std::setfill('0') << std::setw(2) << d;
				dateStream << " " << std::setfill('0') << std::setw(2) << h;
				dateStream << ":" << std::setfill('0') << std::setw(2) << minute;
				dateStream << ":" << std::setfill('0') << std::setw(2) << s;

				logwrite << dateStream.str();

				for(size_t i=0; i<columnValues.size(); ++i)
				{
					logwrite << "\t";			
					logwrite << columnValues.at(i);
				}

				logwrite << std::endl;
				logwrite.close();
			}
		}

		// Start new cycle for counters of all columns:
		for(size_t i=0; i<_cols.size(); ++i)
			_cols.at(i)->startNewCycle(currentTimeSlot);

		_timestamp_last_logentry_written = currentTimeSlot;
		setTimestampForNextLogEntry();
	}
}