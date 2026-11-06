#pragma once
#include "assets.hpp"
#include <stdexcept>
namespace lyko {
class AssetSourceError : public std::runtime_error {
public:
  AssetSourceError(std::string field, std::string message)
      : std::runtime_error(std::move(message)), field_(std::move(field)) {}
  const std::string &field() const { return field_; }

private:
  std::string field_;
};
Json validate_asset_source(std::uint64_t id, const Asset &asset,
                           const std::filesystem::path &root);
} // namespace lyko
