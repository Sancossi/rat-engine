#include "editor_launch_options.hpp"
#include <rat/log.hpp>
#include <rat/file_store.hpp>
#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <vector>

namespace {
std::string utf8(const std::filesystem::path& path) {
  const auto text = path.generic_u8string(); return {text.begin(), text.end()};
}
rat::EditorLaunchContext context() {
  const auto root = std::filesystem::temp_directory_path();
  return {utf8(root / "launch"), utf8(root / "package/rat-editor"), utf8(root / "user-data"), {}};
}
}

TEST_CASE("Launch defaults separate packaged data from user writes", "[unit][launch]") {
  const auto resolved = rat::resolve_editor_launch_options({}, context());
  REQUIRE(resolved.ok);
  const auto root = std::filesystem::temp_directory_path();
  CHECK(resolved.options.data_root == utf8(root / "package/data"));
  CHECK(resolved.options.map_path == utf8(root / "package/data/maps/grey_yard.json"));
  CHECK(resolved.options.save_slot_path == utf8(root / "user-data/saves/slot1.ratsave"));
  CHECK(resolved.options.log_path == utf8(root / "user-data/logs/rat.log"));
  CHECK(resolved.options.debug_snapshot_path == utf8(root / "user-data/debug/rat-debug.json"));
  CHECK(resolved.options.imgui_ini_path == utf8(root / "user-data/imgui.ini"));
}

TEST_CASE("Launch CLI resolves relative Unicode paths once against captured launch directory", "[unit][launch]") {
  const std::vector<std::string> args = {"--data-root", "../data folder", "--user-data-dir", "user/../user folder",
                                       "--map", "карты/двор.json"};
  const auto parsed = rat::parse_editor_launch_arguments(args); REQUIRE(parsed.ok);
  auto captured = context(); captured.log_override = "logs/сессия.log";
  const auto resolved = rat::resolve_editor_launch_options(parsed.arguments, captured); REQUIRE(resolved.ok);
  const auto root = std::filesystem::temp_directory_path();
  CHECK(resolved.options.data_root == utf8(root / "data folder"));
  CHECK(resolved.options.user_data_dir == utf8(root / "launch/user folder"));
  CHECK(resolved.options.map_path == utf8(root / u8"launch/карты/двор.json"));
  CHECK(resolved.options.log_path == utf8(root / u8"launch/logs/сессия.log"));
  captured.launch_directory = utf8(root / "different");
  CHECK(resolved.options.map_path == utf8(root / u8"launch/карты/двор.json"));
}

TEST_CASE("Launch accepts absolute overrides and rejects incomplete or unknown options", "[unit][launch]") {
  auto ctx = context();
  const auto root = std::filesystem::temp_directory_path();
  rat::EditorLaunchArguments args;
  args.data_root = utf8(root / "assets"); args.user_data_dir = utf8(root / "profile");
  args.map = utf8(root / "custom.json");
  const auto resolved = rat::resolve_editor_launch_options(args, ctx); REQUIRE(resolved.ok);
  CHECK(resolved.options.map_path == *args.map);
  CHECK(resolved.options.data_root == *args.data_root);
  for (const auto& invalid : std::vector<std::vector<std::string>>{
      {"--map"}, {"--unknown"}, {"--data-root", ""}, {"--map", "--help"},
      {"--map", "first", "--map", "second"}, {"map.json"}}) {
    const auto parsed = rat::parse_editor_launch_arguments(invalid);
    CHECK_FALSE(parsed.ok); CHECK_FALSE(parsed.error.empty());
  }
  const std::vector<std::string> help{"--help"};
  const auto parsed = rat::parse_editor_launch_arguments(help); CHECK(parsed.ok); CHECK(parsed.help);
  ctx.launch_directory = "relative";
  CHECK_FALSE(rat::resolve_editor_launch_options(args, ctx).ok);
}

TEST_CASE("FileLogSink writes UTF8 paths consistently with FileStore", "[unit][launch][log]") {
  const auto path = std::filesystem::temp_directory_path() / u8"rat-log-юникод.txt";
  const auto name = utf8(path);
  {
    rat::FileLogSink sink(name); REQUIRE(sink.ok());
    sink.write(rat::LogLevel::Info, "launch", "Unicode path works");
  }
  const auto read = rat::os_files().read(name); REQUIRE(read.ok);
  CHECK(read.bytes.as_text().find("Unicode path works") != std::string_view::npos);
  std::filesystem::remove(path);
}

TEST_CASE("Explicit user-data override does not require a platform default", "[unit][launch]") {
  auto ctx = context(); ctx.default_user_data_dir.clear();
  const std::vector<std::string> args{"--user-data-dir", "isolated"};
  const auto parsed = rat::parse_editor_launch_arguments(args); REQUIRE(parsed.ok);
  const auto result = rat::resolve_editor_launch_options(parsed.arguments, ctx);
  REQUIRE(result.ok);
  CHECK(result.options.user_data_dir == utf8(std::filesystem::temp_directory_path() / "launch/isolated"));
  CHECK_FALSE(rat::resolve_editor_launch_options({}, ctx).ok);
}
