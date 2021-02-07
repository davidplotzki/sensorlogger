#ifndef _READOUTBUFFER_H
#define _READOUTBUFFER_H

#define E_UNABLE_TO_OPEN_FILE 3001

#define BUFFERTIME 800  // milliseconds
#define N_TRIALS 2

#include <string>
#include <sstream>
#include <iostream>
#include <fstream>
#include <vector>
#include <thread>

class logger;

class readoutFile
{
private:
	std::string _content;
	uint64_t    _last_read_timestamp;
	bool        _isHTTP;
public:
	std::string _filename;

	readoutFile(const std::string& filename);
	~readoutFile();

	void setFilename(const std::string& filename);
	void cleanUp(uint64_t currentTimestamp);
	std::string getContent(logger* root, uint64_t currentTimestamp);

};

class readoutBuffer
{
private:
	std::vector<readoutFile*> _files;
	logger* _root;

public:
	readoutBuffer(logger* root);
	~readoutBuffer();

	std::string getFileContents(const std::string& filename, uint64_t currentTimestamp);

	void cleanUp(uint64_t currentTimestamp);
	void clear();
};

#endif