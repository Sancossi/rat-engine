#include <rat/entity.hpp>
#include <rat/map_data.hpp>
#include <rat/player.hpp>
#include <rat/render_world.hpp>
#include <rat/simulation_session.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <cstdint>
#include <cstring>
#include <string>
#include <type_traits>
#include <vector>

using Catch::Approx;

namespace {

rat::MapData make_flat_map_with_blocker() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "rw_flat";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);
  rat::BlockerDef blocker;
  blocker.bounds = rat::Aabb2{2.0f, 1.0f, 4.0f, 3.0f};
  map.blockers.push_back(blocker);
  return map;
}

}  // namespace

TEST_CASE("build_render_world emits a packet from Transform and Renderable stores",
          "[unit][render]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  rat::ComponentStore<rat::Renderable> renderables;

  const rat::EntityId id = entities.create();
  rat::Transform xf;
  xf.position = {1.5f, 0.25f, -2.0f};
  xf.yaw = 0.5f;
  REQUIRE(transforms.insert(entities, id, xf));
  rat::Renderable drawable;
  drawable.mesh = rat::make_asset_id("mesh/player");
  REQUIRE(renderables.insert(entities, id, drawable));

  rat::RenderWorldBuildInput input;
  input.entities = &entities;
  input.current = &transforms;
  input.renderables = &renderables;
  input.interpolation_alpha = 1.0f;
  input.tick_id = 7;

  const rat::RenderWorld world = rat::build_render_world(input);
  REQUIRE(world.packets.size() == 1);
  CHECK(world.tick_id == 7);
  CHECK(world.interpolation_alpha == Approx(1.0f));
  CHECK(world.packets[0].entity == id);
  CHECK(world.packets[0].transform.position.x == Approx(1.5f));
  CHECK(world.packets[0].transform.position.y == Approx(0.25f));
  CHECK(world.packets[0].transform.position.z == Approx(-2.0f));
  CHECK(world.packets[0].mesh.key() == "mesh/player");
  CHECK(world.packets[0].pass == rat::RenderPass::Opaque);
}

TEST_CASE("interpolation at alpha 1 is identity with the current Transform", "[unit][render]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> previous;
  rat::ComponentStore<rat::Transform> current;
  rat::ComponentStore<rat::Renderable> renderables;

  const rat::EntityId id = entities.create();
  rat::Transform prev_xf;
  prev_xf.position = {0.0f, 0.0f, 0.0f};
  rat::Transform curr_xf;
  curr_xf.position = {10.0f, 4.0f, -6.0f};
  REQUIRE(previous.insert(entities, id, prev_xf));
  REQUIRE(current.insert(entities, id, curr_xf));
  rat::Renderable drawable;
  drawable.mesh = rat::make_asset_id("mesh/player");
  REQUIRE(renderables.insert(entities, id, drawable));

  rat::RenderWorldBuildInput input;
  input.entities = &entities;
  input.previous = &previous;
  input.current = &current;
  input.renderables = &renderables;
  input.interpolation_alpha = 1.0f;

  const rat::RenderWorld world = rat::build_render_world(input);
  REQUIRE(world.packets.size() == 1);
  CHECK(world.packets[0].transform.position.x == Approx(10.0f));
  CHECK(world.packets[0].transform.position.y == Approx(4.0f));
  CHECK(world.packets[0].transform.position.z == Approx(-6.0f));
}

TEST_CASE("interpolation at alpha 0.5 lerps previous and current position", "[unit][render]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> previous;
  rat::ComponentStore<rat::Transform> current;
  rat::ComponentStore<rat::Renderable> renderables;

  const rat::EntityId id = entities.create();
  rat::Transform prev_xf;
  prev_xf.position = {0.0f, 0.0f, 0.0f};
  prev_xf.yaw = 0.0f;
  rat::Transform curr_xf;
  curr_xf.position = {10.0f, 4.0f, -6.0f};
  curr_xf.yaw = 2.0f;
  REQUIRE(previous.insert(entities, id, prev_xf));
  REQUIRE(current.insert(entities, id, curr_xf));
  rat::Renderable drawable;
  drawable.mesh = rat::make_asset_id("mesh/player");
  REQUIRE(renderables.insert(entities, id, drawable));

  rat::RenderWorldBuildInput input;
  input.entities = &entities;
  input.previous = &previous;
  input.current = &current;
  input.renderables = &renderables;
  input.interpolation_alpha = 0.5f;

  const rat::RenderWorld world = rat::build_render_world(input);
  REQUIRE(world.packets.size() == 1);
  CHECK(world.packets[0].transform.position.x == Approx(5.0f));
  CHECK(world.packets[0].transform.position.y == Approx(2.0f));
  CHECK(world.packets[0].transform.position.z == Approx(-3.0f));
  CHECK(world.packets[0].transform.yaw == Approx(1.0f));
}

TEST_CASE("packets sort by pass then material handle sort_key", "[unit][render]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  rat::ComponentStore<rat::Renderable> renderables;

  const rat::EntityId opaque_late = entities.create();
  const rat::EntityId debug_ent = entities.create();
  const rat::EntityId opaque_early = entities.create();

  REQUIRE(transforms.insert(entities, opaque_late, rat::Transform{}));
  REQUIRE(transforms.insert(entities, debug_ent, rat::Transform{}));
  REQUIRE(transforms.insert(entities, opaque_early, rat::Transform{}));

  rat::Renderable late_draw;
  late_draw.mesh = rat::make_asset_id("mesh/b");
  rat::Renderable debug_draw;
  debug_draw.mesh = rat::make_asset_id("mesh/debug");
  rat::Renderable early_draw;
  early_draw.mesh = rat::make_asset_id("mesh/a");
  REQUIRE(renderables.insert(entities, opaque_late, late_draw));
  REQUIRE(renderables.insert(entities, debug_ent, debug_draw));
  REQUIRE(renderables.insert(entities, opaque_early, early_draw));

  rat::RenderWorldBuildInput input;
  input.entities = &entities;
  input.current = &transforms;
  input.renderables = &renderables;

  rat::RenderWorld world = rat::build_render_world(input);
  REQUIRE(world.packets.size() == 3);
  for (rat::RenderPacket& packet : world.packets) {
    if (packet.entity == debug_ent) {
      packet.pass = rat::RenderPass::Debug;
    }
    if (packet.entity == opaque_late) {
      packet.material.sort_key = 20;
    }
    if (packet.entity == opaque_early) {
      packet.material.sort_key = 5;
    }
  }

  rat::sort_render_packets(world.packets);
  REQUIRE(world.packets.size() == 3);
  CHECK(world.packets[0].entity == opaque_early);
  CHECK(world.packets[1].entity == opaque_late);
  CHECK(world.packets[2].entity == debug_ent);
  CHECK(world.packets[0].pass == rat::RenderPass::Opaque);
  CHECK(world.packets[2].pass == rat::RenderPass::Debug);
}

TEST_CASE("main pass names are Depth Opaque Debug for profiler capture", "[unit][render]") {
  CHECK(std::string(rat::render_pass_name(rat::RenderPass::Depth)) == "Depth");
  CHECK(std::string(rat::render_pass_name(rat::RenderPass::Opaque)) == "Opaque");
  CHECK(std::string(rat::render_pass_name(rat::RenderPass::Debug)) == "Debug");

  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  rat::ComponentStore<rat::Renderable> renderables;
  rat::RenderWorldBuildInput input;
  input.entities = &entities;
  input.current = &transforms;
  input.renderables = &renderables;
  const rat::RenderWorld world = rat::build_render_world(input);
  REQUIRE(world.pass_names.size() == 3);
  CHECK(world.pass_names[0] == "Depth");
  CHECK(world.pass_names[1] == "Opaque");
  CHECK(world.pass_names[2] == "Debug");
  CHECK(world.cpu_markers.size() >= 1);
  CHECK(std::find(world.cpu_markers.begin(), world.cpu_markers.end(), "build_render_world") !=
        world.cpu_markers.end());
}

TEST_CASE("fill_render_stores takes player and map not GameState", "[unit][render]") {
  STATIC_REQUIRE(!std::is_same_v<rat::PlayerBody, rat::RenderWorld>);

  rat::PlayerBody player;
  player.x = 3.0f;
  player.y = 1.0f;
  player.z = -1.5f;
  const rat::MapData map = make_flat_map_with_blocker();

  rat::RenderStoreFill stores;
  rat::fill_render_stores(stores, player, map);

  rat::RenderWorldBuildInput input;
  input.entities = &stores.entities;
  input.current = &stores.transforms;
  input.renderables = &stores.renderables;
  const rat::RenderWorld world = rat::build_render_world(input);

  REQUIRE(world.packets.size() == 2);
  const auto player_it = std::find_if(world.packets.begin(), world.packets.end(),
                                      [](const rat::RenderPacket& packet) {
                                        return packet.mesh.key() == "mesh/player";
                                      });
  REQUIRE(player_it != world.packets.end());
  CHECK(player_it->transform.position.x == Approx(3.0f));
  CHECK(player_it->transform.position.y == Approx(1.0f));
  CHECK(player_it->transform.position.z == Approx(-1.5f));

  const auto blocker_it = std::find_if(world.packets.begin(), world.packets.end(),
                                       [](const rat::RenderPacket& packet) {
                                         return packet.mesh.key() == "mesh/blocker";
                                       });
  REQUIRE(blocker_it != world.packets.end());
  CHECK(blocker_it->transform.position.x == Approx(3.0f));
  CHECK(blocker_it->transform.position.z == Approx(2.0f));
}

TEST_CASE("capture_render_world from SimulationSession does not call bgfx", "[unit][render]") {
  rat::SimulationSession session;
  REQUIRE(session.load(make_flat_map_with_blocker()).ok);
  rat::PlayerBody player;
  player.x = 1.25f;
  player.y = 0.0f;
  player.z = 1.75f;
  session.set_player(player);

  const rat::RenderWorld world = rat::capture_render_world(session);
  REQUIRE(world.tick_id == session.tick_id());
  CHECK(world.interpolation_alpha == Approx(1.0f));
  REQUIRE(world.packets.size() >= 1);
  const auto player_it = std::find_if(world.packets.begin(), world.packets.end(),
                                      [](const rat::RenderPacket& packet) {
                                        return packet.mesh.key() == "mesh/player";
                                      });
  REQUIRE(player_it != world.packets.end());
  CHECK(player_it->transform.position.x == Approx(1.25f));
  CHECK(player_it->transform.position.z == Approx(1.75f));
  CHECK(world.pass_names.size() == 3);
}

TEST_CASE("FrameAllocator bump resets without keeping the previous allocation live",
          "[unit][render]") {
  rat::FrameAllocator frame;
  void* first = frame.allocate(16);
  REQUIRE(first != nullptr);
  CHECK(frame.used() >= 16);
  std::memset(first, 0xAB, 16);
  frame.reset();
  CHECK(frame.used() == 0);
  void* second = frame.allocate(8);
  REQUIRE(second != nullptr);
  CHECK(frame.used() >= 8);
}

TEST_CASE("DeferredGpuFreeQueue retires handles after the frame not immediately",
          "[unit][render]") {
  rat::DeferredGpuFreeQueue queue;
  rat::GpuHandle handle;
  handle.generation = 3;
  CHECK(queue.status(handle) == rat::GpuHandleStatus::Invalid);
  queue.queue(handle);
  CHECK(queue.status(handle) == rat::GpuHandleStatus::PendingDestroy);
  queue.retire();
  CHECK(queue.status(handle) == rat::GpuHandleStatus::Destroyed);
}

TEST_CASE("RenderDocHook is a stub that does not capture", "[unit][render]") {
  rat::RenderDocHook hook;
  REQUIRE_FALSE(hook.available());
  hook.begin_capture();
  CHECK_FALSE(hook.capturing());
  hook.end_capture();
  CHECK_FALSE(hook.capturing());
}
