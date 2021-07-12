#pragma once
// Notes
// Project settings >> C++ >> Precompiled Headers >> Set to Use (/Yu) and set this file
// File setting for pch.cpp >> Set Create (/Yc)

#include <assert.h>
#include <stdint.h>

#include <iostream>
#include <string>
#include <fstream>
#include <vector>
#include <array>
#include <functional>
#include <exception>
#include <optional>

// Testing PCH build times with libs below
#include <algorithm>
#include <functional>
#include <memory>
#include <thread>
#include <utility>

// Data structures
#include <stack>
#include <deque>
#include <set>
#include <map>
#include <unordered_set>
#include <unordered_map>