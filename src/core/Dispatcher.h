#pragma once

#include <string>
#include <vector>

#include "IPlugin.h"
#include "types/CdTime.h"

/// Single-threaded min-heap scheduler.
/// Each read plugin has an independent interval. The heap orders tasks by next
/// execution time, ensuring the earliest-due plugin runs first.
/// Supports exponential backoff on failure.

class Dispatcher
{
public:
	/// Maximum allowed read interval after backoff (default: 86400s).
	static constexpr double MAX_READ_INTERVAL_SEC = 86400.0;

	Dispatcher() = default;
	~Dispatcher() = default;

	Dispatcher(const Dispatcher &) = delete;
	Dispatcher &operator=(const Dispatcher &) = delete;

	/// Register a read plugin with its base interval.
	void RegisterRead(IPlugin *plugin, CdTime interval);

	/// Execute all tasks that are due. Returns time until next task is due.
	/// Called once per main loop iteration.
	CdTime RunOnce();

	/// Mark a plugin as failed (exponential backoff).
	void HandleReadFailure(const std::string &pluginName);

	/// Mark a plugin as succeeded (reset to base interval).
	void HandleReadSuccess(const std::string &pluginName);

	/// Number of registered tasks.
	size_t TaskCount() const { return m_heap.size(); }

private:
	struct ReadTask
	{
		IPlugin *plugin;
		CdTime nextRead;
		CdTime baseInterval;
		CdTime currentInterval;
		int consecutiveFailures;
	};

	/// Min-heap ordered by nextRead.
	std::vector<ReadTask> m_heap;

	/// Heap operations.
	void SiftUp(size_t index);
	void SiftDown(size_t index);

	/// Find task by plugin name.
	int FindTaskIndex(const std::string &name) const;
};
