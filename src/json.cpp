#include "json.h"

const char* intChain = "+-0123456789";
const char* floatChain = "+-eE.0123456789";

bool isIn(const char&c, const char* chain)
{
	for(unsigned i=0; i<strlen(chain); ++i)
	{
		if(c == chain[i])
			return true;
	}

	return false;
}

bool isIntInString(const std::string &s)
{
	for(size_t i=0; i<s.size(); ++i)
	{
		if(isIn(s.at(i), intChain))
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	return true;
}

bool isFloatInString(const std::string &s)
{
	for(size_t i=0; i<s.size(); ++i)
	{
		if(isIn(s.at(i), floatChain))
		{
			continue;
		}
		else
		{
			return false;
		}
	}

	return true;
}


jsonValue::jsonValue()
{
	setBool(false);
	setInt(0);
	setDouble(0.0);
	setNull();
}

jsonType jsonValue::type()
{
	return _type;
}

void jsonValue::setNull()
{
	_type = jsonNull;
}

void jsonValue::setBool(bool v)
{
	_type = jsonBool;
	_vBool = v;
}

void jsonValue::setInt(long v)
{
	_type = jsonInt;
	_vInt = v;
}

void jsonValue::setDouble(double v)
{
	_type = jsonDouble;
	_vDouble = v;
}

void jsonValue::setString(const std::string &v)
{
	_type = jsonString;
	_vString = v;
}

bool jsonValue::isNull() const
{
	return (_type == jsonNull);
}

bool jsonValue::getBool() const
{
	if(_type == jsonBool)
		return _vBool;

	throw E_VALUE_IS_NOT_BOOL;
}

long jsonValue::getInt() const
{
	if(_type == jsonBool)
	{
		if(_vBool)
			return 1;
		else
			return 0;
	}

	if(_type == jsonInt)
		return _vInt;

	if(_type == jsonDouble)
		return static_cast<long>(round(_vDouble));

	if(_type == jsonString)
	{
		if(isIntInString(_vString))
			return atoi(_vString.c_str());
	}

	throw E_VALUE_IS_NOT_INT;
}

double jsonValue::getDouble() const
{
	if(_type == jsonBool)
	{
		if(_vBool)
			return 1.0;
		else
			return 0.0;
	}

	if(_type == jsonInt)
		return static_cast<double>(_vInt);

	if(_type == jsonDouble)
		return _vDouble;

	if(_type == jsonString)
	{
		if(isFloatInString(_vString))
			return atof(_vString.c_str());
	}

	throw E_VALUE_IS_NOT_DOUBLE;
}

std::string jsonValue::getString() const
{
	if(_type == jsonBool)
	{
		if(_vBool)
			return "true";
		else
			return "false";
	}

	if(_type == jsonInt)
	{
		std::stringstream ss;
		ss << _vInt;
		return ss.str();
	}

	if(_type == jsonDouble)
	{
		std::stringstream ss;
		ss << _vDouble;
		return ss.str();
	}

	if(_type == jsonString)
		return _vString;

	throw E_VALUE_IS_NOT_STRING;
}

std::string jsonValue::print() const
{
	std::stringstream ss;

	switch(_type)
	{
		case(jsonNull): ss << "null"; break;
		case(jsonBool):
			if(_vBool) ss << "true";
			else ss << "false";
			break;
		case(jsonInt): ss << _vInt; break;
		case(jsonDouble): ss << _vDouble; break;
		case(jsonString): ss << "\"" << _vString << "\""; break;
	}

	return ss.str();
}

jsonNode::jsonNode()
{
	_isArray = false;
}

jsonNode::~jsonNode()
{
	for(size_t i=0; i<_subnodes.size(); ++i)
		delete _subnodes.at(i);
}

jsonNode* jsonNode::element(const std::string &name)
{
	for(size_t i=0; i<_subnodes.size(); ++i)
	{
		if(_subnodes.at(i)->getName() == name)
			return _subnodes.at(i);
	}

	// Nothing found by that name. Check is string contains
	// an integer, which could mean an array index.
	if(isIntInString(name))
	{
		int pos = atoi(name.c_str());
		if(pos >= 0)
		{
			size_t pos_t = static_cast<size_t>(pos);
			return element(pos_t);
		}
	}

	throw E_NAME_NOT_FOUND;
}

jsonNode* jsonNode::element(size_t pos)
{
	if(pos < _subnodes.size())
		return _subnodes.at(pos);

	throw E_POS_OUT_OF_BOUNDS;
}

bool jsonNode::isNull() const
{
	if(_subnodes.size() == 0)
	{
		return _value.isNull();
	}

	return false;
}

bool jsonNode::exist(const std::string &name) const
{
	for(size_t i=0; i<_subnodes.size(); ++i)
	{
		if(_subnodes.at(i)->getName() == name)
			return true;
	}

	return false;
}

bool jsonNode::exist(size_t pos) const
{
	if(pos < _subnodes.size())
		return true;

	return false;
}

bool jsonNode::existAndNotNull(const std::string &name)
{
	if(exist(name))
		return !element(name)->isNull();

	return false;
}

bool jsonNode::existAndNotNull(size_t pos)
{
	if(exist(pos))
		return !element(pos)->isNull();
	
	return false;
}

size_t jsonNode::nElements() const
{
	return _subnodes.size();
}

jsonValue* jsonNode::value()
{
	return &_value;
}

std::string jsonNode::print(unsigned level) const
{
	std::stringstream ss;

	for(unsigned t=0; t<level; ++t)
		ss << "\t";

	if(_name.size() > 0)
	{
		ss << "\"" << _name << "\": ";
	}

	if(_subnodes.size() > 0)
	{
		if(_isArray)
			ss << "[" << std::endl;
		else
			ss << "{" << std::endl;

		for(size_t i=0; i<_subnodes.size(); ++i)
		{
			if(i > 0)
				ss << "," << std::endl;

			ss << _subnodes.at(i)->print(level+1);
		}

		ss << std::endl;

		for(unsigned t=0; t<level; ++t)
			ss << "\t";

		if(_isArray)
			ss << "]";
		else
			ss << "}";
	}
	else
	{
		ss << _value.print();
	}		

	return ss.str();
}

void jsonNode::setName(const std::string &name)
{
	_name = name;
}

std::string jsonNode::getName() const
{
	return _name;
}

uint64_t jsonNode::durationInMS()
{
	std::string unit = element("unit")->value()->getString();
	if(element("value")->value()->type() == jsonInt)
	{
		long signedVal = element("value")->value()->getInt();
		if(signedVal < 0)
			signedVal = -signedVal;

		uint64_t value = static_cast<uint64_t>(signedVal);

		if(unit == "ms")
			return value;
		else if(unit == "s")
			return 1000*value;
		else if(unit == "min")
			return 60000*value;
		else if(unit == "h")
			return 3600000*value;
		else if(unit == "d")
			return 86400000*value;
		else
			throw E_UNKNOWN_UNIT_OF_TIME;
	}
	else if(element("value")->value()->type() == jsonDouble)
	{
		double signedVal = element("value")->value()->getDouble();
		if(signedVal < 0)
			signedVal = -signedVal;

		double value;
		if(unit == "ms")
			value = signedVal;
		else if(unit == "s")
			value = signedVal * 1000.0;
		else if(unit == "min")
			value = signedVal * 60000.0;
		else if(unit == "h")
			value = signedVal * 3600000.0;
		else if(unit == "d")
			value = signedVal * 86400000.0;
		else
			throw E_UNKNOWN_UNIT_OF_TIME;

		return static_cast<uint64_t>(value);
	}

	throw E_WRONG_TIMEVALUE_TYPE;
}

void jsonNode::parseObject(const std::string &content, size_t start, size_t stop)
{
	if(stop <= content.size())
	{
		while(start < stop)
		{
			if(content.at(start) != '\"')
			{
				std::cerr << "Name expected at position" << start << std::endl;
				throw E_NAME_EXPECTED;
			}

			size_t name_start = start + 1;
			size_t name_end   = findCorrespondingBracket(name_start-1, content);

			if((name_end+1) < stop)
			{
				if(content.at(name_end+1) == ':')
				{
					size_t value_start = name_end+2;
					if(value_start < stop)
					{
						std::string name = content.substr(name_start, name_end-name_start);
						size_t value_stop = value_start;

						if((content.at(value_start) == '{') || (content.at(value_start) == '['))  // New object
						{
							value_stop = findCorrespondingBracket(value_start, content);
							if(value_stop < stop)
							{
								size_t object_start = value_start+1;
								size_t object_stop  = value_stop;
								jsonNode* newNode = new jsonNode();
								newNode->setName(name);

								if(content.at(value_start) == '{')
									newNode->parseObject(content, object_start, object_stop);
								else
									newNode->parseArray(content, object_start, object_stop);

								_subnodes.push_back(newNode);
								++value_stop;
							}
							else
								throw E_BRACKET_EXPECTED;
						}
						else  // Value
						{
							// Go on until comma or stop
							value_stop = value_start;
							while(value_stop < stop)
							{
								// Skip strings:
								if(content.at(value_stop) == '\"')
								{
									value_stop = findCorrespondingBracket(value_stop, content) + 1;
									continue;
								}

								if((content.at(value_stop) == ',') || (content.at(value_stop) == '}') || (content.at(value_stop) == ']'))
									break;

								++value_stop;
							}

							std::string value = content.substr(value_start, value_stop-value_start);
							
							jsonNode* newNode = new jsonNode();
							newNode->setName(name);
							if(value == "null")
								newNode->value()->setNull();
							else if(value == "true")
								newNode->value()->setBool(true);
							else if(value == "false")
								newNode->value()->setBool(false);
							else if(value.at(0) == '\"')
							{
								if(value.size() > 2)
								{
									// Strip quotation marks from string:
									value.erase(0, 1);
									value.erase(value.size()-1, 1);

									newNode->value()->setString(value);
								}
								else
								{
									newNode->value()->setString("");
								}
							}
							else // number
							{
								if(isIntInString(value))
								{
									long v = atoi(value.c_str());
									newNode->value()->setInt(v);
								}
								else if(isFloatInString(value))
								{
									double v = atof(value.c_str());
									newNode->value()->setDouble(v);
								}								
							}

							_subnodes.push_back(newNode);
						}

						if(content.at(value_stop) == ',')
						{
							start = value_stop+1;
							continue;
						}
						else if(value_stop == stop)
							break;
						else
						{
							std::cerr << "Comma or end of object expected at position " << value_stop << ". Stop is at " << stop << ". "<< content.at(value_stop) << std::endl;
							throw E_COMMA_OR_END_OF_OBJECT_EXPECTED;
						}
					}
					else
						throw E_VALUE_EXPECTED;
				}
				else
					throw E_COLON_EXPECTED;
			}
			else
				throw E_COLON_EXPECTED;
		}
	}
	else
		throw E_STOP_POSITION_OUT_OF_BOUNDS;
}

void jsonNode::parseArray(const std::string &content, size_t start, size_t stop)
{
	_isArray = true;

	if(stop <= content.size())
	{
		while(start < stop)
		{
			size_t value_start = start;
			size_t value_stop = value_start;

			if((content.at(value_start) == '{') || (content.at(value_start) == '['))  // New object
			{
				value_stop = findCorrespondingBracket(value_start, content);
				if(value_stop < stop)
				{
					size_t object_start = value_start+1;
					size_t object_stop  = value_stop;
					jsonNode* newNode = new jsonNode();

					if(content.at(value_start) == '{')
						newNode->parseObject(content, object_start, object_stop);
					else
						newNode->parseArray(content, object_start, object_stop);

					_subnodes.push_back(newNode);
					++value_stop;
				}
				else
					throw E_BRACKET_EXPECTED;
			}
			else  // Value
			{
				// Go on until comma or stop
				value_stop = value_start;
				while(value_stop < stop)
				{
					// Skip strings:
					if(content.at(value_stop) == '\"')
					{
						value_stop = findCorrespondingBracket(value_stop, content) + 1;
						continue;
					}

					if((content.at(value_stop) == ',') || (content.at(value_stop) == '}') || (content.at(value_stop) == ']'))
						break;

					++value_stop;
				}

				std::string value = content.substr(value_start, value_stop-value_start);
				
				jsonNode* newNode = new jsonNode();
				if(value == "null")
					newNode->value()->setNull();
				else if(value == "true")
					newNode->value()->setBool(true);
				else if(value == "false")
					newNode->value()->setBool(false);
				else if(value.at(0) == '\"')
				{
					if(value.size() > 2)
					{
						// Strip quotation marks from string:
						value.erase(0, 1);
						value.erase(value.size()-1, 1);

						newNode->value()->setString(value);
					}
					else
					{
						newNode->value()->setString("");
					}
				}
				else // number
				{
					if(isIntInString(value))
					{
						long v = atoi(value.c_str());
						newNode->value()->setInt(v);
					}
					else if(isFloatInString(value))
					{
						double v = atof(value.c_str());
						newNode->value()->setDouble(v);
					}
				}

				_subnodes.push_back(newNode);
			}

			if(content.at(value_stop) == ',')
			{
				start = value_stop+1;
				continue;
			}
			else if(value_stop == stop)
				break;
			else
				throw E_COMMA_OR_END_OF_OBJECT_EXPECTED;
		}
	}
	else
		throw E_STOP_POSITION_OUT_OF_BOUNDS;
}

json::json()
{

}

json::json(const std::string &filename)
{
	setFilename(filename);
}

void json::setFilename(const std::string &filename)
{
	_filename = filename;
}

void json::readFile()
{
	_content.clear();

	// Read JSON file into private _content string:
	std::ifstream in(_filename, std::ios::in | std::ios::binary);
	if(in)
	{
		in.seekg(0, std::ios::end);
		_content.resize(in.tellg());
		in.seekg(0, std::ios::beg);
		in.read(&_content[0], _content.size());
		in.close();
	}
	else
	{
		throw E_CANNOT_READ_JSON;
	}
}

void json::parseFile(const std::string &filename)
{
	setFilename(filename);
	readFile();
	parse();
}

void json::setContent(const std::string &content)
{
	_content = content;
}

void json::parse()
{
	if(_content.size() > 0)
	{
		// Remove white space:
		size_t pos = 0;
		while(pos < _content.size())
		{
			// Skip escaped characters:
			if(_content.at(pos) == '\\')
			{
				pos += 2;
				continue;
			}

			// Skip strings:
			if(_content.at(pos) == '\"')
			{
				pos = findCorrespondingBracket(pos, _content) + 1;
				continue;
			}

			// Remove white space:
			if((_content.at(pos) == ' ') || (_content.at(pos) == '\t') || (_content.at(pos) == '\n') || (_content.at(pos) == '\r'))
			{
				_content.erase(pos, 1);
			}
			else
				++pos;
		}

		if(_content.at(0) == '{')
		{
			size_t start = 1;
			size_t stop = findCorrespondingBracket(0, _content);
			_root.parseObject(_content, start, stop);
		}
		else
		{
			throw E_CURLY_AT_JSONSTART_EXPECTED;
		}		
	}
}

jsonNode* json::root()
{
	return &_root;
}

jsonNode* json::element(const std::string &name)
{
	return _root.element(name);
}

jsonNode* json::element(size_t pos)
{
	return _root.element(pos);
}

bool json::exist(const std::string &name) const
{
	return _root.exist(name);
}

bool json::exist(size_t pos) const
{
	return _root.exist(pos);
}

bool json::existAndNotNull(const std::string &name)
{
	return _root.existAndNotNull(name);
}

bool json::existAndNotNull(size_t pos)
{
	return _root.existAndNotNull(pos);
}

std::string json::print() const
{
	std::stringstream ss;
	ss << _root.print(0);
	return ss.str();
}

size_t findCorrespondingBracket(size_t pos, const std::string &pstring) 
{
	char bracket = pstring.at(pos);
	char corrBracket;	// corresponding bracket
	int dir = 1;		// direction
	bool isQuote = false;

	switch(bracket)
	{
		case('('): corrBracket = ')'; break;
		case('['): corrBracket = ']'; break;
		case('{'): corrBracket = '}'; break;
		case('\"'): corrBracket = '\"'; isQuote=true; break;
		case('\''): corrBracket = '\''; isQuote=true; break;
		case(')'): corrBracket = '('; dir=-1; break;
		case(']'): corrBracket = '['; dir=-1; break;
		case('}'): corrBracket = '{'; dir=-1; break;
		default:
			std::cerr<<"Not a bracket: "<<bracket<<" at position "<<pos<<std::endl;
			throw E_NOT_A_BRACKET;
	}

	int level = 0;	// bracket level
	int position = static_cast<int>(pos);
	int max = static_cast<int>(pstring.length());
	position += dir;
	while(position>=0 && position<max)
	{
		// Skip escaped characters:
		if(pstring.at(position) == '\\')
		{
			position += dir;
			position += dir;
			continue;
		}

		if((pstring.at(position) == bracket) && !isQuote)
			level++;

		if(pstring.at(position) == corrBracket)
		{
			if(level == 0)
			{
				return position;
			}
			else
				level--;
		}

		position += dir;
	}

	std::cerr << "No corresponding bracket for "<< bracket << " at position "<< pos << std::endl;
	std::cerr << "Content:" << std::endl << pstring << std::endl;
	throw E_NO_CORRESPONDING_BRACKET;
}