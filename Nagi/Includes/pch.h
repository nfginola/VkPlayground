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


#include <filesystem>

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

// Important GLM defines
#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE // forces [0, 1] instead of [-1, -1] on persp matrix
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES  // make sure to align math types (for UBO --> there are alignment reqs)
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>