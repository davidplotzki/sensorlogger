#include "readoutbuffer.h"
#include "logger.h"

readoutFile::readoutFile(const std::string& filename)
{
	_content.clear();
	_last_read_timestamp = 0;

	setFilename(filename);
}

readoutFile::~readoutFile()
{
	_content.clear();
	_filename.clear();
}

void readoutFile::setFilename(const std::string& filename)
{
	_filename = filename;
	_isHTTP = false;

	if(_filename.size() >= 7)  // http:// ?
	{
		if(_filename.substr(0, 7) == "http://")
			_isHTTP = true;
	}

	if(_filename.size() >= 8)  // https:// ?
	{
		if(_filename.substr(0, 8) == "https://")
			_isHTTP = true;
	}
}

void readoutFile::cleanUp(uint64_t currentTimestamp)
{
	if((currentTimestamp - _last_read_timestamp) > BUFFERTIME)
	{
		_content.clear();
	}
}

std::string readoutFile::getContent(logger* root, uint64_t currentTimestamp)
{
	if((currentTimestamp - _last_read_timestamp) > BUFFERTIME)
	{
		_content.clear();

		if(_isHTTP)
		{
			try
			{
				_content = root->httpRequest(_filename);
			}
			catch(int e)
			{
				root->message("Cannot read from: " + _filename, true);
			}
		}
		else   // file in file system
		{
			// Try twice:
			for(int i=0; i<N_TRIALS; ++i)
			{
				std::ifstream fs(_filename);
				if(fs.is_open())
				{
					std::stringstream strStream;
					strStream<<fs.rdbuf();
					_content = strStream.str();

					fs.close();
					break;
				}

				if(i >= (N_TRIALS-1))
				{
					std::cerr<<"Cannot open file: "<<_filename<<std::endl;
					throw E_UNABLE_TO_OPEN_FILE;
				}

				std::this_thread::sleep_for(std::chrono::milliseconds(100));
			}
		}

		_last_read_timestamp = currentTimestamp;
	}
	
	return _content;
}

readoutBuffer::readoutBuffer(logger* root)
{
	_root = root;
}

void readoutBuffer::cleanUp(uint64_t currentTimestamp)
{
	for(unsigned i=0; i<_files.size(); ++i)
		_files.at(i)->cleanUp(currentTimestamp);
}

void readoutBuffer::clear()
{
	for(unsigned i=0; i<_files.size(); ++i)
		delete _files.at(i);

	_files.clear();
}

readoutBuffer::~readoutBuffer()
{
	clear();
}

std::string readoutBuffer::getFileContents(const std::string& filename, uint64_t currentTimestamp)
{
	for(unsigned i=0; i<_files.size(); ++i)
	{
		if(_files.at(i)->_filename == filename)
			return _files.at(i)->getContent(_root, currentTimestamp);
	}

	readoutFile* f = new readoutFile(filename);
	_files.push_back(f);
	return f->getContent(_root, currentTimestamp);
}