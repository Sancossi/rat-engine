#include "launch_environment.hpp"
#include "../../platform/process.hpp"

#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <vector>

#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#include <shlobj.h>
#else
#include <unistd.h>
#endif

namespace rat {
namespace {
std::string utf8(const std::filesystem::path& path) {
  const auto value = path.generic_u8string();
  return {value.begin(), value.end()};
}
#ifdef _WIN32
std::string utf8(std::wstring_view value) {
  if (value.empty()) return {};
  const int size = WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), nullptr, 0, nullptr, nullptr);
  if (!size) throw std::runtime_error("invalid Unicode process value");
  std::string out(static_cast<std::size_t>(size), '\0');
  if (!WideCharToMultiByte(CP_UTF8, WC_ERR_INVALID_CHARS, value.data(), static_cast<int>(value.size()), out.data(), size, nullptr, nullptr))
    throw std::runtime_error("failed to convert Unicode process value");
  return out;
}
std::optional<std::string> environment(const wchar_t* key) {
  const DWORD size = GetEnvironmentVariableW(key, nullptr, 0);
  if (!size) return std::nullopt;
  std::wstring value(size, L'\0');
  const DWORD length = GetEnvironmentVariableW(key, value.data(), size);
  if (!length || length >= size) throw std::runtime_error("failed to read process environment");
  value.resize(length);
  return utf8(std::wstring_view(value));
}
#endif
} // namespace

EditorLaunchResult editor_launch_from_process(int argc, char** argv) {
  try {
    const auto args = process_arguments(argc, argv);
    const auto parsed = parse_editor_launch_arguments(args);
    if (!parsed.ok || parsed.help) return {parsed.ok, parsed.help, parsed.error, {}};
    EditorLaunchContext context;
    context.launch_directory = utf8(std::filesystem::current_path());
#ifdef _WIN32
    context.executable_path = utf8(executable_path());
    if (!parsed.arguments.user_data_dir) {
      PWSTR appdata = nullptr;
      if (FAILED(SHGetKnownFolderPath(FOLDERID_LocalAppData, 0, nullptr, &appdata)))
        throw std::runtime_error("failed to resolve LocalAppData");
      struct FolderGuard { PWSTR value; ~FolderGuard() { CoTaskMemFree(value); } } folder{appdata};
      context.default_user_data_dir = utf8(std::filesystem::path(appdata) / "rat-engine");
    }
    context.log_override = environment(L"RAT_LOG_PATH");
#else
    context.executable_path = utf8(executable_path());
    if (!parsed.arguments.user_data_dir) {
      const char* xdg = std::getenv("XDG_DATA_HOME");
      const char* home = std::getenv("HOME");
      if (xdg && *xdg && std::filesystem::path(xdg).is_absolute())
        context.default_user_data_dir = utf8(std::filesystem::path(xdg) / "rat-engine");
      else if (home && *home && std::filesystem::path(home).is_absolute())
        context.default_user_data_dir = utf8(std::filesystem::path(home) / ".local/share/rat-engine");
      else throw std::runtime_error("HOME or an absolute XDG_DATA_HOME is required without --user-data-dir");
    }
    if (const char* log = std::getenv("RAT_LOG_PATH"); log && *log) context.log_override = log;
#endif
    return resolve_editor_launch_options(parsed.arguments, context);
  } catch (const std::exception& ex) { return {false, false, ex.what(), {}}; }
}
} // namespace rat
