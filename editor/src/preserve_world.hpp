#pragma once
#include "world_transform.hpp"
#include <algorithm>
namespace lyko {
inline bool matrix_near(const Matrix4 &a, const Matrix4 &b) {
  for (std::size_t i = 0; i < 16; ++i) {
    const auto delta = std::abs(a[i] - b[i]);
    if (!std::isfinite(delta) || delta > 1e-9 * std::max(1.0, std::abs(a[i]))) {
      return false;
    }
  }
  return true;
}
inline Matrix4 inverse_matrix(Matrix4 matrix) {
  auto inverse = identity_matrix();
  for (std::size_t col = 0; col < 4; ++col) {
    auto pivot = col;
    for (std::size_t row = col + 1; row < 4; ++row) {
      if (std::abs(matrix[row * 4 + col]) > std::abs(matrix[pivot * 4 + col])) {
        pivot = row;
      }
    }
    if (std::abs(matrix[pivot * 4 + col]) < 1e-12) {
      throw std::invalid_argument(
          "Parent matrix is singular or too poorly scaled to invert safely.");
    }
    for (std::size_t j = 0; j < 4; ++j) {
      std::swap(matrix[col * 4 + j], matrix[pivot * 4 + j]);
      std::swap(inverse[col * 4 + j], inverse[pivot * 4 + j]);
    }
    const auto divisor = matrix[col * 4 + col];
    for (std::size_t j = 0; j < 4; ++j) {
      matrix[col * 4 + j] /= divisor;
      inverse[col * 4 + j] /= divisor;
    }
    for (std::size_t row = 0; row < 4; ++row) {
      if (row == col) {
        continue;
      }
      const auto factor = matrix[row * 4 + col];
      for (std::size_t j = 0; j < 4; ++j) {
        matrix[row * 4 + j] -= factor * matrix[col * 4 + j];
        inverse[row * 4 + j] -= factor * inverse[col * 4 + j];
      }
    }
  }
  for (double value : inverse) {
    if (!std::isfinite(value)) {
      throw std::invalid_argument("Parent inverse exceeds finite precision.");
    }
  }
  return inverse;
}
inline Transform decompose_local(const Matrix4 &matrix) {
  Transform result;
  auto &v = result.values;
  v[0] = matrix[3];
  v[1] = matrix[7];
  v[2] = matrix[11];
  Matrix4 rotation = identity_matrix();
  for (std::size_t col = 0; col < 3; ++col) {
    const auto scale =
        std::hypot(matrix[col], matrix[4 + col], matrix[8 + col]);
    if (!std::isfinite(scale) || scale < 1e-12) {
      throw std::invalid_argument(
          "Local scale is too small to decompose safely.");
    }
    v[6 + col] = scale;
    for (std::size_t row = 0; row < 3; ++row) {
      rotation[row * 4 + col] = matrix[row * 4 + col] / scale;
    }
  }
  const auto &r = rotation;
  const double determinant = r[0] * (r[5] * r[10] - r[6] * r[9]) -
                             r[1] * (r[4] * r[10] - r[6] * r[8]) +
                             r[2] * (r[4] * r[9] - r[5] * r[8]);
  if (determinant < 0) {
    v[6] = -v[6];
    for (std::size_t row = 0; row < 3; ++row) {
      rotation[row * 4] = -rotation[row * 4];
    }
  }
  for (std::size_t a = 0; a < 3; ++a) {
    for (std::size_t b = a + 1; b < 3; ++b) {
      double dot = 0;
      for (std::size_t row = 0; row < 3; ++row) {
        dot += rotation[row * 4 + a] * rotation[row * 4 + b];
      }
      if (std::abs(dot) > 1e-10) {
        throw std::invalid_argument(
            "Preserving world placement would require local shear.");
      }
    }
  }
  const double y = std::asin(std::clamp(rotation[2], -1.0, 1.0));
  const double x = std::abs(std::cos(y)) > 1e-8
                       ? std::atan2(-rotation[6], rotation[10])
                       : std::atan2(rotation[9], rotation[5]);
  const double z =
      std::abs(std::cos(y)) > 1e-8 ? std::atan2(-rotation[1], rotation[0]) : 0;
  v[3] = x * 180 / std::numbers::pi;
  v[4] = y * 180 / std::numbers::pi;
  v[5] = z * 180 / std::numbers::pi;
  if (auto issue = transform_error(result)) {
    throw std::invalid_argument(*issue);
  }
  if (!matrix_near(matrix, local_matrix(result))) {
    throw std::invalid_argument(
        "Local TRS recomposition exceeds preservation tolerance.");
  }
  return result;
}
inline void preserve_world(const Document &before, Document &after, EntityId id,
                           EntityId parent) {
  const auto old_world = world_matrix(before, id, false);
  const auto parent_world =
      parent ? world_matrix(after, parent, false) : identity_matrix();
  const auto inverse = inverse_matrix(parent_world);
  if (!matrix_near(identity_matrix(), multiply_matrix(parent_world, inverse))) {
    throw std::invalid_argument("Parent inverse residual exceeds tolerance.");
  }
  const auto local = decompose_local(multiply_matrix(inverse, old_world));
  if (local == Transform{}) {
    after.transforms.erase(id);
  } else {
    after.transforms.insert_or_assign(id, local);
  }
  for (const auto &[entity, name] : before.entities) {
    (void)name;
    auto ancestor = entity;
    while (ancestor != id && before.parents.contains(ancestor)) {
      ancestor = before.parents.at(ancestor);
    }
    if (ancestor == id && !matrix_near(world_matrix(before, entity, false),
                                       world_matrix(after, entity, false))) {
      throw std::invalid_argument(
          "Subtree world matrix exceeds preservation tolerance.");
    }
  }
}
} // namespace lyko
