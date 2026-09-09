#pragma once
#include "scene.hpp"
#include <rat/simulation_session.hpp>
#include <rat/surface_query.hpp>
#include <memory>
namespace rat::expedition {
struct TraversalInput { float screen_x = 0, screen_y = 0; bool crouch_held = false, interact_pressed = false, pause_pressed = false; };
struct TraversalSnapshot { std::string scene_id; Vec3 leader; std::uint64_t tick = 0; int direction = 0, frame = 0; };
class ExpeditionSession {
 public:
  explicit ExpeditionSession(Project project);
  void tick(const TraversalInput& input);
  const Scene& scene() const { return project_.scenes.at(project_.start_scene); }
  const Project& project() const { return project_; }
  const SimulationSession& simulation() const { return *simulation_; }
  TraversalSnapshot snapshot() const;
 private:
  Project project_;
  std::unique_ptr<SimulationSession> simulation_;
  int direction_ = 0;
  bool moving_ = false;
};
}
