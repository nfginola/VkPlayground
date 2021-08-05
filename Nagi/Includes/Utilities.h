#pragma once
#include <string>
#include <vector>

namespace Nagi
{

std::vector<uint8_t> readFile(const std::string& filePath);


uint32_t getAlignedSize(uint32_t size, uint32_t toAlignWith);


}

