#pragma once
#include <array>
#include <cmath>
#include <optional>
#include <string>
namespace lyko {
struct Transform {
  // Local translation, intrinsic XYZ Euler degrees, signed scale.
  std::array<double, 9> values{0, 0, 0, 0, 0, 0, 1, 1, 1};
  bool operator==(const Transform &) const = default;
};
inline constexpr std::array<const char *, 9> transform_fields{
    "px", "py", "pz", "rx", "ry", "rz", "sx", "sy", "sz"};
inline std::optional<std::string> transform_error(const Transform &transform) {
  for (std::size_t i = 0; i < 9; ++i) {
    if (!std::isfinite(transform.values[i])) {
      return "Transform values must be finite.";
    }
    if (i >= 6 && transform.values[i] == 0) {
      return "Scale components must be nonzero.";
    }
  }
  return {};
}
} // namespace lyko
