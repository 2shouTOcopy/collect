#pragma once

#include <cstddef>
#include <vector>

#include "types/DataSet.h"
#include "types/ValueList.h"
#include "IPlugin.h"

/// Bounded write queue — decouples read (collection) from write (output).
/// Replaces unbounded RstDispatcher with a configurable-size queue.
/// When full, drops oldest entry and logs a warning.

class WriteQueue
{
public:
	static constexpr size_t DEFAULT_CAPACITY = 1024;

	explicit WriteQueue(size_t capacity = DEFAULT_CAPACITY);
	~WriteQueue() = default;

	WriteQueue(const WriteQueue &) = delete;
	WriteQueue &operator=(const WriteQueue &) = delete;

	/// Enqueue a value list for writing. Drops oldest if full.
	void Enqueue(const DataSet &ds, const ValueList &vl);

	/// Process up to maxBatch items from the queue, calling Write on each writer.
	/// Returns number of items processed.
	size_t DrainBatch(const std::vector<IPlugin *> &writers, size_t maxBatch);

	/// Process all remaining items (for shutdown).
	size_t DrainAll(const std::vector<IPlugin *> &writers);

	/// Current queue size.
	size_t Size() const { return m_size; }

	/// Whether the queue is empty.
	bool Empty() const { return m_size == 0; }

	/// Number of dropped entries since last reset.
	size_t DroppedCount() const { return m_dropped; }

private:
	struct QueueEntry
	{
		DataSet ds;
		ValueList vl;
	};

	std::vector<QueueEntry> m_buffer;
	size_t m_capacity;
	size_t m_head;     // next write position
	size_t m_tail;     // next read position
	size_t m_size;     // current number of entries
	size_t m_dropped;  // total dropped entries
};
