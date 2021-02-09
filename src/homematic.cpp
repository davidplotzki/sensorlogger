#include "homematic.h"

#include "sensorlogger.h"
#include "logger.h"

homematic::homematic(logger*root)
{
	_root = root;
}

homematic::~homematic()
{

}

void homematic::setXMLAPI_URL(const std::string &xml_api_url)
{
	_xmlAPI_URL = xml_api_url;
}

std::string homematic::getXMLAPI_URL() const
{
	return _xmlAPI_URL;
}

void homematic::publish(const std::string &iseID, const std::string &payload) const
{
	if(_xmlAPI_URL.size() > 0)
	{
		std::string publishURL = _xmlAPI_URL + "/statechange.cgi?ise_id=" + iseID + "&new_value=" + payload;
		
		try
		{
			_root->httpRequest(publishURL);
		}
		catch(int e)
		{
			_root->message("Cannot publish to Homematic ISE " + iseID + ".", true);
		}
	}
}

std::string homematic::getValue(const std::string &iseID) const
{
	if(_xmlAPI_URL.size() > 0)
	{
		std::string getterURL = _xmlAPI_URL + "/state.cgi?datapoint_id=" + iseID;

		try
		{
			std::string sysvarText = _root->httpRequest(getterURL);
			std::string valueKey = "value=";

			size_t valuePos = sysvarText.find(valueKey, 0);

			if(valuePos != std::string::npos)
			{
				size_t stopPos = 0;

				valuePos += valueKey.size();
				if(sysvarText.size() > valuePos)
				{
					char quotationMark = sysvarText.at(valuePos);
					valuePos++;

					if(sysvarText.size() > valuePos)
					{
						// Find next true quotation mark:
						stopPos = valuePos;
						bool skippedEscaped = false;
						for(size_t i=valuePos; i<sysvarText.size(); ++i)
						{
							if((sysvarText.at(i) == '\\') && !skippedEscaped)
							{
								skippedEscaped = true;
								i+=1;
								continue;
							}

							skippedEscaped = false;

							if(sysvarText.at(i) == quotationMark)
							{
								stopPos = i;
								break;
							}
						}
					}
				}

				if(stopPos > valuePos)
				{
					return sysvarText.substr(valuePos, stopPos-valuePos);
				}
				else if(stopPos == valuePos)
				{
					// Empty result will be converted to 0 by the sensor.
					return "";
				}
			}
			else
			{
				_root->message("Cannot measure HomeMatic system variable with ISE ID " + iseID + ". Reason: no value=\'\' key in the string returned from the HomeMatic XML API. Requested URL: " + getterURL, true);
				throw E_CANNOT_READ_HM_SYSVAR;
			}
		}
		catch(int e)
		{
			if(e != E_CANNOT_READ_HM_SYSVAR)
				_root->message("Cannot measure HomeMatic system variable with ISE ID " + iseID + ". Requested URL: " + getterURL, true);

			throw E_CANNOT_READ_HM_SYSVAR;
		}
	}

	_root->message("Cannot measure HomeMatic system variable with ISE ID " + iseID + ". XML API URL: " + _xmlAPI_URL, true);

	throw E_CANNOT_READ_HM_SYSVAR;
}