#include "editor_document.hpp"
#include "frame_coordinator.hpp"

#include <rat/app_mode.hpp>
#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/simulation_session.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <string>
#include <vector>

using Catch::Approx;

namespace {

rat::MapData make_doc_map() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "editor_doc";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 4;
  map.height_grid.height = 4;
  map.height_grid.ground_y.assign(16, 0.0f);
  return map;
}

rat::BlockerDef make_blocker(float min_x, float min_z, float max_x, float max_z) {
  rat::BlockerDef blocker;
  blocker.bounds = {min_x, min_z, max_x, max_z};
  return blocker;
}

}  // namespace

TEST_CASE("EditorDocument execute places a blocker, marks dirty, and records undo",
          "[unit][editordoc]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  REQUIRE_FALSE(document.dirty());
  REQUIRE_FALSE(document.can_undo());

  const rat::EditApplyResult result =
      document.execute(rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f)));
  REQUIRE(result.applied);
  REQUIRE(result.mutates_blockers);
  REQUIRE(document.data().blockers.size() == 1);
  REQUIRE(document.data().blockers[0].bounds.max_x == Approx(1.0f));
  REQUIRE(document.dirty());
  REQUIRE(document.can_undo());
  REQUIRE_FALSE(document.can_redo());
}

TEST_CASE("EditorDocument undo/redo restore map without touching EventRuntime",
          "[unit][editordoc]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  REQUIRE(document.execute(rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f))));
  REQUIRE(document.undo().applied);
  REQUIRE(document.data().blockers.empty());
  REQUIRE(document.can_redo());
  REQUIRE(document.redo().applied);
  REQUIRE(document.data().blockers.size() == 1);
}

TEST_CASE("EditorDocument clamps blocker selection after delete", "[unit][editordoc]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  REQUIRE(document.execute(rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f))));
  REQUIRE(document.execute(rat::make_place_blocker_command(make_blocker(1.0f, 0.0f, 2.0f, 1.0f))));
  document.select_blocker(1);
  REQUIRE(document.selected_blocker() == 1);
  REQUIRE(document.selected_event() == -1);

  REQUIRE(document.execute(rat::make_delete_blocker_command(1)));
  REQUIRE(document.data().blockers.size() == 1);
  REQUIRE(document.selected_blocker() == 0);
}

TEST_CASE("Play/Edit session mode does not destroy EditorDocument authoring state",
          "[unit][editordoc]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  REQUIRE(document.execute(rat::make_place_event_command(rat::make_stub_event("talk", 1, 1))));
  document.select_event(0);
  const std::uint64_t revision = document.map().revision();
  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(document.data());
  REQUIRE(serialized.ok);

  rat::SimulationSession session;
  REQUIRE(session.load(document.data()).ok);
  session.set_app_mode(rat::AppMode::Edit);
  session.set_app_mode(rat::AppMode::Play);
  session.set_app_mode(rat::AppMode::Edit);

  REQUIRE(document.map().revision() == revision);
  REQUIRE(document.selected_event() == 0);
  REQUIRE(document.can_undo());
  REQUIRE(document.dirty());
  const rat::MapSerializeResult after = rat::serialize_map_to_string(document.data());
  REQUIRE(after.ok);
  REQUIRE(after.json_text == serialized.json_text);
}

TEST_CASE("EditorDocument load replaces map, clears history, and marks clean", "[unit][editordoc]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  REQUIRE(document.execute(rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f))));
  REQUIRE(document.dirty());

  rat::MapData next = make_doc_map();
  next.id = "reloaded";
  document.load(std::move(next));
  REQUIRE(document.data().id == "reloaded");
  REQUIRE(document.data().blockers.empty());
  REQUIRE_FALSE(document.dirty());
  REQUIRE_FALSE(document.can_undo());
  REQUIRE_FALSE(document.can_redo());
}

TEST_CASE("EditorDocument preview does not dirty committed map or history", "[unit][editordoc]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  REQUIRE(document.execute(rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f))));
  document.select_blocker(0);
  document.mark_clean();
  REQUIRE_FALSE(document.dirty());

  rat::BlockerDef preview = document.data().blockers[0];
  preview.bounds.max_x = 3.0f;
  REQUIRE(document.preview_blocker(0, preview));
  REQUIRE(document.preview_active());
  REQUIRE(document.visible_data().blockers[0].bounds.max_x == Approx(3.0f));
  REQUIRE(document.data().blockers[0].bounds.max_x == Approx(1.0f));
  REQUIRE_FALSE(document.dirty());
  REQUIRE(document.can_undo());

  document.discard_preview();
  REQUIRE_FALSE(document.preview_active());
  REQUIRE(document.visible_data().blockers[0].bounds.max_x == Approx(1.0f));
}

TEST_CASE("FrameCoordinator runs poll, simulate, audio, ui, present in order",
          "[unit][editordoc]") {
  std::vector<std::string> order;
  rat::FrameCoordinator coordinator;
  coordinator.poll = [&] { order.emplace_back("poll"); };
  coordinator.simulate = [&](float dt) {
    REQUIRE(dt == Approx(0.016f));
    order.emplace_back("simulate");
  };
  coordinator.drain_audio = [&] { order.emplace_back("audio"); };
  coordinator.begin_ui = [&] { order.emplace_back("begin_ui"); };
  coordinator.draw_ui = [&] { order.emplace_back("draw"); };
  coordinator.present = [&] { order.emplace_back("present"); };
  coordinator.run_frame(0.016f);
  REQUIRE(order == std::vector<std::string>{"poll", "simulate", "audio", "begin_ui", "draw",
                                            "present"});
}
