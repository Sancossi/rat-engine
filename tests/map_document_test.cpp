#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>
#include <rat/simulation_session.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <algorithm>
#include <string>

using Catch::Approx;

namespace {

rat::MapData make_flat_document_map() {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "doc_flat";
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

[[nodiscard]] const rat::MapIssue* find_issue_with_path(const std::vector<rat::MapIssue>& issues,
                                                        std::string_view path) {
  for (const rat::MapIssue& issue : issues) {
    if (issue.json_path == path) {
      return &issue;
    }
  }
  return nullptr;
}

[[nodiscard]] bool has_error_at(const std::vector<rat::MapIssue>& issues, std::string_view path) {
  const rat::MapIssue* issue = find_issue_with_path(issues, path);
  return issue != nullptr && issue->severity == rat::MapIssueSeverity::Error &&
         !issue->message.empty();
}

}  // namespace

TEST_CASE("v1 JSON migrates to MapDocument and compiles RuntimeMap", "[unit][mapdoc]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "legacy_doc",
    "width": 3,
    "height": 2,
    "events": [
      {
        "id": "talk",
        "tile": { "x": 1, "z": 0 },
        "pages": [
          { "trigger": "action", "commands": [{ "op": "show_text", "text": "Hi" }] }
        ]
      }
    ]
  })";

  const rat::MapDocumentLoadResult loaded = rat::load_map_document_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.document.data().id == "legacy_doc");
  REQUIRE(loaded.document.data().schema_version == 1);
  REQUIRE(loaded.document.data().height_grid.width == 3);
  REQUIRE(loaded.document.data().height_grid.height == 2);
  REQUIRE(loaded.document.data().height_grid.ground_y.size() == 6);
  REQUIRE(loaded.document.revision() > 0);

  const rat::MapCompileResult compiled = rat::compile_map_document(loaded.document);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.runtime.source_revision == loaded.document.revision());
  REQUIRE(compiled.runtime.data.id == "legacy_doc");
  REQUIRE(compiled.runtime.data.events.size() == 1);
  REQUIRE(compiled.runtime.data.events[0].id == "talk");
}

TEST_CASE("duplicate event ids fail semantic validation with JSON path", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  map.events.push_back(rat::make_stub_event("npc", 1, 0));

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/events/1/id"));
  REQUIRE_FALSE(rat::compile_map_data(map).ok);
}

TEST_CASE("height grid length mismatch reports /height_grid/ground_y", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.height_grid.ground_y.pop_back();

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/height_grid/ground_y"));
}

TEST_CASE("ramp high_y below low_y reports /ramps/0/high_y", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  rat::RampDef ramp;
  ramp.tile = {1, 1};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 2.0f;
  ramp.high_y = 1.0f;
  map.ramps.push_back(ramp);

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/ramps/0/high_y"));
}

TEST_CASE("empty play_se id reports command JSON path", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  rat::EventDef event = rat::make_stub_event("se_event", 0, 0);
  rat::Command play_se;
  play_se.op = rat::CommandOp::PlaySE;
  play_se.text = "";
  event.pages[0].commands = {play_se};
  map.events.push_back(std::move(event));

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/events/0/pages/0/commands/0"));
}

TEST_CASE("jumpable blocker without vertical pair reports blocker path", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  rat::BlockerDef blocker;
  blocker.bounds = {0.0f, 0.0f, 1.0f, 1.0f};
  blocker.jumpable = true;
  map.blockers.push_back(blocker);

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/blockers/0"));
}

TEST_CASE("schema error from JSON is structured with path", "[unit][mapdoc]") {
  const rat::MapDocumentLoadResult loaded =
      rat::load_map_document_from_string(R"({"schema_version":6,"id":"x","width":1,"height":1})");
  REQUIRE_FALSE(loaded.ok);
  REQUIRE(has_error_at(loaded.issues, "/schema_version"));
}

TEST_CASE("schema 4 indoor volume with y_hi < y_lo reports /indoor_volumes/0/y_hi",
          "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 4;
  map.indoor_volumes.push_back(rat::IndoorVolume{
      .xz = {0.0f, 0.0f, 2.0f, 2.0f},
      .y_lo = 2.0f,
      .y_hi = 1.0f,
  });
  REQUIRE(has_error_at(rat::validate_map_document(map), "/indoor_volumes/0/y_hi"));
}

TEST_CASE("schema 4 empty indoor_volumes compiles", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 4;
  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);
}

TEST_CASE("schema 5 empty occupancy compiles", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 5;
  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);
}

TEST_CASE("schema 5 occupancy unknown kind from JSON is structured with path", "[unit][mapdoc]") {
  const rat::MapDocumentLoadResult loaded = rat::load_map_document_from_string(R"({
    "schema_version": 5,
    "id": "bad_occ",
    "width": 1,
    "height": 1,
    "height_grid": {
      "origin_x": 0, "origin_z": 0, "width": 1, "height": 1, "ground_y": [0.0]
    },
    "occupancy": [ { "x": 0, "y": 0, "z": 0, "kind": "glass" } ]
  })");
  REQUIRE_FALSE(loaded.ok);
  REQUIRE(has_error_at(loaded.issues, "/occupancy/0/kind"));
}

TEST_CASE("document replace bumps revision and invalidates prior RuntimeMap stamp",
          "[unit][mapdoc]") {
  rat::MapDocument document(make_flat_document_map());
  const std::uint64_t first_revision = document.revision();
  const rat::MapCompileResult first = rat::compile_map_document(document);
  REQUIRE(first.ok);
  REQUIRE(first.runtime.source_revision == first_revision);

  rat::MapData next = document.data();
  next.events.push_back(rat::make_stub_event("npc", 0, 0));
  document.replace(std::move(next));
  REQUIRE(document.revision() != first_revision);

  const rat::MapCompileResult second = rat::compile_map_document(document);
  REQUIRE(second.ok);
  REQUIRE(second.runtime.source_revision == document.revision());
  REQUIRE(second.runtime.source_revision != first.runtime.source_revision);
}

TEST_CASE("EventRuntime tick does not mutate compiled map", "[unit][mapdoc][events]") {
  rat::MapData map = make_flat_document_map();
  rat::EventDef intro = rat::make_stub_event("intro", 1, 1);
  intro.pages[0].trigger = rat::TriggerKind::Autorun;
  map.events.push_back(std::move(intro));

  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);

  rat::EventRuntime runtime;
  runtime.load(compiled.runtime);
  const rat::MapSerializeResult before = rat::serialize_map_to_string(runtime.map());
  REQUIRE(before.ok);

  rat::GameState state;
  rat::PlayerBody player;
  player.x = 1.5f;
  player.y = 0.0f;
  player.z = 1.5f;
  runtime.update(state, player, true, 1.0f / 60.0f);
  REQUIRE(runtime.active_message() == "New event");

  const rat::MapSerializeResult after = rat::serialize_map_to_string(runtime.map());
  REQUIRE(after.ok);
  REQUIRE(after.json_text == before.json_text);
  REQUIRE(runtime.runtime_map().source_revision == compiled.runtime.source_revision);
}

TEST_CASE("broken map does not start SimulationSession", "[unit][mapdoc][sim]") {
  rat::MapData good = make_flat_document_map();
  rat::SimulationSession session;
  const rat::SimulationLoadResult loaded_good = session.load(good);
  REQUIRE(loaded_good.ok);
  REQUIRE(session.state().map_id() == "doc_flat");

  rat::InputFrame frame;
  session.tick(frame);
  REQUIRE(session.tick_id() == 1);

  rat::MapData broken = good;
  broken.events.push_back(rat::make_stub_event("dup", 0, 0));
  broken.events.push_back(rat::make_stub_event("dup", 1, 0));
  const rat::SimulationLoadResult loaded_broken = session.load(broken);
  REQUIRE_FALSE(loaded_broken.ok);
  REQUIRE(has_error_at(loaded_broken.issues, "/events/1/id"));
  REQUIRE(session.tick_id() == 1);
  REQUIRE(session.state().map_id() == "doc_flat");
  REQUIRE(session.events().map().events.empty());
}

TEST_CASE("undo redo restores semantically identical MapDocument", "[unit][mapdoc][edit]") {
  rat::MapDocument document(make_flat_document_map());
  const rat::MapSerializeResult original = rat::serialize_map_to_string(document.data());
  REQUIRE(original.ok);

  rat::EditHistory history;
  rat::MapData working = document.data();
  rat::BlockerDef blocker;
  blocker.bounds = {0.0f, 0.0f, 1.0f, 1.0f};
  REQUIRE(history.execute(working, rat::make_place_blocker_command(blocker)));
  document.replace(working);
  REQUIRE_FALSE(document.data().blockers.empty());

  REQUIRE(history.undo(working));
  document.replace(working);
  const rat::MapSerializeResult undone = rat::serialize_map_to_string(document.data());
  REQUIRE(undone.ok);
  REQUIRE(undone.json_text == original.json_text);

  REQUIRE(history.redo(working));
  document.replace(working);
  REQUIRE(history.undo(working));
  document.replace(working);
  const rat::MapSerializeResult undone_again = rat::serialize_map_to_string(document.data());
  REQUIRE(undone_again.ok);
  REQUIRE(undone_again.json_text == original.json_text);
}

TEST_CASE("MapDocument save/load round-trip is semantically identical", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.events.push_back(rat::make_stub_event("npc", 2, 1));
  rat::RampDef ramp;
  ramp.tile = {1, 0};
  ramp.direction = rat::RampDirection::East;
  ramp.low_y = 0.0f;
  ramp.high_y = 1.0f;
  map.ramps.push_back(ramp);
  rat::EdgeBarrierDef edge;
  edge.tile = {0, 1};
  edge.direction = rat::RampDirection::North;
  edge.height = 0.45f;
  map.edge_barriers.push_back(edge);
  rat::BlockerDef blocker;
  blocker.bounds = {2.0f, 2.0f, 3.0f, 3.0f};
  map.blockers.push_back(blocker);

  rat::MapDocument document(std::move(map));
  REQUIRE(rat::compile_map_document(document).ok);
  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(document.data());
  REQUIRE(serialized.ok);

  const rat::MapDocumentLoadResult reloaded =
      rat::load_map_document_from_string(serialized.json_text);
  REQUIRE(reloaded.ok);
  const rat::MapSerializeResult again = rat::serialize_map_to_string(reloaded.document.data());
  REQUIRE(again.ok);

  const rat::MapDocumentLoadResult reloaded_twice =
      rat::load_map_document_from_string(again.json_text);
  REQUIRE(reloaded_twice.ok);
  const rat::MapSerializeResult third = rat::serialize_map_to_string(reloaded_twice.document.data());
  REQUIRE(third.ok);
  REQUIRE(third.json_text == again.json_text);
}

TEST_CASE("schema 1 missing height grid still compiles after v1 fallback", "[unit][mapdoc]") {
  rat::MapData map;
  map.schema_version = 1;
  map.id = "v1_no_grid";
  map.width = 2;
  map.height = 2;
  map.tile_size = 1.0f;

  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE(compiled.ok);
  REQUIRE(compiled.runtime.data.height_grid.width == 2);
  REQUIRE(compiled.runtime.data.height_grid.height == 2);
  REQUIRE(compiled.runtime.data.height_grid.ground_y.size() == 4);
}

TEST_CASE("schema 2 missing height grid fails compile the same as validate", "[unit][mapdoc]") {
  rat::MapData map;
  map.schema_version = 2;
  map.id = "v2_no_grid";
  map.width = 3;
  map.height = 2;
  map.tile_size = 1.0f;

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/height_grid/width"));

  const rat::MapCompileResult compiled = rat::compile_map_data(map);
  REQUIRE_FALSE(compiled.ok);
  REQUIRE(has_error_at(compiled.issues, "/height_grid/width"));
}

TEST_CASE("two stacked slabs on one tile are valid; overlapping Y is an error", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.25f});
  map.floor_slabs.push_back({{0, 0}, 4.0f, 0.25f});
  REQUIRE_FALSE(rat::map_issues_have_errors(rat::validate_map_document(map)));

  map.floor_slabs[1].top_y = 2.1f;
  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(has_error_at(issues, "/floor_slabs/1/top_y"));
}

TEST_CASE("ladder with y_hi <= y_lo reports /ladders/0/y_hi", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.ladders.push_back({{0, 0}, rat::RampDirection::East, 0.0f, 0.0f});
  REQUIRE(has_error_at(rat::validate_map_document(map), "/ladders/0/y_hi"));
}

TEST_CASE("slab thickness <= 0 reports /floor_slabs/0/thickness", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.floor_slabs.push_back({{0, 0}, 2.0f, 0.0f});
  REQUIRE(has_error_at(rat::validate_map_document(map), "/floor_slabs/0/thickness"));
}

TEST_CASE("slab tile outside height grid reports /floor_slabs/0/tile", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.floor_slabs.push_back({{9, 0}, 2.0f, 0.25f});
  REQUIRE(has_error_at(rat::validate_map_document(map), "/floor_slabs/0/tile"));
}

TEST_CASE("ladder tile outside height grid reports /ladders/0/tile", "[unit][mapdoc]") {
  rat::MapData map = make_flat_document_map();
  map.schema_version = 3;
  map.ladders.push_back({{9, 0}, rat::RampDirection::East, 0.0f, 1.6f});
  REQUIRE(has_error_at(rat::validate_map_document(map), "/ladders/0/tile"));
}

TEST_CASE("EventRuntime load of MapData reports compile failure and keeps previous map",
          "[unit][mapdoc][events]") {
  rat::MapData good = make_flat_document_map();
  good.events.push_back(rat::make_stub_event("keep", 0, 0));
  rat::EventRuntime runtime;
  const rat::MapCompileResult loaded_good = runtime.load(good);
  REQUIRE(loaded_good.ok);
  REQUIRE(runtime.map().id == "doc_flat");
  REQUIRE(runtime.map().events.size() == 1);

  rat::MapData broken = good;
  broken.events.push_back(rat::make_stub_event("keep", 1, 0));
  const rat::MapCompileResult loaded_broken = runtime.load(broken);
  REQUIRE_FALSE(loaded_broken.ok);
  REQUIRE(has_error_at(loaded_broken.issues, "/events/1/id"));
  REQUIRE(runtime.map().id == "doc_flat");
  REQUIRE(runtime.map().events.size() == 1);
  REQUIRE(runtime.map().events[0].id == "keep");
}
