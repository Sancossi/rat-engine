#include <rat/audio.hpp>
#include <rat/event_runtime.hpp>
#include <rat/game_state.hpp>
#include <rat/map_loader.hpp>
#include <rat/player.hpp>

#include <catch2/catch_test_macros.hpp>

#include <string>

namespace {

constexpr const char* kPlaySeMap = R"({
  "schema_version": 1,
  "id": "t",
  "width": 4,
  "height": 4,
  "events": [
    {
      "id": "sfx",
      "tile": { "x": 0, "z": 0 },
      "pages": [
        {
          "trigger": "autorun",
          "commands": [
            { "op": "play_se", "id": "jump" }
          ]
        }
      ]
    }
  ]
})";

}  // namespace

TEST_CASE("PlaySE enqueues PlaySfx and applies only after drain", "[unit][events][playse]") {
  const auto loaded = rat::load_map_from_string(kPlaySeMap);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.events[0].pages[0].commands[0].text == "jump");

  rat::RecordingAudioSink sink;
  rat::QueuedAudio audio(sink);
  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.set_audio(&audio);

  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE(sink.commands().empty());

  audio.drain();

  REQUIRE(sink.commands().size() == 1);
  CHECK(sink.commands()[0].kind == rat::AudioCommandKind::PlaySfx);
  CHECK(sink.commands()[0].id == "jump");
}

TEST_CASE("PlaySE without audio is a no-op and is consumed", "[unit][events][playse]") {
  const auto loaded = rat::load_map_from_string(kPlaySeMap);
  REQUIRE(loaded.ok);

  rat::GameState state;
  rat::EventRuntime runtime;
  runtime.load(loaded.map);
  runtime.update(state, rat::PlayerBody{}, false, 1.0f / 60.0f);

  REQUIRE_FALSE(runtime.player_input_blocked());
  REQUIRE_FALSE(runtime.active_message().has_value());
}

TEST_CASE("PlaySE round-trips op and id through serialize", "[unit][events][playse]") {
  const auto loaded = rat::load_map_from_string(kPlaySeMap);
  REQUIRE(loaded.ok);

  const auto serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  REQUIRE(serialized.json_text.find("\"play_se\"") != std::string::npos);
  REQUIRE(serialized.json_text.find("\"id\": \"jump\"") != std::string::npos);

  const auto again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.events.size() == 1);
  REQUIRE(again.map.events[0].pages[0].commands.size() == 1);
  REQUIRE(again.map.events[0].pages[0].commands[0].op == rat::CommandOp::PlaySE);
  REQUIRE(again.map.events[0].pages[0].commands[0].text == "jump");
}

TEST_CASE("PlaySE loader rejects empty id", "[unit][events][playse]") {
  constexpr const char* kEmptyId = R"({
    "schema_version": 1,
    "id": "t",
    "width": 4,
    "height": 4,
    "events": [
      {
        "id": "sfx",
        "tile": { "x": 0, "z": 0 },
        "pages": [
          {
            "trigger": "autorun",
            "commands": [
              { "op": "play_se", "id": "" }
            ]
          }
        ]
      }
    ]
  })";

  const auto loaded = rat::load_map_from_string(kEmptyId);
  REQUIRE_FALSE(loaded.ok);
  REQUIRE_FALSE(loaded.error.empty());
}
