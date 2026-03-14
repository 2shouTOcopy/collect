#include "Dispatcher.h"

#include <algorithm>
#include <chrono>
#include <iostream>

void Dispatcher::RegisterRead(IPlugin *plugin, CdTime interval, bool immediate)
{
	if (plugin == nullptr)
	{
		return;
	}

	ReadTask task;
	task.plugin = plugin;
	task.baseInterval = interval;
	task.currentInterval = interval;
	task.nextRead = immediate ? CdTime::Now() : CdTime::Now() + interval;
	task.consecutiveFailures = 0;

	m_heap.push_back(task);
	SiftUp(m_heap.size() - 1);

	std::cerr << "[Dispatcher] Registered: " << plugin->Name()
	          << " interval=" << interval.ToDouble() << "s\n";
}

CdTime Dispatcher::RunOnce()
{
	if (m_heap.empty())
	{
		// No tasks — sleep for 1 second
		return CdTime::FromDouble(1.0);
	}

	CdTime now = CdTime::Now();

	// Execute all due tasks
	while (!m_heap.empty() && m_heap[0].nextRead <= now)
	{
		ReadTask &task = m_heap[0];

		// Measure read duration
		auto readStart = std::chrono::steady_clock::now();
		int ret = task.plugin->Read();
		auto readEnd = std::chrono::steady_clock::now();

		double readDurationSec = std::chrono::duration<double>(
			readEnd - readStart).count();

		// Warn if read took longer than the plugin's interval
		double intervalSec = task.currentInterval.ToDouble();
		if (readDurationSec > intervalSec)
		{
			std::cerr << "[Dispatcher] WARNING: '" << task.plugin->Name()
			          << "' Read() took " << readDurationSec
			          << "s (interval=" << intervalSec << "s)\n";
		}
		else if (readDurationSec > intervalSec * 0.8)
		{
			std::cerr << "[Dispatcher] NOTICE: '" << task.plugin->Name()
			          << "' Read() took " << readDurationSec
			          << "s (80%+ of interval=" << intervalSec << "s)\n";
		}

		if (ret == 0)
		{
			HandleReadSuccess(task.plugin->Name());
		}
		else
		{
			HandleReadFailure(task.plugin->Name());
		}

		// Update next execution time
		now = CdTime::Now();
		m_heap[0].nextRead = now + m_heap[0].currentInterval;
		SiftDown(0);
	}

	// Return time until next task
	if (m_heap.empty())
	{
		return CdTime::FromDouble(1.0);
	}

	CdTime remaining = m_heap[0].nextRead - CdTime::Now();
	if (remaining.Raw() == 0)
	{
		return CdTime::FromDouble(0.001);
	}
	return remaining;
}

void Dispatcher::HandleReadFailure(const std::string &pluginName)
{
	int idx = FindTaskIndex(pluginName);
	if (idx < 0)
	{
		return;
	}

	ReadTask &task = m_heap[static_cast<size_t>(idx)];
	task.consecutiveFailures++;

	// Exponential backoff: currentInterval = baseInterval * 2^failures
	double backoffSec = task.baseInterval.ToDouble();
	for (int i = 0; i < task.consecutiveFailures; ++i)
	{
		backoffSec *= 2.0;
		if (backoffSec >= MAX_READ_INTERVAL_SEC)
		{
			backoffSec = MAX_READ_INTERVAL_SEC;
			break;
		}
	}

	task.currentInterval = CdTime::FromDouble(backoffSec);
	std::cerr << "[Dispatcher] Read failed for '" << pluginName
	          << "' (failures=" << task.consecutiveFailures
	          << ", next interval=" << backoffSec << "s)\n";
}

void Dispatcher::HandleReadSuccess(const std::string &pluginName)
{
	int idx = FindTaskIndex(pluginName);
	if (idx < 0)
	{
		return;
	}

	ReadTask &task = m_heap[static_cast<size_t>(idx)];
	if (task.consecutiveFailures > 0)
	{
		std::cerr << "[Dispatcher] '" << pluginName
		          << "' recovered, resetting interval\n";
	}
	task.consecutiveFailures = 0;
	task.currentInterval = task.baseInterval;
}

int Dispatcher::FindTaskIndex(const std::string &name) const
{
	for (size_t i = 0; i < m_heap.size(); ++i)
	{
		if (m_heap[i].plugin->Name() == name)
		{
			return static_cast<int>(i);
		}
	}
	return -1;
}

void Dispatcher::SiftUp(size_t index)
{
	while (index > 0)
	{
		size_t parent = (index - 1) / 2;
		if (m_heap[index].nextRead < m_heap[parent].nextRead)
		{
			std::swap(m_heap[index], m_heap[parent]);
			index = parent;
		}
		else
		{
			break;
		}
	}
}

void Dispatcher::SiftDown(size_t index)
{
	size_t size = m_heap.size();
	while (true)
	{
		size_t smallest = index;
		size_t left = 2 * index + 1;
		size_t right = 2 * index + 2;

		if (left < size && m_heap[left].nextRead < m_heap[smallest].nextRead)
		{
			smallest = left;
		}
		if (right < size && m_heap[right].nextRead < m_heap[smallest].nextRead)
		{
			smallest = right;
		}

		if (smallest != index)
		{
			std::swap(m_heap[index], m_heap[smallest]);
			index = smallest;
		}
		else
		{
			break;
		}
	}
}
