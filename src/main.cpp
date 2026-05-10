#include "app/CollectDaemon.h"

#include <cstdlib>
#include <iostream>

int main(int argc, char **argv)
{
	try
	{
		CollectDaemon daemon;

		if (argc >= 2 && std::string(argv[1]) == "snapshot")
		{
			return daemon.RunSnapshotCommand(argc, argv) == 0
				? EXIT_SUCCESS
				: EXIT_FAILURE;
		}

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
