#include "app/CollectDaemon.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	try
	{
		CollectDaemon daemon;

		if (daemon.Configure(argc, argv) != 0)
		{
			return EXIT_FAILURE;
		}

		return daemon.Run();
	}
	catch (const std::exception &e)
	{
		std::cerr << "[collect] Fatal: " << e.what() << "\n";
		return EXIT_FAILURE;
	}
}
