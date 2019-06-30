#ifndef PCH_H
#define PCH_H

#pragma comment(lib, "version")
#pragma comment(lib, "mincore")

#include <Windows.h>
#include <string>
#include <iterator>
#include <tchar.h>

#include <sstream>
#include <fstream>

#include <array>
#include <vector>
#include <unordered_map>
#include <unordered_set>

#include <ctime>
#include <chrono>

#include <algorithm>
#include <future>
#include <functional>
#include <utility>

#include <filesystem>
#include <mutex>

#include <cinttypes>
#include <cctype>
#include <cassert>
#include <bitset>

#include "BypaPH.h"
#include "ImGUI/imgui.h"
#include "HttpWorker.h"

namespace fs = std::filesystem;
using namespace std::literals;

#define INVALID_POINTER_VALUE(x) ((x == (uintptr_t)-1) || x == NULL)
#endif //PCH_H
