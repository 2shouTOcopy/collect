#include "WriteQueue.h"

#include <iostream>

WriteQueue::WriteQueue(size_t capacity)
	: m_buffer(capacity)
	, m_capacity(capacity)
	, m_head(0)
	, m_tail(0)
	, m_size(0)
	, m_dropped(0)
{
}

void WriteQueue::Enqueue(const DataSet &ds, const ValueList &vl)
{
	if (m_size >= m_capacity)
	{
		// Drop oldest entry — advance tail
		m_tail = (m_tail + 1) % m_capacity;
		--m_size;
		++m_dropped;
		std::cerr << "[WriteQueue] Queue full, dropped oldest (total dropped: "
		          << m_dropped << ")\n";
	}

	m_buffer[m_head].ds = ds;
	m_buffer[m_head].vl = vl;
	m_head = (m_head + 1) % m_capacity;
	++m_size;
}

size_t WriteQueue::DrainBatch(const std::vector<IPlugin *> &writers,
                               size_t maxBatch)
{
	size_t processed = 0;

	while (m_size > 0 && processed < maxBatch)
	{
		const QueueEntry &entry = m_buffer[m_tail];

		for (auto *writer : writers)
		{
			int ret = writer->Write(entry.ds, entry.vl);
			if (ret != 0)
			{
				std::cerr << "[WriteQueue] Write failed for plugin: "
				          << writer->Name() << "\n";
			}
		}

		m_tail = (m_tail + 1) % m_capacity;
		--m_size;
		++processed;
	}

	return processed;
}

size_t WriteQueue::DrainAll(const std::vector<IPlugin *> &writers)
{
	return DrainBatch(writers, m_size);
}
