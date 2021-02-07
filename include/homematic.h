#ifndef _HOMEMATIC_H
#define _HOMEMATIC_H

#define E_CANNOT_READ_HM_SYSVAR 8001

#include <string>

class logger;

class homematic
{
private:
	std::string _xmlAPI_URL;  // Full Homematic XML API URL, including http(s)

	logger*     _root;

public:
	homematic(logger* root);
	~homematic();

	void setXMLAPI_URL(const std::string &xml_api_url);
	std::string getXMLAPI_URL() const;

	void publish(const std::string &iseID, const std::string &payload) const;
	std::string getValue(const std::string &iseID) const;
};

#endif