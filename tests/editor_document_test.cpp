#include "editor_document.hpp"
#include "frame_coordinator.hpp"
#include "editor_action_controller.hpp"
#include <rat/authoring_snapshot.hpp>

#include <rat/app_mode.hpp>
#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/height_edit.hpp>
#include <rat/map_data.hpp>
#include <rat/map_loader.hpp>
#include <rat/simulation_session.hpp>
#include <rat/save_game.hpp>
#include <limits>

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
  REQUIRE(result.ok);
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
  REQUIRE(document.undo().ok);
  REQUIRE(document.data().blockers.empty());
  REQUIRE(document.can_redo());
  REQUIRE(document.redo().ok);
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

TEST_CASE("EditorDocument grouped cubes undo as one stroke", "[unit][editordoc][edit]") {
  rat::EditorDocument document;
  document.load(make_doc_map());
  document.begin_stroke();
  REQUIRE(document.execute(rat::make_place_map_tile_cube_command(0, 0)));
  REQUIRE(document.execute(rat::make_place_map_tile_cube_command(1, 0)));
  document.end_stroke();
  const rat::HeightGetResult raised0 = rat::get_tile_ground_y(document.data().height_grid, 0, 0);
  const rat::HeightGetResult raised1 = rat::get_tile_ground_y(document.data().height_grid, 1, 0);
  REQUIRE(raised0.ok);
  REQUIRE(raised1.ok);
  REQUIRE(raised0.value == Approx(rat::kPlaceCubeDeltaY));
  REQUIRE(raised1.value == Approx(rat::kPlaceCubeDeltaY));
  REQUIRE(document.undo().ok);
  const rat::HeightGetResult undone0 = rat::get_tile_ground_y(document.data().height_grid, 0, 0);
  const rat::HeightGetResult undone1 = rat::get_tile_ground_y(document.data().height_grid, 1, 0);
  REQUIRE(undone0.ok);
  REQUIRE(undone1.ok);
  REQUIRE(undone0.value == Approx(0.0f));
  REQUIRE(undone1.value == Approx(0.0f));
  REQUIRE_FALSE(document.can_undo());
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

TEST_CASE("Editor clean checkpoint follows authored content through undo redo and noops", "[unit][editordoc]") {
  rat::EditorDocument doc;
  doc.load(make_doc_map());
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("e", 0, 0))).changed);
  doc.mark_clean();
  REQUIRE(doc.undo());
  REQUIRE(doc.dirty());
  const auto noop = doc.execute(rat::make_remove_map_occupancy_cell_command(0, 0, 0));
  REQUIRE(noop.ok); REQUIRE_FALSE(noop.changed); REQUIRE_FALSE(noop.mutates_elevation);
  REQUIRE(doc.can_redo());
  doc.begin_stroke();
  REQUIRE(doc.execute(rat::make_place_blocker_command(make_blocker(0, 0, 1, 1))));
  REQUIRE(doc.abort_stroke());
  REQUIRE(doc.can_redo());
  REQUIRE(doc.redo());
  REQUIRE_FALSE(doc.dirty());
  const auto revision = doc.map().revision();
  REQUIRE(doc.execute(rat::make_replace_event_command(0, doc.data().events[0])).ok);
  REQUIRE(doc.map().revision() == revision);
  REQUIRE_FALSE(doc.dirty());
  REQUIRE(doc.compile_graphs_for_apply().ok);
  REQUIRE(doc.map().revision() == revision);
  REQUIRE_FALSE(doc.dirty());
}

TEST_CASE("Duplicate ids and invalid previews fail without losing authored state", "[unit][editordoc]") {
  rat::EditorDocument doc; doc.load(make_doc_map());
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("one", 0, 0))));
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("two", 1, 0))));
  doc.mark_clean();
  const auto before = rat::authoring_snapshot(doc.data());
  CHECK_FALSE(doc.execute(rat::make_place_event_command(rat::make_stub_event("one", 2, 0))).ok);
  CHECK_FALSE(doc.execute(rat::make_delete_event_command(100)).ok);
  auto renamed = doc.data().events[1]; renamed.id = "one";
  CHECK_FALSE(doc.execute(rat::make_replace_event_command(1, renamed)).ok);
  REQUIRE(doc.preview_event(1, renamed));
  CHECK_FALSE(doc.commit_preview().ok);
  CHECK(doc.preview_active());
  CHECK(rat::authoring_snapshot(doc.data()) == before);
  CHECK_FALSE(doc.dirty());
  renamed.id = "three"; REQUIRE(doc.preview_event(1, renamed));
  REQUIRE(doc.commit_preview().changed);
  CHECK_FALSE(doc.preview_active()); CHECK(doc.dirty());
}

TEST_CASE("Unsaved action settles previews and retains failed saves exactly once", "[unit][editordoc]") {
  rat::EditorDocument doc; doc.load(make_doc_map());
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("one", 0, 0))));
  doc.mark_clean();
  auto changed = doc.data().events[0]; changed.id = "edited";
  REQUIRE(doc.preview_event(0, changed));
  rat::EditorActionController c;
  int performed = 0, resets = 0, saves = 0;
  bool save_ok = false;
  c.settle = [&] { const auto r = doc.commit_preview(); return rat::EditorActionResult{r.ok, r.error}; };
  c.dirty = [&] { return doc.dirty(); };
  c.save = [&] { ++saves; if (save_ok) doc.mark_clean(); return rat::EditorActionResult{save_ok, "disk failure"}; };
  c.perform = [&](const auto& action) { CHECK(action.kind == rat::EditorActionKind::Close); ++performed; return rat::EditorActionResult{}; };
  c.reset_input = [&] { ++resets; };
  REQUIRE(c.request({rat::EditorActionKind::Close}));
  REQUIRE_FALSE(c.request({rat::EditorActionKind::LoadMap, "other"}));
  c.pump(); REQUIRE(c.awaiting_decision()); REQUIRE(doc.dirty()); CHECK(performed == 0);
  c.choose(rat::UnsavedChoice::Save); CHECK(saves == 1); CHECK(c.pending()); CHECK(performed == 0);
  save_ok = true; c.choose(rat::UnsavedChoice::Save);
  CHECK(performed == 1); CHECK(resets == 1); CHECK_FALSE(c.pending());
  c.pump(); c.choose(rat::UnsavedChoice::Discard); CHECK(performed == 1);
  REQUIRE(c.request({rat::EditorActionKind::Close})); c.pump();
  CHECK(performed == 2); CHECK(resets == 2);
}

TEST_CASE("Unsaved cancel and discard preserve their distinct document effects", "[unit][editordoc]") {
  rat::EditorDocument doc; doc.load(make_doc_map());
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("edit", 0, 0))));
  rat::EditorActionController c;
  c.dirty = [&] { return doc.dirty(); };
  c.perform = [&](const auto&) { doc.load(make_doc_map()); return rat::EditorActionResult{}; };
  REQUIRE(c.request({rat::EditorActionKind::LoadMap, "main"})); c.pump();
  c.choose(rat::UnsavedChoice::Cancel); CHECK(doc.dirty()); CHECK(doc.data().events.size() == 1);
  REQUIRE(c.request({rat::EditorActionKind::LoadMap, "main"})); c.pump();
  c.choose(rat::UnsavedChoice::Discard); CHECK_FALSE(doc.dirty()); CHECK(doc.data().events.empty());
}

TEST_CASE("Restored map retains main file clean baseline", "[unit][editordoc]") {
  rat::EditorDocument doc; auto main = make_doc_map(); doc.load(main);
  auto backup = main; backup.events.push_back(rat::make_stub_event("backup", 0, 0));
  doc.restore(backup); CHECK(doc.dirty()); CHECK_FALSE(doc.can_undo());
  doc.mark_clean(); CHECK_FALSE(doc.dirty());
  doc.restore(main); CHECK(doc.dirty());
}

TEST_CASE("Authored snapshots retain draft layout and exclude derived graph commands", "[unit][editordoc]") {
  auto map = make_doc_map(); auto event = rat::make_stub_event("e", 0, 0);
  rat::EventGraphNode node; node.id = "draft"; node.kind = "not_finished";
  event.pages[0].graph = rat::EventGraph{{node}, {}}; map.events.push_back(event);
  const auto before = rat::authoring_snapshot(map);
  map.events[0].pages[0].commands[0].text = "derived cache changed";
  CHECK(rat::authoring_snapshot(map) == before);
  rat::EditorDocument doc; doc.load(map);
  auto edited = doc.data().events[0]; edited.pages[0].graph->nodes[0].layout = {{12.5f, -8.0f}};
  REQUIRE(doc.preview_event(0, edited)); CHECK_FALSE(doc.dirty());
  REQUIRE(doc.commit_preview().changed); CHECK(doc.dirty());
  const auto revision = doc.map().revision();
  CHECK_FALSE(doc.compile_graphs_for_apply().ok);
  CHECK(doc.dirty()); CHECK(doc.map().revision() == revision);
  const auto serialized = rat::serialize_map_to_string(doc.data()); REQUIRE(serialized.ok);
  const auto loaded = rat::load_map_from_string(serialized.json_text); REQUIRE(loaded.ok);
  CHECK(loaded.map.events[0].pages[0].graph->nodes[0].layout->x == Approx(12.5f));
  REQUIRE(doc.undo()); CHECK_FALSE(doc.dirty());
  REQUIRE(doc.redo()); CHECK(doc.dirty());
  edited.pages[0].graph->nodes[0].layout->x = std::numeric_limits<float>::infinity();
  REQUIRE(doc.preview_event(0, edited)); CHECK_FALSE(doc.commit_preview().ok); CHECK(doc.preview_active());
  auto signed_zero = make_doc_map(); const auto zero = rat::authoring_snapshot(signed_zero);
  signed_zero.height_grid.ground_y[0] = -0.0f; CHECK(rat::authoring_snapshot(signed_zero) == zero);
}

TEST_CASE("Pinned backup survives guard Save rotating the on-disk backup", "[unit][editordoc][storage]") {
  rat::MemoryFileStore files;
  auto backup = make_doc_map(); backup.events.push_back(rat::make_stub_event("older", 0, 0));
  auto main = make_doc_map(); main.events.push_back(rat::make_stub_event("main", 1, 0));
  REQUIRE(rat::save_map_to_file(backup, "map", files).ok);
  REQUIRE(rat::save_map_to_file(main, "map", files).ok);
  rat::EditorDocument doc; doc.load(main);
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("unsaved", 2, 0))));
  const auto candidate = rat::load_map_from_file("map.bak", files); REQUIRE(candidate.ok);
  rat::EditorActionController c;
  c.dirty = [&] { return doc.dirty(); };
  c.save = [&] { const auto r = rat::save_map_to_file(doc.data(), "map", files); if (r.ok) doc.mark_clean(); return rat::EditorActionResult{r.ok, r.error}; };
  c.perform = [&](const auto& action) { REQUIRE(action.path == "map"); REQUIRE(action.candidate); doc.restore(*action.candidate); return rat::EditorActionResult{}; };
  REQUIRE(c.request({rat::EditorActionKind::RestoreMapBackup, "map", true, candidate.map}));
  c.pump(); c.choose(rat::UnsavedChoice::Save);
  CHECK_FALSE(c.pending()); CHECK(doc.data().events[0].id == "older"); CHECK(doc.dirty());
  const auto saved_main = rat::load_map_from_file("map", files); REQUIRE(saved_main.ok);
  CHECK(saved_main.map.events.size() == 2);
  const auto rotated_backup = rat::load_map_from_file("map.bak", files); REQUIRE(rotated_backup.ok);
  CHECK(rotated_backup.map.events[0].id == "main");
}

TEST_CASE("Malformed map action retains current document and reports failure", "[unit][editordoc][storage]") {
  rat::MemoryFileStore files; REQUIRE(files.write("bad", "{malformed").ok);
  rat::EditorDocument doc; doc.load(make_doc_map());
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("keep", 0, 0))));
  const auto before = rat::authoring_snapshot(doc.data());
  rat::EditorActionController c; c.dirty = [&] { return doc.dirty(); };
  c.perform = [&](const auto& action) { const auto loaded = rat::load_map_from_file(action.path, files); if (loaded.ok) doc.load(loaded.map); return rat::EditorActionResult{loaded.ok, loaded.error}; };
  REQUIRE(c.request({rat::EditorActionKind::LoadMap, "bad"})); c.pump(); c.choose(rat::UnsavedChoice::Discard);
  CHECK(c.awaiting_decision()); CHECK_FALSE(c.error().empty());
  CHECK(rat::authoring_snapshot(doc.data()) == before); CHECK(doc.dirty()); CHECK(doc.can_undo());
  c.choose(rat::UnsavedChoice::Cancel); CHECK_FALSE(c.pending());
}

TEST_CASE("Explicit save-slot backup applies transactionally without changing main bytes", "[unit][editordoc][storage]") {
  rat::MemoryFileStore files; rat::SimulationSession session; REQUIRE(session.load(make_doc_map()).ok);
  session.state().set_variable(3, 10); REQUIRE(rat::save_game(files, "slot", session.state()).ok);
  session.state().set_variable(3, 20); REQUIRE(rat::save_game(files, "slot", session.state()).ok);
  const std::string main(files.read("slot").bytes.as_text());
  rat::GameState loaded; REQUIRE(rat::load_game(files, "slot.bak", loaded).ok);
  REQUIRE(session.apply_loaded_game(loaded).ok); CHECK(session.state().get_variable(3) == 10);
  CHECK(files.read("slot").bytes.as_text() == main);
  REQUIRE(files.write("slot.bak", "invalid").ok);
  CHECK_FALSE(rat::load_game(files, "slot.bak", loaded).ok);
  CHECK(session.state().get_variable(3) == 10); CHECK(files.read("slot").bytes.as_text() == main);
}

TEST_CASE("Event id allocation scans reloaded content and duplicate rename retains redo", "[unit][editordoc]") {
  auto map = make_doc_map(); map.events.push_back(rat::make_stub_event("stub_1", 0, 0));
  const auto text = rat::serialize_map_to_string(map); REQUIRE(text.ok);
  const auto loaded = rat::load_map_from_string(text.json_text); REQUIRE(loaded.ok);
  rat::EditorDocument doc; doc.load(loaded.map);
  const auto id = rat::allocate_unique_event_id(doc.data(), "stub_1"); REQUIRE(id != "stub_1");
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event(id, 1, 0))));
  REQUIRE(doc.undo());
  CHECK_FALSE(doc.execute(rat::make_place_event_command(rat::make_stub_event("stub_1", 2, 0))).ok);
  CHECK(doc.can_redo()); REQUIRE(doc.redo()); CHECK(doc.data().events.size() == 2);
}

TEST_CASE("Changing selection settles the previous field and retains invalid drafts", "[unit][editordoc]") {
  auto map = make_doc_map(); map.events.push_back(rat::make_stub_event("one", 0, 0));
  map.events.push_back(rat::make_stub_event("two", 1, 0));
  rat::EditorDocument doc; doc.load(map); doc.select_event(0);
  auto event = doc.data().events[0]; event.id = "renamed";
  REQUIRE(doc.preview_event(0, event));
  doc.select_event(1); CHECK(doc.selected_event() == 1); CHECK(doc.data().events[0].id == "renamed");
  CHECK_FALSE(doc.preview_active()); CHECK(doc.dirty());
  event = doc.data().events[1]; event.id = "renamed";
  REQUIRE(doc.preview_event(1, event));
  doc.select_event(0); CHECK(doc.selected_event() == 1); CHECK(doc.preview_active());
  CHECK_FALSE(doc.execute(rat::make_delete_event_command(0)).ok);
  CHECK(doc.data().events.size() == 2);
  rat::EditorActionController c;
  c.settle = [&] { const auto r = doc.commit_preview(); return rat::EditorActionResult{r.ok, r.error}; };
  c.dirty = [&] { return doc.dirty(); };
  REQUIRE(c.request({rat::EditorActionKind::Close})); c.pump();
  CHECK(c.awaiting_decision()); CHECK_FALSE(c.error().empty());
  c.choose(rat::UnsavedChoice::Save); CHECK(c.pending()); CHECK(doc.preview_active());
  c.choose(rat::UnsavedChoice::Cancel); CHECK_FALSE(c.pending()); CHECK(doc.preview_active());
}

TEST_CASE("Real map save failure keeps the guarded load and old file bytes", "[unit][editordoc][storage]") {
  class RejectAtomic final : public rat::FileStore {
   public:
    rat::MemoryFileStore memory;
    rat::FileReadResult read(std::string_view path) const override { return memory.read(path); }
    rat::FileWriteResult write(std::string_view path, std::string_view bytes) override { return memory.write(path, bytes); }
    rat::FileWriteResult write_atomic(std::string_view, std::string_view) override { return {false, "injected atomic replace failure"}; }
  } files;
  auto main = make_doc_map(); REQUIRE(files.write("main", rat::serialize_map_to_string(main).json_text).ok);
  const std::string original(files.read("main").bytes.as_text());
  rat::EditorDocument doc; doc.load(main);
  REQUIRE(doc.execute(rat::make_place_event_command(rat::make_stub_event("edit", 0, 0))));
  rat::EditorActionController c; int loads = 0;
  c.dirty = [&] { return doc.dirty(); };
  c.save = [&] { const auto r = rat::save_map_to_file(doc.data(), "main", files); if (r.ok) doc.mark_clean(); return rat::EditorActionResult{r.ok, r.error}; };
  c.perform = [&](const auto&) { ++loads; return rat::EditorActionResult{}; };
  REQUIRE(c.request({rat::EditorActionKind::LoadMap, "main"})); c.pump(); c.choose(rat::UnsavedChoice::Save);
  CHECK(loads == 0); CHECK(c.pending()); CHECK(doc.dirty()); CHECK(doc.can_undo());
  CHECK(files.read("main").bytes.as_text() == original);
  CHECK(c.error() == "injected atomic replace failure");
  c.choose(rat::UnsavedChoice::Discard); CHECK(loads == 1); CHECK_FALSE(c.pending());
}
