#include "scene.hpp"
#include <rat/map_loader.hpp>
#include <rat/map_document.hpp>
#include <rat/collision.hpp>
#include <nlohmann/json.hpp>
#include <cmath>
#include <fstream>
#include <stdexcept>
#include <unordered_set>
namespace rat::expedition {
namespace {
using Json = nlohmann::json;
Json read_json(const std::filesystem::path& path) {
  std::ifstream stream(path, std::ios::binary);
  if (!stream) throw std::runtime_error("Cannot open " + path.generic_string());
  std::vector<std::unordered_set<std::string>> keys;
  try { return Json::parse(stream, [&keys](int, Json::parse_event_t event, Json& value) {
    if (event == Json::parse_event_t::object_start) keys.emplace_back();
    if (event == Json::parse_event_t::key && !keys.back().insert(value.get<std::string>()).second)
      throw std::runtime_error("Duplicate JSON key: " + value.get<std::string>());
    if (event == Json::parse_event_t::object_end) keys.pop_back();
    return true;
  }); }
  catch (const std::exception& ex) { throw std::runtime_error(path.generic_string() + ": " + ex.what()); }
}
float finite(const Json& value) {
  if (!value.is_number()) throw std::runtime_error("Expected a finite number");
  const float result = value.get<float>();
  if (!std::isfinite(result)) throw std::runtime_error("Expected a finite number");
  return result;
}
Vec3 position(const Json& value) { return {finite(value.at("x")), finite(value.at("y")), finite(value.at("z"))}; }
void schema(const Json& value) {
  if (!value.is_object() || !value.contains("schema_version") || !value.at("schema_version").is_number_integer()
      || value.at("schema_version") != 1) throw std::runtime_error("Expected expedition schema_version 1");
}
}
std::filesystem::path asset_path(const std::filesystem::path& root, const std::string& relative) {
  const auto path = std::filesystem::path(std::u8string(relative.begin(), relative.end()));
  if (relative.empty() || relative.find('\\') != std::string::npos || relative.find(':') != std::string::npos
      || path.is_absolute() || path.has_root_path()) throw std::runtime_error("Asset path must be relative: " + relative);
  for (const auto& part : path) if (part == ".." || part == ".") throw std::runtime_error("Unsafe asset path: " + relative);
  const auto base = std::filesystem::canonical(root);
  const auto resolved = std::filesystem::weakly_canonical(base / path);
  const auto inside = resolved.lexically_relative(base);
  if (inside.empty() || inside.is_absolute() || *inside.begin() == "..") throw std::runtime_error("Asset escapes data directory: " + relative);
  return resolved;
}
Project load_project(const std::filesystem::path& root) {
  try {
    Project out;
    out.root = std::filesystem::canonical(root);
    const auto project = read_json(out.root / "project.json");
    schema(project);
    out.start_scene = project.at("start_scene").get<std::string>();
    out.sprite = asset_path(out.root, project.at("sprite").get<std::string>());
    if (!std::filesystem::is_regular_file(out.sprite)) throw std::runtime_error("Missing sprite PNG: " + out.sprite.generic_string());
    const auto& scenes = project.at("scenes");
    if (!scenes.is_object() || scenes.empty()) throw std::runtime_error("Project must contain scenes");
    for (const auto& [id, metadata] : scenes.items()) {
      const auto path = asset_path(out.root, metadata.get<std::string>());
      const auto data = read_json(path);
      schema(data);
      Scene scene;
      scene.id = data.at("scene_id").get<std::string>();
      if (scene.id.empty() || scene.id != id) throw std::runtime_error("Scene id mismatch: " + id);
      const auto map_path = asset_path(out.root, data.at("map").get<std::string>());
      std::ifstream map_file(map_path, std::ios::binary);
      if (!map_file) throw std::runtime_error("Cannot open map " + map_path.generic_string());
      const std::string text((std::istreambuf_iterator<char>(map_file)), std::istreambuf_iterator<char>());
      auto map = load_map_from_string(text);
      if (!map.ok) throw std::runtime_error(map_path.generic_string() + ": " + map.error);
      if (map.map.id != id) throw std::runtime_error("Map id mismatch: " + id);
      scene.map = std::move(map.map);
      const auto& spawns = data.at("spawns");
      if (!spawns.is_object()) throw std::runtime_error("spawns must be an object");
      for (const auto& [name, coordinates] : spawns.items()) {
        if (name.empty()) throw std::runtime_error("Empty spawn id");
        scene.spawns.emplace(name, position(coordinates));
      }
      if (!scene.spawns.contains("entry")) throw std::runtime_error("Missing entry spawn: " + id);
      SurfaceQuery surface(scene.map);
      const auto world = bake_collision_world(scene.map, surface);
      for (const auto& [name, spawn] : scene.spawns) {
        const auto support = query_solid_support(world, spawn.x, spawn.z, 0.2f, spawn.y, 0.001f);
        CollisionBody body; body.x=spawn.x; body.y=spawn.y; body.z=spawn.z; body.radius=0.2f;
        bool blocked = cylinder_hits_ceiling(body, world) || cylinder_hits_walls(body, world, 0.001f) || cylinder_hits_fences(body, world);
        const Aabb2 footprint{spawn.x-0.2f,spawn.z-0.2f,spawn.x+0.2f,spawn.z+0.2f};
        for (const auto& blocker : scene.map.blockers)
          if (blocker_blocks_feet(blocker, spawn.y) && aabb_overlap(footprint, blocker.bounds)) blocked=true;
        if (!support || std::abs(support->y-spawn.y)>0.001f || blocked)
          throw std::runtime_error("Spawn has no free stable support: " + id + "/" + name);
      }
      scene.camera_focus = position(data.at("camera_focus"));
      scene.recovery_y = finite(data.at("recovery_y"));
      // P1.1 has no interactive metadata. Reject it until the owning slice implements validation.
      for (const char* key : {"portals", "low_passages", "occluder_groups"})
        if (!data.at(key).is_array() || !data.at(key).empty()) throw std::runtime_error(std::string(key) + " requires P1 traversal implementation");
      out.scenes.emplace(id, std::move(scene));
    }
    if (!out.scenes.contains(out.start_scene)) throw std::runtime_error("Unknown start_scene: " + out.start_scene);
    return out;
  } catch (const std::exception& ex) { throw std::runtime_error("Expedition project " + root.generic_string() + ": " + ex.what()); }
}
}
