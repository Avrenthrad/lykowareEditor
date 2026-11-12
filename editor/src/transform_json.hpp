#pragma once
#include "json.hpp"
#include "transform.hpp"
namespace lyko {
inline double json_number(const Json &value) {
  if (auto p = std::get_if<double>(&value.value)) {
    return *p;
  }
  if (auto p = std::get_if<std::int64_t>(&value.value)) {
    return static_cast<double>(*p);
  }
  if (auto p = std::get_if<std::uint64_t>(&value.value)) {
    return static_cast<double>(*p);
  }
  throw std::invalid_argument("Expected a number.");
}
inline Json transform_json(const Transform &transform) {
  Json::Object object;
  for (std::size_t i = 0; i < 9; ++i) {
    object.emplace(transform_fields[i], transform.values[i]);
  }
  return object;
}
inline Transform parse_transform(const Json &value) {
  if (value.as<Json::Object>().size() != 9) {
    throw std::invalid_argument("Expected nine transform components.");
  }
  Transform result;
  for (std::size_t i = 0; i < 9; ++i) {
    result.values[i] = json_number(value.at(transform_fields[i]));
  }
  if (auto error = transform_error(result)) {
    throw std::invalid_argument(*error);
  }
  return result;
}
} // namespace lyko
