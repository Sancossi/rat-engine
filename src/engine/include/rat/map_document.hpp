#pragma once

#include "rat/file_store.hpp"
#include "rat/map_data.hpp"

#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

enum class MapIssueSeverity {
  Error,
  Warning,
};

struct MapIssue {
  MapIssueSeverity severity = MapIssueSeverity::Error;
  std::string json_path;
  std::string message;
};

class MapDocument {
 public:
  MapDocument() = default;
  explicit MapDocument(MapData data);

  [[nodiscard]] const MapData& data() const { return data_; }
  [[nodiscard]] std::uint64_t revision() const { return revision_; }

  void replace(MapData data);

 private:
  MapData data_{};
  std::uint64_t revision_ = 0;
};

struct RuntimeMap {
  MapData data;
  std::uint64_t source_revision = 0;
};

struct MapCompileResult {
  bool ok = false;
  RuntimeMap runtime;
  std::vector<MapIssue> issues;
};

struct MapDocumentLoadResult {
  bool ok = false;
  MapDocument document;
  std::vector<MapIssue> issues;
};

[[nodiscard]] bool map_issues_have_errors(const std::vector<MapIssue>& issues);
[[nodiscard]] std::string format_map_issues(const std::vector<MapIssue>& issues);

// 64 MiB of float heights; checked before allocation. This is a file-format
// resource limit, shared by loaders and programmatic map compilation.
inline constexpr std::uint64_t kMaxMapGridCells = 16u * 1024u * 1024u;
[[nodiscard]] bool safe_map_grid(int width, int height, int origin_x = 0, int origin_z = 0);
// Numeric/structural safety only: intentionally permits unfinished event graphs.
[[nodiscard]] std::vector<MapIssue> validate_map_structure(const MapData& data);
[[nodiscard]] std::vector<MapIssue> validate_map_document(const MapData& data);
[[nodiscard]] MapCompileResult compile_map_data(const MapData& data);
[[nodiscard]] MapCompileResult compile_map_document(const MapDocument& document);

[[nodiscard]] MapDocumentLoadResult load_map_document_from_string(std::string_view json_text);
[[nodiscard]] MapDocumentLoadResult load_map_document_from_file(const std::string& path);
[[nodiscard]] MapDocumentLoadResult load_map_document_from_file(const std::string& path,
                                                                const FileStore& files);

}  // namespace rat
