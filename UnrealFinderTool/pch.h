#ifndef PCH_H
#define PCH_H

#pragma comment(lib, "version")
#pragma comment(lib, "mincore")

#include <Windows.h>
#include <string>
#include <sstream>
#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>
#include <iterator>
#include <future>
#include <algorithm>
#include <filesystem>
#include <mutex>
#include <cinttypes>
#include <cctype>
#include <cassert>
#include <tchar.h>
#include <chrono>
#include <fstream>
#include <bitset>
#include <functional>
#include <utility>

#include "BypaPH.h"
#include "ImGUI/imgui.h"
#include "HttpWorker.h"

namespace fs = std::filesystem;

#define INVALID_POINTER_VALUE(x) ((x == (uintptr_t)-1) || x == NULL)
#endif //PCH_H
