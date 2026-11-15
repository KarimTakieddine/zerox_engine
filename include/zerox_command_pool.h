#pragma once

#include <atomic>

#include "zerox_command_buffer.h"

namespace ZeroX
{
    inline constexpr size_t CACHE_LINE_SIZE{ 1ULL << 6 };

	struct CommandPool
	{
	public:
        void allocate(size_t capacity);
        bool push(CommandBuffer buffer);
        bool pop(CommandBuffer& buffer);

	private:
		std::vector<CommandBuffer> m_data;
        alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_head{ 0 };
		size_t m_cachedHead{ 0 };
        alignas(CACHE_LINE_SIZE) std::atomic<size_t> m_tail{ 0 };
		size_t m_cachedTail{ 0 };
        size_t m_capacityMask{ 0 };
	};
}
