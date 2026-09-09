#include "process.hpp"
#include <stdexcept>
#ifdef _WIN32
#include <windows.h>
#include <shellapi.h>
#else
#include <unistd.h>
#endif
namespace rat {
std::vector<std::string> process_arguments(int argc, char** argv) {
  std::vector<std::string> args;
#ifdef _WIN32
  (void)argc; (void)argv;
  int count = 0;
  auto** wide = CommandLineToArgvW(GetCommandLineW(), &count);
  if (!wide) throw std::runtime_error("Cannot read process arguments");
  struct Guard { wchar_t** p; ~Guard() { LocalFree(p); } } guard{wide};
  for (int i = 1; i < count; ++i) {
    const auto value = std::filesystem::path(wide[i]).u8string();
    args.emplace_back(value.begin(), value.end());
  }
#else
  for (int i = 1; i < argc; ++i) args.emplace_back(argv[i]);
#endif
  return args;
}
std::filesystem::path executable_path() {
#ifdef _WIN32
  std::wstring buffer(32768, L'\0');
  const DWORD length = GetModuleFileNameW(nullptr, buffer.data(), static_cast<DWORD>(buffer.size()));
  if (!length || length == buffer.size()) throw std::runtime_error("Cannot resolve executable path");
  buffer.resize(length);
  return std::filesystem::path(buffer);
#else
  std::vector<char> buffer(4096);
  for (;;) {
    const auto length = readlink("/proc/self/exe", buffer.data(), buffer.size());
    if (length < 0) throw std::runtime_error("Cannot resolve /proc/self/exe");
    if (static_cast<std::size_t>(length) < buffer.size())
      return std::filesystem::path(std::string(buffer.data(), static_cast<std::size_t>(length)));
    buffer.resize(buffer.size() * 2);
  }
#endif
}
}
