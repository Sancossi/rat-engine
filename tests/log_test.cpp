#include <rat/log.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdlib>
#include <filesystem>
#include <fstream>
#include <string>
#include <system_error>

TEST_CASE("Memory sink records rat::log level, channel, and message", "[unit][log]") {
  rat::MemoryLogSink sink;
  rat::Logger logger(sink);

  rat::log(logger, rat::LogLevel::Info, "core", "hello");

  REQUIRE(sink.lines().size() == 1);
  CHECK(sink.lines()[0].level == rat::LogLevel::Info);
  CHECK(sink.lines()[0].channel == "core");
  CHECK(sink.lines()[0].message == "hello");
}

TEST_CASE("File sink writes formatted line to override path", "[unit][log]") {
  const auto path =
      std::filesystem::temp_directory_path() / "rat-log-test.log";
  std::error_code ec;
  std::filesystem::remove(path, ec);

  {
    rat::FileLogSink sink(path.string());
    REQUIRE(sink.ok());
    rat::Logger logger(sink);
    rat::log(logger, rat::LogLevel::Warn, "events", "parallel limit");
  }

  {
    std::ifstream in(path);
    REQUIRE(in.good());
    std::string line;
    REQUIRE(std::getline(in, line));
    CHECK(line.find("WARN") != std::string::npos);
    CHECK(line.find("events") != std::string::npos);
    CHECK(line.find("parallel limit") != std::string::npos);
  }

  std::filesystem::remove(path, ec);
}

TEST_CASE("check logs Error on failure path and returns false", "[unit][log]") {
  rat::MemoryLogSink sink;
  rat::Logger logger(sink);

  CHECK(rat::check(logger, true, "core", "should not log"));
  REQUIRE(sink.lines().empty());

  CHECK_FALSE(rat::check(logger, false, "core", "assert failed"));
  REQUIRE(sink.lines().size() == 1);
  CHECK(sink.lines()[0].level == rat::LogLevel::Error);
  CHECK(sink.lines()[0].channel == "core");
  CHECK(sink.lines()[0].message == "assert failed");
}

TEST_CASE("default_log_path uses working dir unless RAT_LOG_PATH is set", "[unit][log]") {
#ifdef _WIN32
  _putenv_s("RAT_LOG_PATH", "");
#else
  unsetenv("RAT_LOG_PATH");
#endif
  CHECK(rat::default_log_path() == "rat.log");

#ifdef _WIN32
  _putenv_s("RAT_LOG_PATH", "C:/tmp/custom.log");
  CHECK(rat::default_log_path() == "C:/tmp/custom.log");
  _putenv_s("RAT_LOG_PATH", "");
#else
  setenv("RAT_LOG_PATH", "/tmp/custom.log", 1);
  CHECK(rat::default_log_path() == "/tmp/custom.log");
  unsetenv("RAT_LOG_PATH");
#endif
}
