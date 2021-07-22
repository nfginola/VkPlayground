#include "pch.h"
#include "Timer.h"

namespace Nagi
{
    Timer::Timer()
    {
        m_start = std::chrono::system_clock::now();
    }
    float Timer::time() const
    {
        auto timeEnd = std::chrono::system_clock::now();
        std::chrono::duration<float> diff = timeEnd - m_start;
        return diff.count();
    }

}
