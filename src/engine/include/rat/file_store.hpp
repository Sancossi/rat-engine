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

// Low-level transaction operations. Implementations own at most one exclusive,
// same-directory temporary file. Used by production and failure-injection tests.
class AtomicFileOps {
 public:
  virtual ~AtomicFileOps() = default;
  virtual FileWriteResult create_temp(std::string_view target) = 0;
  virtual FileWriteResult write_temp(std::string_view bytes) = 0;
  virtual FileWriteResult finish_temp() = 0;  // checked flush AND close
  virtual FileWriteResult backup_existing(std::string_view target) = 0;
  virtual FileWriteResult replace_target(std::string_view target) = 0;
  virtual void cleanup_temp() noexcept = 0;
};

[[nodiscard]] FileWriteResult atomic_write(AtomicFileOps& ops, std::string_view path,
                                           std::string_view bytes);

class FileStore {
 public:
  virtual ~FileStore() = default;
  [[nodiscard]] virtual FileReadResult read(std::string_view path) const = 0;
  virtual FileWriteResult write(std::string_view path, std::string_view bytes) = 0;
  virtual FileWriteResult write_atomic(std::string_view path, std::string_view bytes) = 0;
};

class MemoryFileStore final : public FileStore {
 public:
  [[nodiscard]] FileReadResult read(std::string_view path) const override;
  FileWriteResult write(std::string_view path, std::string_view bytes) override;
  FileWriteResult write_atomic(std::string_view path, std::string_view bytes) override;

 private:
  std::unordered_map<std::string, std::string> files_;
};

class OsFileStore final : public FileStore {
 public:
  [[nodiscard]] FileReadResult read(std::string_view path) const override;
  FileWriteResult write(std::string_view path, std::string_view bytes) override;
  FileWriteResult write_atomic(std::string_view path, std::string_view bytes) override;
};

[[nodiscard]] FileStore& os_files();

}  // namespace rat
