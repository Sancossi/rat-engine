#include "editor_launch_options.hpp"

#include <filesystem>
#include <stdexcept>

namespace rat {
namespace {
std::filesystem::path path(std::string_view value) {
  return std::filesystem::path(std::u8string(value.begin(), value.end()));
}
std::string utf8(const std::filesystem::path& value) {
  const auto text = value.generic_u8string();
  return {text.begin(), text.end()};
}
std::filesystem::path resolve(std::string_view value, const std::filesystem::path& cwd) {
  if (value.empty() || value.find('\0') != std::string_view::npos)
    throw std::runtime_error("path must not be empty or contain NUL");
  auto candidate = path(value);
  if (candidate.is_relative()) candidate = cwd / candidate;
  candidate = candidate.lexically_normal();
  if (!candidate.is_absolute()) throw std::runtime_error("path must resolve to an absolute path");
  return candidate;
}
} // namespace

EditorLaunchParseResult parse_editor_launch_arguments(std::span<const std::string> args) {
  EditorLaunchParseResult out;
  for (std::size_t i = 0; i < args.size(); ++i) {
    const auto& arg = args[i];
    if (arg == "--help" || arg == "-h") { out.help = true; continue; }
    std::optional<std::string>* value = nullptr;
    if (arg == "--data-root") value = &out.arguments.data_root;
    else if (arg == "--user-data-dir") value = &out.arguments.user_data_dir;
    else if (arg == "--map") value = &out.arguments.map;
    else { out.error = "unknown argument: " + arg; return out; }
    if (value->has_value()) { out.error = "duplicate argument: " + arg; return out; }
    if (i + 1 == args.size() || args[i + 1].empty() || args[i + 1].starts_with("--")) {
      out.error = "missing path after " + arg; return out;
    }
    *value = args[++i];
  }
  out.ok = true;
  return out;
}

EditorLaunchResult resolve_editor_launch_options(const EditorLaunchArguments& args,
                                                const EditorLaunchContext& context) {
  EditorLaunchResult out;
  try {
    const auto cwd = path(context.launch_directory);
    if (!cwd.is_absolute()) throw std::runtime_error("launch directory must be absolute");
    const auto executable = resolve(context.executable_path, cwd);
    const auto data = args.data_root ? resolve(*args.data_root, cwd) : executable.parent_path() / "data";
    const auto user = resolve(args.user_data_dir.value_or(context.default_user_data_dir), cwd);
    out.options.data_root = utf8(data.lexically_normal());
    out.options.user_data_dir = utf8(user);
    out.options.map_path = utf8(args.map ? resolve(*args.map, cwd) : data / "maps/grey_yard.json");
    out.options.save_slot_path = utf8(user / "saves/slot1.ratsave");
    out.options.log_path = utf8(context.log_override ? resolve(*context.log_override, cwd) : user / "logs/rat.log");
    out.options.debug_snapshot_path = utf8(user / "debug/rat-debug.json");
    out.options.imgui_ini_path = utf8(user / "imgui.ini");
    out.ok = true;
  } catch (const std::exception& ex) { out.error = ex.what(); }
  return out;
}

std::string_view editor_launch_usage() {
  return "rat-editor [--data-root PATH] [--user-data-dir PATH] [--map PATH]\n"
         "Paths are resolved relative to the launch directory once.\n"
         "Defaults: executable-adjacent data, platform user data, data/maps/grey_yard.json.\n";
}
} // namespace rat
