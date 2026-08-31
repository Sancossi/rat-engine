#include <rat/audio.hpp>
#include <rat/log.hpp>

#include <catch2/catch_test_macros.hpp>

TEST_CASE("play commands stay queued until drain then apply FIFO", "[unit][audio]") {
  rat::RecordingAudioSink sink;
  rat::QueuedAudio audio(sink);

  audio.play_sfx("jump");
  audio.play_music("theme");
  audio.stop();

  REQUIRE(sink.commands().empty());

  audio.drain();

  REQUIRE(sink.commands().size() == 3);
  CHECK(sink.commands()[0].kind == rat::AudioCommandKind::PlaySfx);
  CHECK(sink.commands()[0].id == "jump");
  CHECK(sink.commands()[1].kind == rat::AudioCommandKind::PlayMusic);
  CHECK(sink.commands()[1].id == "theme");
  CHECK(sink.commands()[2].kind == rat::AudioCommandKind::Stop);
  CHECK(sink.commands()[2].id.empty());
}

TEST_CASE("second drain with no new posts applies nothing", "[unit][audio]") {
  rat::RecordingAudioSink sink;
  rat::QueuedAudio audio(sink);

  audio.play_sfx("jump");
  audio.drain();
  REQUIRE(sink.commands().size() == 1);

  audio.drain();
  REQUIRE(sink.commands().size() == 1);
  CHECK(sink.commands()[0].kind == rat::AudioCommandKind::PlaySfx);
  CHECK(sink.commands()[0].id == "jump");
}

TEST_CASE("NullAudioSink does not crash on drain", "[unit][audio]") {
  rat::NullAudioSink sink;
  rat::QueuedAudio audio(sink);

  audio.play_sfx("jump");
  audio.play_music("theme");
  audio.stop();
  audio.drain();
  audio.drain();
}

TEST_CASE("LogAudioSink writes kind and id on channel audio", "[unit][audio]") {
  rat::MemoryLogSink memory;
  rat::Logger logger(memory);
  rat::LogAudioSink sink(logger);
  rat::QueuedAudio audio(sink);

  audio.play_sfx("jump");
  REQUIRE(memory.lines().empty());

  audio.drain();

  REQUIRE(memory.lines().size() == 1);
  CHECK(memory.lines()[0].channel == "audio");
  CHECK(memory.lines()[0].message == "PlaySfx jump");
}
