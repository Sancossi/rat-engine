#include "rat/log.hpp"

#include <cstdlib>
#include <fstream>
#include <filesystem>
#include <memory>
#include <ostream>
#include <utility>

namespace rat {

const char* log_level_name(LogLevel level) {
  switch (level) {
    case LogLevel::Debug:
      return "DEBUG";
    case LogLevel::Info:
      return "INFO";
    case LogLevel::Warn:
      return "WARN";
    case LogLevel::Error:
      return "ERROR";
  }
  return "INFO";
}

std::string default_log_path() {
#if defined(_MSC_VER)
#pragma warning(push)
#pragma warning(disable : 4996)
#endif
  const char* override_path = std::getenv("RAT_LOG_PATH");
#if defined(_MSC_VER)
#pragma warning(pop)
#endif
  if (override_path != nullptr && override_path[0] != '\0') {
    return override_path;
  }
  return "rat.log";
}

std::string format_log_line(LogLevel level, std::string_view channel, std::string_view message) {
  std::string line;
  line += '[';
  line += log_level_name(level);
  line += "] ";
  line.append(channel.data(), channel.size());
  line += ": ";
  line.append(message.data(), message.size());
  return line;
}

void MemoryLogSink::write(LogLevel level, std::string_view channel, std::string_view message) {
  lines_.push_back(LogLine{level, std::string(channel), std::string(message)});
}

StreamLogSink::StreamLogSink(std::ostream& out) : out_(&out) {}

void StreamLogSink::write(LogLevel level, std::string_view channel, std::string_view message) {
  if (out_ == nullptr) {
    return;
  }
  *out_ << format_log_line(level, channel, message) << '\n';
}

FileLogSink::FileLogSink(std::string path)
    : path_(std::move(path)), file_(std::make_unique<std::ofstream>(std::filesystem::path(std::u8string(path_.begin(), path_.end())), std::ios::out | std::ios::trunc)) {}

FileLogSink::~FileLogSink() = default;

bool FileLogSink::ok() const {
  return file_ != nullptr && file_->is_open() && file_->good();
}

void FileLogSink::write(LogLevel level, std::string_view channel, std::string_view message) {
  if (!ok()) {
    return;
  }
  *file_ << format_log_line(level, channel, message) << '\n';
  file_->flush();
}

TeeLogSink::TeeLogSink(LogSink& first, LogSink& second) : first_(&first), second_(&second) {}

void TeeLogSink::write(LogLevel level, std::string_view channel, std::string_view message) {
  if (first_ != nullptr) {
    first_->write(level, channel, message);
  }
  if (second_ != nullptr) {
    second_->write(level, channel, message);
  }
}

Logger::Logger(LogSink& sink) : sink_(&sink) {}

void Logger::log(LogLevel level, std::string_view channel, std::string_view message) {
  if (sink_ != nullptr) {
    sink_->write(level, channel, message);
  }
}

void log(Logger& logger, LogLevel level, std::string_view channel, std::string_view message) {
  logger.log(level, channel, message);
}

bool check(Logger& logger, bool condition, std::string_view channel, std::string_view message) {
  if (condition) {
    return true;
  }
  logger.log(LogLevel::Error, channel, message);
  return false;
}

}  // namespace rat
