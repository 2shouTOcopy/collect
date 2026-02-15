#pragma once

/// Signal handler setup for SIGINT, SIGTERM, SIGUSR1.
/// Decoupled from CollectDaemon to allow unit testing of signal logic.

class CollectDaemon;

namespace SignalHandler
{
	/// Install signal handlers. Call once at startup.
	void Install(CollectDaemon &daemon);

	/// Remove signal handlers. Call at shutdown.
	void Remove();
}
