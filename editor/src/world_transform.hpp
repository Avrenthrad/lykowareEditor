#pragma once
#include "command_bus.hpp"
#include "transform_json.hpp"
#include <numbers>
#include <stdexcept>
namespace lyko {
using Matrix4 = std::array<double, 16>;
inline Matrix4 identity_matrix() {
  return {1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1, 0, 0, 0, 0, 1};
}
inline Matrix4 multiply_matrix(const Matrix4 &a, const Matrix4 &b) {
  Matrix4 result{};
  for (std::size_t row = 0; row < 4; ++row) {
    for (std::size_t col = 0; col < 4; ++col) {
      for (std::size_t k = 0; k < 4; ++k) {
        const double term = a[row * 4 + k] * b[k * 4 + col];
        if (!std::isfinite(term)) {
          throw std::overflow_error(
              "World transform exceeds finite double precision.");
        }
        result[row * 4 + col] += term;
        if (!std::isfinite(result[row * 4 + col])) {
          throw std::overflow_error(
              "World transform exceeds finite double precision.");
        }
      }
    }
  }
  return result;
}
inline Matrix4 local_matrix(const Transform &transform) {
  if (auto error = transform_error(transform)) {
    throw std::invalid_argument(*error);
  }
  const auto &v = transform.values;
  const auto radians = [](double degrees) {
    return std::remainder(degrees, 360.0) * (std::numbers::pi / 180.0);
  };
  const double x = radians(v[3]), y = radians(v[4]), z = radians(v[5]);
  auto t = identity_matrix(), s = identity_matrix();
  t[3] = v[0];
  t[7] = v[1];
  t[11] = v[2];
  s[0] = v[6];
  s[5] = v[7];
  s[10] = v[8];
  const Matrix4 rx{1, 0, 0,           0,           0, std::cos(x), -std::sin(x),
                   0, 0, std::sin(x), std::cos(x), 0, 0,           0,
                   0, 1};
  const Matrix4 ry{std::cos(y),  0, std::sin(y), 0, 0, 1, 0, 0,
                   -std::sin(y), 0, std::cos(y), 0, 0, 0, 0, 1};
  const Matrix4 rz{std::cos(z), -std::sin(z),
                   0,           0,
                   std::sin(z), std::cos(z),
                   0,           0,
                   0,           0,
                   1,           0,
                   0,           0,
                   0,           1};
  return multiply_matrix(
      multiply_matrix(multiply_matrix(multiply_matrix(t, rx), ry), rz), s);
}
inline Matrix4 world_matrix(const Document &document, EntityId id,
                            bool validate_forest = true) {
  if (!document.entities.contains(id)) {
    throw std::invalid_argument("Entity does not exist.");
  }
  if (validate_forest) {
    if (auto error = hierarchy_error(document)) {
      throw std::invalid_argument(*error);
    }
  }
  std::vector<EntityId> chain{id};
  while (document.parents.contains(chain.back())) {
    const auto parent = document.parents.at(chain.back());
    if (chain.size() > 64 || !document.entities.contains(parent)) {
      throw std::invalid_argument("Invalid ancestor chain.");
    }
    chain.push_back(parent);
  }
  auto result = identity_matrix();
  for (auto it = chain.rbegin(); it != chain.rend(); ++it) {
    result =
        multiply_matrix(result, local_matrix(document.transforms.contains(*it)
                                                 ? document.transforms.at(*it)
                                                 : Transform{}));
  }
  return result;
}
inline Json world_transform_json(const Document &document, EntityId id) {
  Json::Array matrix;
  for (double value : world_matrix(document, id)) {
    matrix.emplace_back(value);
  }
  return Json::Object{{"id", id},
                      {"matrix", std::move(matrix)},
                      {"storage", "row_major"},
                      {"vectors", "column"},
                      {"local_order", "T*Rx*Ry*Rz*S"},
                      {"handedness", "right"},
                      {"up_axis", "Y"},
                      {"forward_axis", "-Z"},
                      {"units", "editor_units"}};
}
} // namespace lyko
