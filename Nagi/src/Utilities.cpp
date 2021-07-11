#include "pch.h"
#include "Utilities.h"

namespace Nagi
{

namespace Utils
{
	std::vector<char> readFile(const std::string& filePath)
	{
		std::ifstream file(filePath, std::ios::ate | std::ios::binary);
		if (!file.is_open())
			assert(false);	// failed to open file

		size_t fileSize = static_cast<size_t>(file.tellg());
		std::vector<char> buffer(fileSize);

		file.seekg(0);  // go back to the beginning
		file.read(buffer.data(), fileSize);     // read "filSize" from file, put into buffer.data()

		file.close();

		return buffer;
	}

}

}


