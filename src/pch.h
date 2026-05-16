#pragma once

#ifdef _MSC_VER
#	pragma warning(push)
#	pragma warning(disable: 6011)  // CommonLibVR BSTHashMap iterator analysis false positive
#endif
#include <SKSE/SKSE.h>
#ifdef _MSC_VER
#	pragma warning(pop)
#endif

#include <algorithm>
#include <array>
#include <atomic>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <cwctype>
#include <deque>
#include <exception>
#include <filesystem>
#include <fstream>
#include <memory>
#include <mutex>
#include <optional>
#include <string>
#include <string_view>
#include <thread>
#include <vector>

namespace logs = SKSE::log;
