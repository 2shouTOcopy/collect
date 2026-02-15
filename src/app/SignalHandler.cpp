#include "SignalHandler.h"
#include "CollectDaemon.h"

#include <csignal>

namespace
{
	CollectDaemon *g_pDaemon = nullptr;

	void OnTerminate(int /*sig*/)
	{
		if (g_pDaemon != nullptr)
		{
			g_pDaemon->RequestStop();
		}
	}
}

namespace SignalHandler
{
	void Install(CollectDaemon &daemon)
	{
		g_pDaemon = &daemon;

		struct sigaction sa = {};
		sa.sa_handler = OnTerminate;
		sigemptyset(&sa.sa_mask);
		sa.sa_flags = 0;

		sigaction(SIGINT,  &sa, nullptr);
		sigaction(SIGTERM, &sa, nullptr);

		// TODO Phase 4: SIGUSR1 → trigger flush
	}

	void Remove()
	{
		signal(SIGINT,  SIG_DFL);
		signal(SIGTERM, SIG_DFL);
		g_pDaemon = nullptr;
	}
}
