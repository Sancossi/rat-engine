#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rat {

struct FileBytes {
  std::vector<std::uint8_t> data;

  [[nodiscard]] std::string_view as_text() const {
    if (data.empty()) {
      return {};
    }
    return {reinterpret_cast<const char*>(data.data()), data.size()};
  }
};

struct FileReadResult {
  bool ok = false;
  std::string error;
  FileBytes bytes;
};

struct FileWriteResult {
  bool ok = false;
  std::string error;
};

class FileStore {
 public:
  virtual ~FileStore() = default;
  [[nodiscard]] virtual FileReadResult read(std::string_view path) const = 0;
  virtual FileWriteResult write(std::string_view path, std::string_view bytes) = 0;
};

class MemoryFileStore final : public FileStore {
 public:
  [[nodiscard]] FileReadResult read(std::string_view path) const override;
  FileWriteResult write(std::string_view path, std::string_view bytes) override;

 private:
  std::unordered_map<std::string, std::string> files_;
};

class OsFileStore final : public FileStore {
 public:
  [[nodiscard]] FileReadResult read(std::string_view path) const override;
  FileWriteResult write(std::string_view path, std::string_view bytes) override;
};

[[nodiscard]] FileStore& os_files();

}  // namespace rat
