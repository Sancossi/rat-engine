#pragma once
#include <filesystem>
#include <string>
#include <vector>
namespace rat::expedition {
struct LaunchOptions {
  bool help = false, hidden = false;
  int width = 1280, height = 720;
  unsigned frames = 0;
  std::filesystem::path data_dir, screenshot, report;
  std::string renderer = "auto";
};
LaunchOptions parse_launch_options(const std::vector<std::string>& args, const std::filesystem::path& executable);
}
