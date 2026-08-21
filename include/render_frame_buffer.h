#pragma once

#include <atomic>
#include <vector>

#include "render_frame.h"

namespace ZeroX
{
    class RenderFrameBuffer
    {
    public:
        void allocate(size_t capacity);
        bool push(const RenderFrame& frame);
        bool pop(RenderFrame& outFrame);

    private:
        std::vector<RenderFrame> m_data;

        alignas(64) std::atomic<size_t> m_head  { 0 };
        alignas(64) std::atomic<size_t> m_tail  { 0 };
        size_t m_capacityMask                   { 0 };
    };
}
