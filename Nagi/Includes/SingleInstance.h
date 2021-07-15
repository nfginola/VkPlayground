#pragma once

namespace Nagi
{

template <typename T>
class SingleInstance
{
public:
	SingleInstance()
	{
		if (m_initialized)
			assert(false);	// or maybe throw exception
		m_initialized = true;
	}

	~SingleInstance()
	{
		m_initialized = false;
	}

	SingleInstance(const SingleInstance&) = delete;
	SingleInstance& operator=(const SingleInstance&) = delete;

private:
	inline static bool m_initialized = false;


};



}
