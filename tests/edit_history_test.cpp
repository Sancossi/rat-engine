#include <rat/blocker_edit.hpp>
#include <rat/edit_history.hpp>
#include <rat/event_edit.hpp>
#include <rat/map_data.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

using Catch::Approx;

namespace {

rat::MapData make_tiny_map() {
  rat::MapData map;
  map.id = "undo";
  map.width = 4;
  map.height = 4;
  map.tile_size = 1.0f;
  return map;
}

rat::BlockerDef make_blocker(float min_x, float min_z, float max_x, float max_z) {
  rat::BlockerDef blocker;
  blocker.bounds = {min_x, min_z, max_x, max_z};
  return blocker;
}

}  // namespace

TEST_CASE("place blocker undo removes it and redo restores AABB", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  rat::EditHistory history;
  const rat::BlockerDef blocker = make_blocker(0.0f, 0.0f, 1.0f, 1.0f);

  history.execute(map, rat::make_place_blocker_command(blocker));
  REQUIRE(map.blockers.size() == 1);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(1.0f));
  REQUIRE(history.can_undo());
  REQUIRE_FALSE(history.can_redo());

  REQUIRE(history.undo(map));
  REQUIRE(map.blockers.empty());
  REQUIRE_FALSE(history.can_undo());
  REQUIRE(history.can_redo());

  REQUIRE(history.redo(map));
  REQUIRE(map.blockers.size() == 1);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.min_z == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(1.0f));
  REQUIRE(map.blockers[0].bounds.max_z == Approx(1.0f));
}

TEST_CASE("move blocker by one tile undo restores AABB and redo moves again", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.blockers.push_back(make_blocker(2.0f, -1.0f, 4.0f, 1.0f));
  rat::EditHistory history;

  history.execute(map, rat::make_move_blocker_command(0, 1, 0, map.tile_size));
  REQUIRE(map.blockers[0].bounds.min_x == Approx(3.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(5.0f));
  REQUIRE(map.blockers[0].bounds.min_z == Approx(-1.0f));
  REQUIRE(map.blockers[0].bounds.max_z == Approx(1.0f));

  REQUIRE(history.undo(map));
  REQUIRE(map.blockers[0].bounds.min_x == Approx(2.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(4.0f));
  REQUIRE(map.blockers[0].bounds.min_z == Approx(-1.0f));
  REQUIRE(map.blockers[0].bounds.max_z == Approx(1.0f));

  REQUIRE(history.redo(map));
  REQUIRE(map.blockers[0].bounds.min_x == Approx(3.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(5.0f));
}

TEST_CASE("delete event undo restores it at the same index and redo deletes", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.events.push_back(rat::make_stub_event("a", 0, 0));
  map.events.push_back(rat::make_stub_event("b", 1, 0));
  map.events.push_back(rat::make_stub_event("c", 2, 0));
  rat::EditHistory history;

  history.execute(map, rat::make_delete_event_command(1));
  REQUIRE(map.events.size() == 2);
  REQUIRE(map.events[0].id == "a");
  REQUIRE(map.events[1].id == "c");

  REQUIRE(history.undo(map));
  REQUIRE(map.events.size() == 3);
  REQUIRE(map.events[0].id == "a");
  REQUIRE(map.events[1].id == "b");
  REQUIRE(map.events[1].tile.has_value());
  REQUIRE(map.events[1].tile->x == 1);
  REQUIRE(map.events[1].tile->z == 0);
  REQUIRE(map.events[2].id == "c");

  REQUIRE(history.redo(map));
  REQUIRE(map.events.size() == 2);
  REQUIRE(map.events[0].id == "a");
  REQUIRE(map.events[1].id == "c");
}

TEST_CASE("clear after execute makes undo false even with prior history", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  rat::EditHistory history;
  history.execute(map, rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f)));
  REQUIRE(history.can_undo());

  history.clear();
  REQUIRE_FALSE(history.can_undo());
  REQUIRE_FALSE(history.can_redo());
  REQUIRE_FALSE(history.undo(map));
  REQUIRE(map.blockers.size() == 1);
}

TEST_CASE("new execute after undo clears the redo stack", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  rat::EditHistory history;
  history.execute(map, rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f)));
  REQUIRE(history.undo(map));
  REQUIRE(history.can_redo());
  REQUIRE(map.blockers.empty());

  history.execute(map, rat::make_place_blocker_command(make_blocker(5.0f, 5.0f, 6.0f, 6.0f)));
  REQUIRE_FALSE(history.can_redo());
  REQUIRE_FALSE(history.redo(map));
  REQUIRE(map.blockers.size() == 1);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(5.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(6.0f));
}

TEST_CASE("place event and delete blocker round-trip ids and AABB", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  map.blockers.push_back(make_blocker(2.0f, 0.0f, 3.0f, 1.0f));
  rat::EditHistory history;

  history.execute(map, rat::make_place_event_command(rat::make_stub_event("npc", 3, -1)));
  REQUIRE(map.events.size() == 1);
  REQUIRE(map.events[0].id == "npc");
  REQUIRE(history.undo(map));
  REQUIRE(map.events.empty());
  REQUIRE(history.redo(map));
  REQUIRE(map.events[0].id == "npc");
  REQUIRE(map.events[0].tile->x == 3);

  history.execute(map, rat::make_delete_blocker_command(0));
  REQUIRE(map.blockers.size() == 1);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(2.0f));
  REQUIRE(history.undo(map));
  REQUIRE(map.blockers.size() == 2);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(0.0f));
  REQUIRE(map.blockers[1].bounds.min_x == Approx(2.0f));
}

TEST_CASE("blocker commands mutate blockers only; event commands mutate events only",
          "[unit][edit]") {
  const auto place_blocker = rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  REQUIRE(place_blocker->mutates_blockers());
  REQUIRE_FALSE(place_blocker->mutates_events());

  const auto delete_blocker = rat::make_delete_blocker_command(0);
  REQUIRE(delete_blocker->mutates_blockers());
  REQUIRE_FALSE(delete_blocker->mutates_events());

  const auto move_blocker = rat::make_move_blocker_command(0, 1, 0, 1.0f);
  REQUIRE(move_blocker->mutates_blockers());
  REQUIRE_FALSE(move_blocker->mutates_events());

  const auto place_event = rat::make_place_event_command(rat::make_stub_event("e", 0, 0));
  REQUIRE_FALSE(place_event->mutates_blockers());
  REQUIRE(place_event->mutates_events());

  const auto delete_event = rat::make_delete_event_command(0);
  REQUIRE_FALSE(delete_event->mutates_blockers());
  REQUIRE(delete_event->mutates_events());

  const auto move_event = rat::make_move_event_command(0, 1, 0, 1.0f);
  REQUIRE_FALSE(move_event->mutates_blockers());
  REQUIRE(move_event->mutates_events());

  const auto replace_blocker = rat::make_replace_blocker_command(0, make_blocker(0.0f, 0.0f, 2.0f, 1.0f));
  REQUIRE(replace_blocker->mutates_blockers());
  REQUIRE_FALSE(replace_blocker->mutates_events());

  const auto replace_event = rat::make_replace_event_command(0, rat::make_stub_event("e", 0, 0));
  REQUIRE_FALSE(replace_event->mutates_blockers());
  REQUIRE(replace_event->mutates_events());
}

TEST_CASE("execute undo redo of a blocker command report blockers only", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  rat::EditHistory history;

  const rat::EditApplyResult executed =
      history.execute(map, rat::make_place_blocker_command(make_blocker(0.0f, 0.0f, 1.0f, 1.0f)));
  REQUIRE(executed.applied);
  REQUIRE(executed.mutates_blockers);
  REQUIRE_FALSE(executed.mutates_events);

  const rat::EditApplyResult undone = history.undo(map);
  REQUIRE(undone.applied);
  REQUIRE(undone.mutates_blockers);
  REQUIRE_FALSE(undone.mutates_events);

  const rat::EditApplyResult redone = history.redo(map);
  REQUIRE(redone.applied);
  REQUIRE(redone.mutates_blockers);
  REQUIRE_FALSE(redone.mutates_events);
}

TEST_CASE("execute undo redo of an event command report events only", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.events.push_back(rat::make_stub_event("npc", 1, 2));
  rat::EditHistory history;

  const rat::EditApplyResult executed =
      history.execute(map, rat::make_move_event_command(0, 1, 0, map.tile_size));
  REQUIRE(executed.applied);
  REQUIRE_FALSE(executed.mutates_blockers);
  REQUIRE(executed.mutates_events);

  const rat::EditApplyResult undone = history.undo(map);
  REQUIRE(undone.applied);
  REQUIRE_FALSE(undone.mutates_blockers);
  REQUIRE(undone.mutates_events);

  const rat::EditApplyResult redone = history.redo(map);
  REQUIRE(redone.applied);
  REQUIRE_FALSE(redone.mutates_blockers);
  REQUIRE(redone.mutates_events);
}

TEST_CASE("move event by one tile undo restores tile", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.events.push_back(rat::make_stub_event("npc", 1, 2));
  rat::EditHistory history;

  history.execute(map, rat::make_move_event_command(0, 0, -1, map.tile_size));
  REQUIRE(map.events[0].tile->x == 1);
  REQUIRE(map.events[0].tile->z == 1);

  REQUIRE(history.undo(map));
  REQUIRE(map.events[0].tile->x == 1);
  REQUIRE(map.events[0].tile->z == 2);

  REQUIRE(history.redo(map));
  REQUIRE(map.events[0].id == "npc");
  REQUIRE(map.events[0].tile->z == 1);
}

TEST_CASE("replace blocker AABB undo restores previous box and redo grows again", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  rat::BlockerDef grown = map.blockers[0];
  grown.bounds = rat::resize_aabb_on_grid(grown.bounds, rat::AabbEdge::MaxX, 1, map.tile_size);
  rat::EditHistory history;

  const rat::EditApplyResult executed =
      history.execute(map, rat::make_replace_blocker_command(0, grown));
  REQUIRE(executed.applied);
  REQUIRE(executed.mutates_blockers);
  REQUIRE_FALSE(executed.mutates_events);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.min_z == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(2.0f));
  REQUIRE(map.blockers[0].bounds.max_z == Approx(1.0f));

  const rat::EditApplyResult undone = history.undo(map);
  REQUIRE(undone.applied);
  REQUIRE(undone.mutates_blockers);
  REQUIRE_FALSE(undone.mutates_events);
  REQUIRE(map.blockers[0].bounds.min_x == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.min_z == Approx(0.0f));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(1.0f));
  REQUIRE(map.blockers[0].bounds.max_z == Approx(1.0f));

  REQUIRE(history.redo(map));
  REQUIRE(map.blockers[0].bounds.max_x == Approx(2.0f));
  REQUIRE(map.blockers[0].bounds.max_z == Approx(1.0f));
}

TEST_CASE("replace blocker jumpable vertical pair undo restores jumpable and range",
          "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.blockers.push_back(make_blocker(0.0f, 0.0f, 1.0f, 1.0f));
  rat::BlockerDef jumpable = map.blockers[0];
  REQUIRE(rat::set_blocker_vertical_range(jumpable, true, 0.25f, 1.25f));
  rat::EditHistory history;

  history.execute(map, rat::make_replace_blocker_command(0, jumpable));
  REQUIRE(map.blockers[0].jumpable);
  REQUIRE(map.blockers[0].base_y.has_value());
  REQUIRE(map.blockers[0].top_y.has_value());
  REQUIRE(*map.blockers[0].base_y == Approx(0.25f));
  REQUIRE(*map.blockers[0].top_y == Approx(1.25f));

  REQUIRE(history.undo(map));
  REQUIRE_FALSE(map.blockers[0].jumpable);
  REQUIRE_FALSE(map.blockers[0].base_y.has_value());
  REQUIRE_FALSE(map.blockers[0].top_y.has_value());

  REQUIRE(history.redo(map));
  REQUIRE(map.blockers[0].jumpable);
  REQUIRE(*map.blockers[0].base_y == Approx(0.25f));
  REQUIRE(*map.blockers[0].top_y == Approx(1.25f));
}

TEST_CASE("replace event Show Text undo restores the old string", "[unit][edit]") {
  rat::MapData map = make_tiny_map();
  map.events.push_back(rat::make_stub_event("npc", 0, 0));
  REQUIRE(map.events[0].pages[0].commands[0].text == "New event");
  rat::EventDef edited = map.events[0];
  edited.pages[0].commands[0].text = "Hello";
  rat::EditHistory history;

  const rat::EditApplyResult executed =
      history.execute(map, rat::make_replace_event_command(0, edited));
  REQUIRE(executed.applied);
  REQUIRE_FALSE(executed.mutates_blockers);
  REQUIRE(executed.mutates_events);
  REQUIRE(map.events[0].pages[0].commands[0].text == "Hello");

  REQUIRE(history.undo(map));
  REQUIRE(map.events[0].pages[0].commands[0].text == "New event");
  REQUIRE(map.events[0].id == "npc");

  REQUIRE(history.redo(map));
  REQUIRE(map.events[0].pages[0].commands[0].text == "Hello");
}
