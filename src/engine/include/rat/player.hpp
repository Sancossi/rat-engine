#pragma once

#include "rat/camera.hpp"

#include <span>

namespace rat {

class SurfaceQuery;
struct BlockerDef;
struct EdgeBarrierDef;
struct MapData;

struct Aabb2 {
  float min_x = 0.0f;
  float min_z = 0.0f;
  float max_x = 0.0f;
  float max_z = 0.0f;
};

struct PlayerBody {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  float half_extent = 0.4f;
  float speed = 5.0f;
};

struct MoveInput {
  float axis_x = 0.0f;  // world X delta weight (after camera remap)
  float axis_z = 0.0f;  // world Z delta weight (after camera remap)
};

struct JumpState {
  float jump_offset = 0.0f;
  float vertical_speed = 0.0f;
  float coyote_time_left = 0.0f;
  float jump_buffer_left = 0.0f;
  bool grounded = true;
  int support_blocker_index = -1;
  float ladder_lockout_left = 0.0f;
  float ladder_bounce_x = 0.0f;
  float ladder_bounce_z = 0.0f;
  bool climbing = false;  // set by integrate when the Climb motor owns the tick
  float climb_into_x = 0.0f;
  float climb_into_z = 0.0f;
};

struct JumpTuning {
  float gravity = 24.0f;
  float jump_speed = 7.5f;
  float jump_cut = 0.4f;
  float faster_fall_gravity = 42.0f;
  float max_fall_speed = 32.0f;
  float coyote_seconds = 0.1f;
  float input_buffer_seconds = 0.1f;
  float max_substep_seconds = 1.0f / 120.0f;
  float ladder_lockout_seconds = 0.20f;
  float ladder_hop_speed = 4.0f;
  float ladder_bounce_speed = 4.0f;
};

inline constexpr float kLadderBounceNudge = 0.6f;

struct PlayerFrameInput {
  MoveInput move{};
  MoveInput climb_move{};  // camera-relative; empty → climb uses `move`
  bool jump_pressed = false;
  bool jump_held = false;
  bool interact_pressed = false;
};

struct PlayerFrameResult {
  PlayerBody body{};
  JumpState jump{};
  bool landed = false;  // true if any substep goes airborne→grounded; stays true if they take off later this frame
};

[[nodiscard]] JumpState make_grounded_jump_state();

// Screen WASD → world XZ for ortho 3/4: screen_x = A/D (-1..1), screen_z = S/W (-1..1, W=+1).
[[nodiscard]] MoveInput camera_relative_move(float screen_x, float screen_z, Vec3 eye,
                                             Vec3 focus);

// Stable editor/game axes across camera switches: W=-Z, D=-X. Play input no longer uses this.
[[nodiscard]] MoveInput world_aligned_move(float screen_x, float screen_z);

[[nodiscard]] PlayerBody integrate_player(PlayerBody player, MoveInput input, float dt,
                                          std::span<const BlockerDef> blockers,
                                          std::span<const EdgeBarrierDef> edge_barriers = {},
                                          const SurfaceQuery* surface_query = nullptr,
                                          const MapData* map = nullptr);

[[nodiscard]] PlayerBody integrate_player_surface(PlayerBody player, MoveInput input, float dt,
                                                  std::span<const BlockerDef> blockers,
                                                  const SurfaceQuery& surface_query,
                                                  float max_step_up = 0.35f,
                                                  std::span<const EdgeBarrierDef> edge_barriers = {},
                                                  const MapData* map = nullptr,
                                                  bool ignore_ladders = false,
                                                  MoveInput climb_move = {});

[[nodiscard]] PlayerFrameResult integrate_player_frame_surface(
    PlayerBody player, JumpState jump, const PlayerFrameInput& input, float dt,
    std::span<const BlockerDef> blockers, const SurfaceQuery& surface_query,
    const JumpTuning& tuning = {}, float max_step_up = 0.35f,
    std::span<const EdgeBarrierDef> edge_barriers = {}, const MapData* map = nullptr);

[[nodiscard]] Vec3 snap_to_grid(float x, float y, float z, float tile_size);

[[nodiscard]] bool aabb_overlap(const Aabb2& a, const Aabb2& b);

}  // namespace rat
