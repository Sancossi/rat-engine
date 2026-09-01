#include "rat/file_store.hpp"

#include <fstream>
#include <sstream>

namespace rat {
namespace {

[[nodiscard]] FileBytes bytes_from_text(std::string_view text) {
  FileBytes bytes;
  bytes.data.assign(reinterpret_cast<const std::uint8_t*>(text.data()),
                    reinterpret_cast<const std::uint8_t*>(text.data()) + text.size());
  return bytes;
}

}  // namespace

FileReadResult MemoryFileStore::read(std::string_view path) const {
  FileReadResult result;
  const auto it = files_.find(std::string(path));
  if (it == files_.end()) {
    result.error = "file not found: " + std::string(path);
    return result;
  }
  result.ok = true;
  result.bytes = bytes_from_text(it->second);
  return result;
}

FileWriteResult MemoryFileStore::write(std::string_view path, std::string_view bytes) {
  files_[std::string(path)] = std::string(bytes);
  return FileWriteResult{true, {}};
}

FileReadResult OsFileStore::read(std::string_view path) const {
  FileReadResult result;
  const std::string path_str(path);
  std::ifstream in(path_str, std::ios::binary);
  if (!in) {
    result.error = "failed to open file: " + path_str;
    return result;
  }
  std::ostringstream oss;
  oss << in.rdbuf();
  if (!in && !in.eof()) {
    result.error = "failed to read file: " + path_str;
    return result;
  }
  result.ok = true;
  result.bytes = bytes_from_text(oss.str());
  return result;
}

FileWriteResult OsFileStore::write(std::string_view path, std::string_view bytes) {
  FileWriteResult result;
  const std::string path_str(path);
  std::ofstream out(path_str, std::ios::binary | std::ios::trunc);
  if (!out) {
    result.error = "failed to open file for writing: " + path_str;
    return result;
  }
  out.write(bytes.data(), static_cast<std::streamsize>(bytes.size()));
  out.flush();
  if (!out) {
    result.error = "failed to write file: " + path_str;
    return result;
  }
  result.ok = true;
  return result;
}

FileStore& os_files() {
  static OsFileStore store;
  return store;
}

}  // namespace rat
