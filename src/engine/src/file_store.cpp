#include "rat/file_store.hpp"

#include <fstream>
#include <sstream>
#include <atomic>
#include <algorithm>
#include <cstdio>
#include <filesystem>
#include <limits>
#include <system_error>

#ifdef _WIN32
#ifndef WIN32_LEAN_AND_MEAN
#define WIN32_LEAN_AND_MEAN
#endif
#ifndef NOMINMAX
#define NOMINMAX
#endif
#include <windows.h>
#else
#include <cerrno>
#include <fcntl.h>
#include <unistd.h>
#endif

namespace rat {
namespace {

std::filesystem::path utf8_path(std::string_view text) {
  return std::filesystem::path(std::u8string(text.begin(), text.end()));
}

class OsAtomicFileOps final : public AtomicFileOps {
 public:
  ~OsAtomicFileOps() override { cleanup_temp(); }

  FileWriteResult create_temp(std::string_view target) override {
    static std::atomic<unsigned long long> counter{0};
    const auto base = utf8_path(target);
    for (int attempt = 0; attempt < 128; ++attempt) {
#ifdef _WIN32
      const auto pid = GetCurrentProcessId();
#else
      const auto pid = getpid();
#endif
      temp_ = base;
      temp_ += ".tmp." + std::to_string(pid) + "." + std::to_string(counter.fetch_add(1));
#ifdef _WIN32
      handle_ = CreateFileW(temp_.c_str(), GENERIC_WRITE, 0, nullptr, CREATE_NEW,
                            FILE_ATTRIBUTE_NORMAL, nullptr);
      if (handle_ != INVALID_HANDLE_VALUE) return {true, {}};
      const auto error = GetLastError();
      if (error != ERROR_FILE_EXISTS && error != ERROR_ALREADY_EXISTS) break;
#else
      handle_ = ::open(temp_.c_str(), O_CREAT | O_EXCL | O_WRONLY, 0600);
      if (handle_ >= 0) return {true, {}};
      if (errno != EEXIST) break;
#endif
    }
    // We never owned this path: cleanup must not remove somebody else's file.
    temp_.clear();
    return {false, "failed to create exclusive temporary file"};
  }

  FileWriteResult write_temp(std::string_view bytes) override {
    while (!bytes.empty()) {
      const auto chunk = (std::min)(bytes.size(), std::size_t{1u << 30});
#ifdef _WIN32
      DWORD written = 0;
      if (!WriteFile(handle_, bytes.data(), static_cast<DWORD>(chunk), &written, nullptr) || !written)
        return {false, "failed to write temporary file"};
#else
      const auto written = ::write(handle_, bytes.data(), chunk);
      if (written < 0 && errno == EINTR) continue;
      if (written <= 0) return {false, "failed to write temporary file"};
#endif
      bytes.remove_prefix(static_cast<std::size_t>(written));
    }
    return {true, {}};
  }

  FileWriteResult finish_temp() override {
#ifdef _WIN32
    const bool flushed = FlushFileBuffers(handle_) != 0;
    const bool closed = CloseHandle(handle_) != 0;
    handle_ = INVALID_HANDLE_VALUE;
#else
    const bool flushed = ::fsync(handle_) == 0;
    const bool closed = ::close(handle_) == 0;
    handle_ = -1;
#endif
    return {flushed && closed, flushed && closed ? "" : "failed to flush/close temporary file"};
  }

  FileWriteResult backup_existing(std::string_view target) override {
    const auto main = utf8_path(target);
    std::error_code ec;
    const bool exists = std::filesystem::exists(main, ec);
    if (ec) return {false, "failed to inspect existing file: " + ec.message()};
    if (!exists) return {true, {}};
    // Backup itself uses a checked temporary write/replace, so a failed backup
    // does not destroy the previous .bak. It does not create a .bak.bak chain.
    std::ifstream in(main, std::ios::binary);
    if (!in) return {false, "failed to open previous file for backup"};
    OsAtomicFileOps backup;
    std::string backup_path(target);
    backup_path += ".bak";
    auto result = backup.create_temp(backup_path);
    if (!result.ok) return result;
    char buffer[65536];
    while (in) {
      in.read(buffer, sizeof(buffer));
      result = backup.write_temp({buffer, static_cast<std::size_t>(in.gcount())});
      if (!result.ok) return result;
    }
    if (!in.eof() || in.bad()) return {false, "failed to read previous file for backup"};
    result = backup.finish_temp();
    if (!result.ok) return result;
    return backup.replace_target(backup_path);
  }

  FileWriteResult replace_target(std::string_view target) override {
    const auto main = utf8_path(target);
#ifdef _WIN32
    // MOVEFILE_REPLACE_EXISTING replaces without a delete-before-rename gap.
    if (!MoveFileExW(temp_.c_str(), main.c_str(), MOVEFILE_REPLACE_EXISTING))
      return {false, "failed to replace target file (Windows error " + std::to_string(GetLastError()) + ")"};
#else
    if (::rename(temp_.c_str(), main.c_str()) != 0)
      return {false, "failed to replace target file"};
#endif
    temp_.clear();
    return {true, {}};
  }

  void cleanup_temp() noexcept override {
#ifdef _WIN32
    if (handle_ != INVALID_HANDLE_VALUE) CloseHandle(handle_);
    handle_ = INVALID_HANDLE_VALUE;
#else
    if (handle_ >= 0) ::close(handle_);
    handle_ = -1;
#endif
    if (!temp_.empty()) {
      std::error_code ec;
      std::filesystem::remove(temp_, ec);
      temp_.clear();
    }
  }

 private:
  std::filesystem::path temp_;
#ifdef _WIN32
  HANDLE handle_ = INVALID_HANDLE_VALUE;
#else
  int handle_ = -1;
#endif
};

[[nodiscard]] FileBytes bytes_from_text(std::string_view text) {
  FileBytes bytes;
  bytes.data.assign(reinterpret_cast<const std::uint8_t*>(text.data()),
                    reinterpret_cast<const std::uint8_t*>(text.data()) + text.size());
  return bytes;
}

}  // namespace

FileWriteResult atomic_write(AtomicFileOps& ops, std::string_view path, std::string_view bytes) {
  struct Cleanup { AtomicFileOps& ops; ~Cleanup() { ops.cleanup_temp(); } } cleanup{ops};
  if (path.empty() || path.find('\0') != std::string_view::npos)
    return {false, "invalid target path"};
  auto result = ops.create_temp(path);
  if (!result.ok) return result;
  result = ops.write_temp(bytes);
  if (!result.ok) return result;
  result = ops.finish_temp();
  if (!result.ok) return result;
  result = ops.backup_existing(path);
  if (!result.ok) return result;
  return ops.replace_target(path);
}

FileWriteResult MemoryFileStore::write_atomic(std::string_view path, std::string_view bytes) {
  if (path.empty() || path.find('\0') != std::string_view::npos) return {false, "invalid target path"};
  // Construct the transaction before publishing either entry.
  auto next = files_;
  const std::string key(path);
  if (const auto it = next.find(key); it != next.end()) next[key + ".bak"] = it->second;
  next[key] = std::string(bytes);
  files_.swap(next);
  return {true, {}};
}

FileWriteResult OsFileStore::write_atomic(std::string_view path, std::string_view bytes) {
  OsAtomicFileOps ops;
  return atomic_write(ops, path, bytes);
}

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
  std::ifstream in(utf8_path(path), std::ios::binary);
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
  std::ofstream out(utf8_path(path), std::ios::binary | std::ios::trunc);
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
