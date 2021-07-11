#pragma once

namespace Nagi
{

namespace Utils
{

template <typename T>
class Singleton
{
public:
	Singleton()
	{
		if (m_initialized)
			assert(false);	// or maybe throw exception
		m_initialized = true;
	}

	~Singleton()
	{
		m_initialized = false;
	}

	Singleton(const Singleton&) = delete;
	Singleton& operator=(const Singleton&) = delete;

private:
	inline static bool m_initialized = false;


};

}



}
