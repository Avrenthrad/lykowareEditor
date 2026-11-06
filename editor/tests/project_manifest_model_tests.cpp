#include "project_manifest.hpp"
#include <cstdlib>
#include <iostream>
void check(bool condition, const char *message) {
  if (!condition) {
    std::cerr << "FAIL: " << message << '\n';
    std::exit(1);
  }
}
int main() {
  lyko::ProjectManifest value;
  value.project_id = std::string(32, 'a');
  value.name = "Studio";
  check(lyko::parse_project_manifest(lyko::project_manifest_toml(value)) ==
            value,
        "typed manifest round trip");
  auto reject = [&](const lyko::ProjectManifest &invalid,
                    const std::string &field) {
    try {
      (void)lyko::project_manifest_toml(invalid);
    } catch (const lyko::ProjectManifestError &error) {
      check(error.field() == field, "typed error field");
      return;
    }
    check(false, "invalid typed model accepted");
  };
  auto invalid = value;
  invalid.name = std::string(1, static_cast<char>(0xff));
  reject(invalid, "name");
  invalid = value;
  invalid.directories.erase("scenes");
  reject(invalid, "directories");
  invalid = value;
  invalid.directories.at("cache") = "ASSETS/cache";
  reject(invalid, "directories.cache");
  invalid = value;
  invalid.directories.at("assets") = "../outside";
  reject(invalid, "directories.assets");
  value.directories.at("scenes") = "assets/scenes";
  check(lyko::parse_project_manifest(lyko::project_manifest_toml(value)) ==
            value,
        "source roles can nest");
  std::cout << "Passed typed project model validation and round trips\n";
}
