#pragma once
#include <rat/map_data.hpp>
#include <filesystem>
#include <map>
#include <string>
namespace rat::expedition {
struct Scene {
  std::string id;
  MapData map;
  std::map<std::string, Vec3> spawns;
  Vec3 camera_focus{};
  float recovery_y = -2;
};
struct Project {
  std::filesystem::path root;
  std::filesystem::path sprite;
  std::string start_scene;
  std::map<std::string, Scene> scenes;
};
// Throws a contextual error. Every path must remain inside root, including symlinks.
std::filesystem::path asset_path(const std::filesystem::path& root, const std::string& relative);
Project load_project(const std::filesystem::path& root);
}
