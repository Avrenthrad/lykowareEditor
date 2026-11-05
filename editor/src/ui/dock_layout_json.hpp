#pragma once
#include "dock_layout.hpp"
#include "json.hpp"
namespace lyko {
inline Json::Object dock_layout_constraints() {
  using O = Json::Object;
  using A = Json::Array;
  const Json id = O{{"type", "string"},
                    {"minLength", std::uint64_t{1}},
                    {"maxLength", std::uint64_t{64}},
                    {"pattern", "^[A-Za-z0-9_-]+$"}};
  const Json panel =
      O{{"type", "string"},
        {"enum", A{"scene", "inspector", "viewport", "console", "history"}}};
  const Json panels = O{{"type", "array"},
                        {"items", panel},
                        {"uniqueItems", true},
                        {"maxItems", std::uint64_t{5}}};
  const Json tabs =
      O{{"type", "object"},
        {"additionalProperties", false},
        {"required", A{"kind", "panels", "active"}},
        {"properties",
         O{{"kind", O{{"const", "tabs"}}},
           {"panels", panels},
           {"active", O{{"type", "string"},
                        {"enum", A{"", "scene", "inspector", "viewport",
                                   "console", "history"}}}}}}};
  const Json split = O{
      {"type", "object"},
      {"additionalProperties", false},
      {"required", A{"kind", "axis", "ratio", "first", "second"}},
      {"properties",
       O{{"kind", O{{"const", "split"}}},
         {"axis", O{{"type", "string"}, {"enum", A{"horizontal", "vertical"}}}},
         {"ratio", O{{"type", "integer"},
                     {"minimum", std::uint64_t{500}},
                     {"maximum", std::uint64_t{9500}}}},
         {"first", id},
         {"second", id}}}};
  return O{{"type", "object"},
           {"required", A{"root", "nodes", "hidden"}},
           {"additionalProperties", false},
           {"properties", O{{"root", id},
                            {"hidden", panels},
                            {"nodes", O{{"type", "object"},
                                        {"minProperties", std::uint64_t{1}},
                                        {"maxProperties", std::uint64_t{63}},
                                        {"propertyNames", id},
                                        {"additionalProperties",
                                         O{{"oneOf", A{tabs, split}}}}}}}}};
}
inline Json dock_layout_json(const DockLayout &layout) {
  using O = Json::Object;
  using A = Json::Array;
  O nodes;
  A hidden;
  for (const auto &panel : layout.hidden) {
    hidden.emplace_back(panel);
  }
  for (const auto &[id, node] : layout.nodes) {
    if (node.axis.empty()) {
      A panels;
      for (const auto &panel : node.panels) {
        panels.emplace_back(panel);
      }
      nodes.emplace(id, O{{"kind", "tabs"},
                          {"panels", std::move(panels)},
                          {"active", node.active}});
    } else {
      nodes.emplace(id, O{{"kind", "split"},
                          {"axis", node.axis},
                          {"ratio", node.ratio},
                          {"first", node.first},
                          {"second", node.second}});
    }
  }
  return O{{"root", layout.root},
           {"hidden", std::move(hidden)},
           {"nodes", std::move(nodes)}};
}
inline DockLayout parse_dock_layout(const Json &json) {
  using O = Json::Object;
  const auto exact = [](const Json &value,
                        std::initializer_list<std::string> keys) {
    if (value.as<O>().size() != keys.size()) {
      throw std::invalid_argument("Unexpected layout fields");
    }
    for (const auto &key : keys) {
      (void)value.at(key);
    }
  };
  exact(json, {"root", "hidden", "nodes"});
  if (json.at("nodes").as<O>().empty() ||
      json.at("nodes").as<O>().size() > 63 ||
      json.at("hidden").as<Json::Array>().size() > 5) {
    throw std::invalid_argument(
        "Dock layout exceeds node or hidden-panel limits");
  }
  DockLayout layout;
  layout.root = json.at("root").as<std::string>();
  for (const auto &panel : json.at("hidden").as<Json::Array>()) {
    layout.hidden.push_back(panel.as<std::string>());
  }
  for (const auto &[id, value] : json.at("nodes").as<O>()) {
    DockNode node;
    const auto &kind = value.at("kind").as<std::string>();
    if (kind == "tabs") {
      exact(value, {"kind", "panels", "active"});
      if (value.at("panels").as<Json::Array>().size() > 5) {
        throw std::invalid_argument("Too many panels in tab group");
      }
      for (const auto &panel : value.at("panels").as<Json::Array>()) {
        node.panels.push_back(panel.as<std::string>());
      }
      node.active = value.at("active").as<std::string>();
    } else if (kind == "split") {
      exact(value, {"kind", "axis", "ratio", "first", "second"});
      node.axis = value.at("axis").as<std::string>();
      node.ratio = value.at("ratio").as<std::uint64_t>();
      node.first = value.at("first").as<std::string>();
      node.second = value.at("second").as<std::string>();
    } else {
      throw std::invalid_argument("Unknown layout node kind");
    }
    layout.nodes.emplace(id, std::move(node));
  }
  if (auto error = dock_layout_error(layout)) {
    throw std::invalid_argument(*error);
  }
  return canonical_dock_layout(layout);
}
} // namespace lyko
