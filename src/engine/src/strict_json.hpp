#pragma once

#include <nlohmann/json.hpp>
#include <cmath>
#include <limits>
#include <stdexcept>
#include <string_view>
#include <type_traits>
#include <unordered_set>
#include <vector>

namespace rat::detail {

inline nlohmann::json parse_unique_json(std::string_view text) {
  using json = nlohmann::json;
  std::vector<std::unordered_set<std::string>> keys;
  return json::parse(text, [&keys](int, json::parse_event_t event, json& value) {
    if (event == json::parse_event_t::object_start) keys.emplace_back();
    if (event == json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
      throw std::runtime_error("duplicate JSON object key: " + value.get<std::string>());
    if (event == json::parse_event_t::object_end) keys.pop_back();
    return true;
  });
}

// nlohmann get<int/float> permits narrowing and float-to-int truncation. All
// persisted numbers pass this check before conversion, never after allocation.
template<class T> T checked_number(const nlohmann::json& value) {
  if constexpr (std::is_integral_v<T>) {
    if (!value.is_number_integer()) throw std::runtime_error("expected integral number");
    if (value.is_number_unsigned()) {
      const auto n = value.get<std::uint64_t>();
      if (n > static_cast<std::uint64_t>((std::numeric_limits<T>::max)()))
        throw std::runtime_error("integer out of range");
    } else {
      const auto n = value.get<std::int64_t>();
      if constexpr (std::is_unsigned_v<T>) {
        if (n < 0 || static_cast<std::uint64_t>(n) > (std::numeric_limits<T>::max)())
          throw std::runtime_error("integer out of range");
      } else if (n < (std::numeric_limits<T>::min)() || n > (std::numeric_limits<T>::max)())
        throw std::runtime_error("integer out of range");
    }
  } else {
    if (!value.is_number()) throw std::runtime_error("expected number");
    const double n = value.get<double>();
    if (!std::isfinite(n) || std::abs(n) > (std::numeric_limits<T>::max)())
      throw std::runtime_error("nonfinite or out-of-range number");
  }
  return value.get<T>();
}

template<class T> T checked_value(const nlohmann::json& object, const char* key, T fallback) {
  return object.contains(key) ? checked_number<T>(object.at(key)) : fallback;
}

inline void exact_fields(const nlohmann::json& value, std::initializer_list<const char*> fields) {
  if (!value.is_object() || value.size() != fields.size())
    throw std::runtime_error("missing or unknown fields");
  for (const auto* key : fields) if (!value.contains(key)) throw std::runtime_error("missing required field");
}

}  // namespace rat::detail
