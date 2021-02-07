#include <string>
#include <thread>
#include "logger.h"

int main(int argc, char const *argv[])
{
	std::string configFile = "sensorlogger.json";

	if(argc > 1)
	{
		configFile = argv[1];
	}

	logger me;

	// Start up and read configuration:
	try
	{		
		me.loadConfig(configFile);
		me.setUpConnections();
	}
	catch(int e)
	{
		std::cerr<<"Error: "<<e<<std::endl;
		return e;
	}

	// Infinite measurement loop
	while(true)
	{
		me.trigger();
		std::this_thread::sleep_for(std::chrono::milliseconds(100));
	}

	return 0;
}