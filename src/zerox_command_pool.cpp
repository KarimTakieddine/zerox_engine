#include <stdexcept>

#include "zerox_command_pool.h"

namespace ZeroX
{
	void CommandPool::allocate(size_t capacity)
	{
		if (capacity < 2 || (capacity & (capacity - 1)) != 0)
			throw std::runtime_error("Capacity must be a power of 2!");

		m_data.resize(capacity);

		m_capacityMask = capacity - 1;

		m_head.store(0, std::memory_order_release);
		m_tail.store(0, std::memory_order_release);

		m_cachedHead = 0;
		m_cachedTail = 0;
	}

	bool CommandPool::push(CommandBuffer buffer)
	{
		const size_t current_tail	= m_tail.load(std::memory_order_relaxed);

		const size_t next_tail		= (current_tail + 1) & m_capacityMask;

		if (next_tail == m_cachedHead)
		{
			m_cachedHead = m_head.load(std::memory_order_acquire);

			if (next_tail == m_cachedHead)
			{
				return false;
			}
		}

		m_data[current_tail] = std::move(buffer);

		m_tail.store(next_tail, std::memory_order_release);

		return true;
	}

	bool CommandPool::pop(CommandBuffer& buffer)
	{
		const size_t current_head = m_head.load(std::memory_order_relaxed);

		if (current_head == m_cachedTail)
		{
			m_cachedTail = m_tail.load(std::memory_order_acquire);

			if (current_head == m_cachedTail)
			{
				return false;
			}
		}

		buffer = std::move(m_data[current_head]);

		m_head.store((current_head + 1) & m_capacityMask, std::memory_order_release);

		return true;
	}
}