#pragma once

#include <optional>
#include <span>
#include <string>
#include <string_view>

namespace rat {

// All strings are UTF-8. Pure parsing/resolution does not access the environment
// or create files; the platform frontend supplies this context once at startup.
struct EditorLaunchArguments {
  std::optional<std::string> data_root;
  std::optional<std::string> user_data_dir;
  std::optional<std::string> map;
};
struct EditorLaunchContext {
  std::string launch_directory;
  std::string executable_path;
  std::string default_user_data_dir;
  std::optional<std::string> log_override;
};
struct EditorLaunchOptions {
  std::string data_root;
  std::string user_data_dir;
  std::string map_path;
  std::string save_slot_path;
  std::string log_path;
  std::string debug_snapshot_path;
  std::string imgui_ini_path;
};
struct EditorLaunchParseResult {
  bool ok = false;
  bool help = false;
  std::string error;
  EditorLaunchArguments arguments;
};
struct EditorLaunchResult {
  bool ok = false;
  bool help = false;
  std::string error;
  EditorLaunchOptions options;
};

// Pass arguments after argv[0]. Every path switch takes a separate value.
[[nodiscard]] EditorLaunchParseResult parse_editor_launch_arguments(std::span<const std::string> args);
[[nodiscard]] EditorLaunchResult resolve_editor_launch_options(const EditorLaunchArguments& args,
                                                               const EditorLaunchContext& context);
[[nodiscard]] std::string_view editor_launch_usage();

} // namespace rat
