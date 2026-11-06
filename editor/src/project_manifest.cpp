#include "project_manifest.hpp"
#include "text_file.hpp"
#include <set>
#include <sstream>
namespace lyko {
namespace {
std::string trim(const std::string &text) {
  const auto first = text.find_first_not_of(" \t\r");
  return first == std::string::npos
             ? ""
             : text.substr(first, text.find_last_not_of(" \t\r") - first + 1);
}
std::size_t outside(const std::string &text, char delimiter) {
  char quote = 0;
  bool escaped = false;
  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto c = text[i];
    if (escaped) {
      escaped = false;
      continue;
    }
    if (quote == '"' && c == '\\') {
      escaped = true;
      continue;
    }
    if (quote && c == quote) {
      quote = 0;
      continue;
    }
    if (!quote && (c == '"' || c == '\'')) {
      quote = c;
      continue;
    }
    if (!quote && c == delimiter) {
      return i;
    }
  }
  return std::string::npos;
}
std::string string_value(const std::string &value, const std::string &field) {
  const auto fail = [&]() -> void {
    throw ProjectManifestError(field,
                               "Use a supported single-line TOML string.");
  };
  if (value.size() < 2 || (value.front() != '"' && value.front() != '\'') ||
      value.back() != value.front()) {
    fail();
  }
  std::string result;
  const auto end = value.size() - 1;
  for (std::size_t i = 1; i < end; ++i) {
    const auto c = static_cast<unsigned char>(value[i]);
    if ((c < 32 && c != '\t') || c == 127 ||
        c == static_cast<unsigned char>(value.front())) {
      fail();
    }
    if (value.front() == '\'' || c != '\\') {
      result += static_cast<char>(c);
      continue;
    }
    if (++i >= end) {
      fail();
    }
    switch (value[i]) {
    case 'b':
      result += '\b';
      break;
    case 't':
      result += '\t';
      break;
    case 'n':
      result += '\n';
      break;
    case 'f':
      result += '\f';
      break;
    case 'r':
      result += '\r';
      break;
    case '"':
      result += '"';
      break;
    case '\\':
      result += '\\';
      break;
    case 'u':
    case 'U': {
      const auto digits = value[i] == 'u' ? 4U : 8U;
      std::uint32_t cp = 0;
      for (unsigned n = 0; n < digits; ++n) {
        if (++i >= end) {
          fail();
        }
        const auto hex = value[i];
        cp *= 16;
        if (hex >= '0' && hex <= '9') {
          cp += hex - '0';
        } else if (hex >= 'a' && hex <= 'f') {
          cp += hex - 'a' + 10;
        } else if (hex >= 'A' && hex <= 'F') {
          cp += hex - 'A' + 10;
        } else {
          fail();
        }
      }
      if (cp > 0x10ffff || (cp >= 0xd800 && cp <= 0xdfff)) {
        fail();
      }
      json_detail::utf8(result, cp);
      break;
    }
    default:
      fail();
    }
  }
  return result;
}
bool nested(const std::string &a, const std::string &b) {
  return a == b ||
         (a.size() > b.size() && a.starts_with(b) && a[b.size()] == '/');
}
} // namespace
void validate_project_manifest(const ProjectManifest &manifest) {
  if (manifest.project_id.size() != 32 ||
      !std::all_of(manifest.project_id.begin(), manifest.project_id.end(),
                   [](char c) {
                     return (c >= '0' && c <= '9') || (c >= 'a' && c <= 'f');
                   })) {
    throw ProjectManifestError(
        "project_id",
        "Use 32 lowercase hexadecimal project identity characters.");
  }
  if (manifest.name.empty() || manifest.name.size() > 128 ||
      !json_detail::valid_utf8(manifest.name) ||
      std::any_of(manifest.name.begin(), manifest.name.end(),
                  [](unsigned char c) { return c < 32 || c == 127; })) {
    throw ProjectManifestError(
        "name", "Name needs 1 to 128 UTF-8 bytes without ASCII controls.");
  }
  const ProjectManifest defaults;
  if (manifest.directories.size() != defaults.directories.size()) {
    throw ProjectManifestError("directories",
                               "Supply the five supported directory roles.");
  }
  for (const auto &[key, path] : manifest.directories) {
    if (!defaults.directories.contains(key)) {
      throw ProjectManifestError("directories",
                                 "Unknown project directory role.");
    }
    if (auto issue = asset_path_error(path)) {
      throw ProjectManifestError("directories." + key, *issue);
    }
  }
  const auto cache = asset_fold(manifest.directories.at("cache"));
  for (const auto &[key, path] : manifest.directories) {
    const auto source = asset_fold(path);
    if (key != "cache" && (nested(cache, source) || nested(source, cache))) {
      throw ProjectManifestError(
          "directories.cache",
          "Cache and source directories must not overlap.");
    }
  }
}
ProjectManifest parse_project_manifest(const std::string &text) {
  if (text.size() > 1024 * 1024 || !json_detail::valid_utf8(text)) {
    throw ProjectManifestError("manifest",
                               "Manifest must be UTF-8 and at most 1 MiB.");
  }
  for (std::size_t i = 0; i < text.size(); ++i) {
    const auto c = static_cast<unsigned char>(text[i]);
    if ((c < 32 && c != '\t' && c != '\n' && c != '\r') || c == 127 ||
        (c == '\r' && (i + 1 == text.size() || text[i + 1] != '\n'))) {
      throw ProjectManifestError(
          "manifest",
          "Manifest contains invalid TOML control characters or line endings.");
    }
  }
  ProjectManifest manifest;
  std::set<std::string> seen;
  bool directories = false;
  std::istringstream input(text);
  for (std::string line; std::getline(input, line);) {
    const auto comment = outside(line, '#');
    if (comment != std::string::npos) {
      line.resize(comment);
    }
    line = trim(line);
    if (line.empty()) {
      continue;
    }
    if (line == "[directories]" && !directories) {
      directories = true;
      continue;
    }
    const auto equal = outside(line, '=');
    if (equal == std::string::npos) {
      throw ProjectManifestError(
          "manifest", "Expected supported TOML key = value or [directories].");
    }
    const auto key = trim(line.substr(0, equal));
    const auto value = trim(line.substr(equal + 1));
    const auto field = directories ? "directories." + key : key;
    if (!seen.insert(field).second) {
      throw ProjectManifestError(field, "Duplicate project manifest key.");
    }
    if (directories) {
      if (!manifest.directories.contains(key)) {
        throw ProjectManifestError(field, "Unknown project directory role.");
      }
      manifest.directories.at(key) = string_value(value, field);
    } else if (key == "schema_version") {
      if (value != "1") {
        throw ProjectManifestError(
            key, "Supported project schema_version is decimal 1.");
      }
    } else if (key == "project_id") {
      manifest.project_id = string_value(value, field);
    } else if (key == "name") {
      manifest.name = string_value(value, field);
    } else {
      throw ProjectManifestError(field, "Unknown project manifest key.");
    }
  }
  for (const auto *key : {"schema_version", "project_id", "name"}) {
    if (!seen.contains(key)) {
      throw ProjectManifestError(key, "Missing required project manifest key.");
    }
  }
  validate_project_manifest(manifest);
  return manifest;
}
std::string project_manifest_toml(const ProjectManifest &manifest) {
  validate_project_manifest(manifest);
  std::string text =
      "# Lykoware project configuration v1\nschema_version = 1\nproject_id = " +
      Json(manifest.project_id).dump() +
      "\nname = " + Json(manifest.name).dump() + "\n\n[directories]\n";
  for (const auto &[key, path] : manifest.directories) {
    text += key + " = " + Json(path).dump() + '\n';
  }
  return text;
}
Json project_manifest_json(const ProjectManifest &manifest) {
  validate_project_manifest(manifest);
  Json::Object directories;
  for (const auto &[key, path] : manifest.directories) {
    directories.emplace(key, path);
  }
  return Json::Object{{"schema_version", std::uint64_t{1}},
                      {"project_id", manifest.project_id},
                      {"name", manifest.name},
                      {"directories", std::move(directories)}};
}
ProjectManifest load_project_manifest(const std::filesystem::path &path) {
  try {
    if (!std::filesystem::is_regular_file(path)) {
      throw ProjectManifestError("path", "Manifest must be a regular file.");
    }
    return parse_project_manifest(read_text_file(path));
  } catch (const ProjectManifestError &) {
    throw;
  } catch (const std::exception &error) {
    throw ProjectManifestError("path", error.what());
  }
}
void save_project_manifest(const std::filesystem::path &path,
                           const ProjectManifest &manifest) {
  const auto text = project_manifest_toml(manifest);
  try {
    publish_text_file(path, text);
  } catch (const std::exception &error) {
    throw ProjectManifestError("path", error.what());
  }
}
} // namespace lyko
