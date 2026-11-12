#pragma once
#include "command_bus.hpp"
#include "json.hpp"
namespace lyko {
struct DocumentObservation {
  std::string kind, path;
  Json value;
  std::string sha256 = {};
};
// External persistence observations are not undone when authoring history
// moves.
class DocumentTracking {
public:
  DocumentTracking();
  void observe(const DocumentObservation &observation);
  Json status(const Document &document) const;
  std::optional<std::string> fingerprint(const std::string &kind,
                                         const std::string &path) const;
  std::optional<Json> clean_project(const std::string &path) const;

private:
  struct Record {
    Json clean;
    std::string path, sha256;
  };
  std::map<std::string, Json> initial_;
  std::map<std::string, Record> records_;
};
std::map<std::string, Json> persisted_documents(const Document &document);
std::string observation_path(const std::string &path);
std::optional<DocumentObservation> loaded_document(const std::string &command,
                                                   const Json &args,
                                                   const Mutation &mutation);
} // namespace lyko
