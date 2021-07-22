#pragma once
#include <chrono>


namespace Nagi
{

	class Timer
	{
	public:
		Timer();
		float time() const;

	private:
		std::chrono::system_clock::time_point m_start;

	};

}
