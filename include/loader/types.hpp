#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>
#include <map>
#include <unordered_map>
#include <memory>
#include <functional>
#include <chrono>
#include <optional>
#include <variant>
#include <span>
#include <mutex>
#include <shared_mutex>
#include <atomic>
#include <future>
#include <queue>
#include <condition_variable>

namespace xiaopeng {
namespace loader {

using Byte = std::uint8_t;
using ByteBuffer = std::vector<Byte>;
using StringView = std::string_view;
using TimePoint = std::chrono::steady_clock::time_point;
using Duration = std::chrono::milliseconds;

template<typename T>
using Ptr = std::shared_ptr<T>;

template<typename T>
using WeakPtr = std::weak_ptr<T>;

template<typename T>
using UniquePtr = std::unique_ptr<T>;

template<typename T>
using Atomic = std::atomic<T>;

template<typename... Args>
using Callback = std::function<Args...>;

template<typename T>
using Future = std::future<T>;

template<typename T>
using Promise = std::promise<T>;

template<typename K, typename V, typename Hash = std::hash<K>>
using HashMap = std::unordered_map<K, V, Hash>;

template<typename K, typename V>
using TreeMap = std::map<K, V>;

}
}
