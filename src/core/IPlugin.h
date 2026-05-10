#pragma once

#include <string>
#include <functional>
#include "types/DataSet.h"
#include "types/ValueList.h"
#include "types/CdTime.h"
#include "snapshot/SnapshotContext.h"

/// Plugin interface (replaces CAbstractUserModule).
/// Plugins declare their capabilities via HasRead()/HasWrite()/HasFlush().
/// PluginManager auto-classifies plugins into read/write/flush lists at registration.

class IPlugin
{
public:
	virtual ~IPlugin() = default;

	/// Unique plugin name (e.g. "cpu", "csv").
	virtual std::string Name() const = 0;

	/// Capability declaration — return true if this plugin supports the callback.
	virtual bool HasRead()  const { return false; }
	virtual bool HasWrite() const { return false; }
	virtual bool HasFlush() const { return false; }
	virtual bool HasSnapshot() const { return false; }

	/// Lifecycle: configuration from collect.conf key-value pairs.
	virtual int Configure(const std::string &key, const std::string &val)
	{
		(void)key;
		(void)val;
		return 0;
	}

	/// Lifecycle: one-time initialization (open files, allocate buffers, etc.).
	virtual int Init() { return 0; }

	/// Lifecycle: cleanup (close files, release resources).
	virtual int Shutdown() { return 0; }

	/// Read callback: perform one data collection cycle.
	/// Called by Dispatcher when this plugin's interval expires.
	/// Read plugins should call Dispatch() to push collected values.
	virtual int Read() { return 0; }

	/// Write callback: output one ValueList.
	/// Called by WriteQueue::DrainBatch for each queued data point.
	virtual int Write(const DataSet &ds, const ValueList &vl)
	{
		(void)ds;
		(void)vl;
		return 0;
	}

	/// Flush callback: force-flush buffered output.
	/// Triggered by SIGUSR1 or IPC command.
	virtual int Flush(CdTime timeout)
	{
		(void)timeout;
		return 0;
	}

	/// Snapshot callback: write one-shot diagnostic files under ctx.snapshotDir.
	virtual int Snapshot(const SnapshotContext &ctx)
	{
		(void)ctx;
		return 0;
	}

	// ─── Dispatch mechanism ──────────────────────────────────

	/// Callback type: takes DataSet + ValueList, pushes to WriteQueue.
	using DispatchFunc = std::function<void(const DataSet &, const ValueList &)>;

	/// Set the dispatch callback (called by CollectDaemon during wiring).
	void SetDispatchCallback(DispatchFunc fn) { m_dispatchFn = fn; }

protected:
	/// Call this from Read() to push collected values to the write queue.
	void Dispatch(const DataSet &ds, const ValueList &vl)
	{
		if (m_dispatchFn)
		{
			m_dispatchFn(ds, vl);
		}
	}

private:
	DispatchFunc m_dispatchFn;
};

/// Exported C functions from each plugin .so:
///   IPlugin* CreateModule();
///   void     DestroyModule(IPlugin *);
using PfnCreateModule  = IPlugin *(*)();
using PfnDestroyModule = void (*)(IPlugin *);
