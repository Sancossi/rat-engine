#pragma once

#include <cassert>
#include <iosfwd>
#include <memory>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

enum class LogLevel {
  Debug,
  Info,
  Warn,
  Error,
};

[[nodiscard]] const char* log_level_name(LogLevel level);
[[nodiscard]] std::string default_log_path();
[[nodiscard]] std::string format_log_line(LogLevel level, std::string_view channel,
                                          std::string_view message);

class LogSink {
 public:
  virtual ~LogSink() = default;
  virtual void write(LogLevel level, std::string_view channel, std::string_view message) = 0;
};

struct LogLine {
  LogLevel level = LogLevel::Info;
  std::string channel;
  std::string message;
};

class MemoryLogSink : public LogSink {
 public:
  void write(LogLevel level, std::string_view channel, std::string_view message) override;
  [[nodiscard]] const std::vector<LogLine>& lines() const { return lines_; }

 private:
  std::vector<LogLine> lines_;
};

class StreamLogSink : public LogSink {
 public:
  explicit StreamLogSink(std::ostream& out);
  void write(LogLevel level, std::string_view channel, std::string_view message) override;

 private:
  std::ostream* out_ = nullptr;
};

class FileLogSink : public LogSink {
 public:
  explicit FileLogSink(std::string path);
  ~FileLogSink() override;

  FileLogSink(const FileLogSink&) = delete;
  FileLogSink& operator=(const FileLogSink&) = delete;

  void write(LogLevel level, std::string_view channel, std::string_view message) override;
  [[nodiscard]] const std::string& path() const { return path_; }
  [[nodiscard]] bool ok() const;

 private:
  std::string path_;
  std::unique_ptr<std::ofstream> file_;
};

class TeeLogSink : public LogSink {
 public:
  TeeLogSink(LogSink& first, LogSink& second);
  void write(LogLevel level, std::string_view channel, std::string_view message) override;

 private:
  LogSink* first_ = nullptr;
  LogSink* second_ = nullptr;
};

class Logger {
 public:
  explicit Logger(LogSink& sink);
  void log(LogLevel level, std::string_view channel, std::string_view message);

 private:
  LogSink* sink_ = nullptr;
};

void log(Logger& logger, LogLevel level, std::string_view channel, std::string_view message);

// Returns true if condition holds. On failure, logs Error and returns false.
// Debug assert is the caller's RAT_ASSERT; this stays callable from tests.
[[nodiscard]] bool check(Logger& logger, bool condition, std::string_view channel,
                         std::string_view message);

#define RAT_ASSERT(logger, condition, channel, message)                                          \
  do {                                                                                           \
    if (!::rat::check((logger), static_cast<bool>(condition), (channel), (message))) {           \
      assert(false);                                                                             \
    }                                                                                            \
  } while (0)

}  // namespace rat
