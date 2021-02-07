#ifndef _JSON_H
#define _JSON_H

#include <string>
#include <cstring>
#include <sstream>
#include <fstream>
#include <vector>
#include <iostream>
#include <cmath>

#define E_VALUE_IS_NOT_BOOL               2001
#define E_VALUE_IS_NOT_INT                2002
#define E_VALUE_IS_NOT_DOUBLE             2003
#define E_VALUE_IS_NOT_STRING             2004
#define E_CANNOT_READ_JSON                2005
#define E_NOT_A_BRACKET                   2006
#define E_NO_CORRESPONDING_BRACKET        2007
#define E_NAME_NOT_FOUND                  2008
#define E_POS_OUT_OF_BOUNDS               2009
#define E_NAME_EXPECTED                   2010
#define E_STOP_POSITION_OUT_OF_BOUNDS     2011
#define E_COLON_EXPECTED                  2012
#define E_VALUE_EXPECTED                  2013
#define E_BRACKET_EXPECTED                2014
#define E_COMMA_OR_END_OF_OBJECT_EXPECTED 2015
#define E_CURLY_AT_JSONSTART_EXPECTED     2016
#define E_UNKNOWN_UNIT_OF_TIME            2017
#define E_WRONG_TIMEVALUE_TYPE            2018

enum jsonType {jsonNull, jsonBool, jsonInt, jsonDouble, jsonString};

bool isIn(const char& c, const char* chain);
bool isIntInString(const std::string &s);
bool isFloatInString(const std::string &s);

class jsonValue
{
private:
	jsonType    _type;
	bool        _vBool;
	long        _vInt;
	double      _vDouble;
	std::string _vString;

public:
	jsonValue();
	jsonType type();

	void setNull();
	void setBool(bool v);
	void setInt(long v);
	void setDouble(double v);
	void setString(const std::string &v);

	bool        isNull() const;
	bool        getBool() const;
	long        getInt() const;
	double      getDouble() const;
	std::string getString() const;

	std::string print() const;
};

class jsonNode
{
private:
	std::string            _name;
	jsonValue              _value;
	std::vector<jsonNode*> _subnodes;
	bool                   _isArray;

public:
	jsonNode();
	~jsonNode();
	jsonNode* element(const std::string &name);
	jsonNode* element(size_t pos);

	bool isNull() const;
	bool exist(const std::string &name) const;
	bool exist(size_t pos) const;
	bool existAndNotNull(const std::string &name);
	bool existAndNotNull(size_t pos);

	size_t nElements() const;
	jsonValue* value();
	std::string print(unsigned level) const;

	void setName(const std::string &name);
	std::string getName() const;
	void parseObject(const std::string &content, size_t start, size_t stop);
	void parseArray(const std::string &content, size_t start, size_t stop);

	// Return duration in milliseconds of value-unit pair:
	uint64_t durationInMS();
};

// Simple representation of a JSON file
class json
{
private:
	jsonNode    _root;
	std::string _filename;
	std::string _content;

public:
	json();
	json(const std::string &filename);

	void setFilename(const std::string &filename);
	void readFile();
	void parseFile(const std::string &filename);
	void setContent(const std::string &content);

	void parse();

	jsonNode* root();
	jsonNode* element(const std::string &name);
	jsonNode* element(size_t pos);
	bool exist(const std::string &name) const;
	bool exist(size_t pos) const;
	bool existAndNotNull(const std::string &name);
	bool existAndNotNull(size_t pos);

	std::string print() const;
};

// returns position of corresponding bracket
size_t findCorrespondingBracket(size_t pos, const std::string &pstring);

#endif