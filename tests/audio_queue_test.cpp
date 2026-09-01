#include <rat/audio.hpp>
#include <rat/log.hpp>  // LogAudioSink case; FIFO / Null / nested drain use audio.hpp only

#include <catch2/catch_test_macros.hpp>

namespace {

struct NestedPlaySink : rat::AudioSink {
  rat::QueuedAudio* audio = nullptr;
  std::vector<rat::AudioCommand> applied;

  void apply(const rat::AudioCommand& command) override {
    applied.push_back(command);
    if (audio != nullptr && command.kind == rat::AudioCommandKind::PlaySfx &&
        command.id == "jump") {
      audio->play_sfx("nested");
    }
  }
};

}  // namespace

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

TEST_CASE("drain applies snapped batch only; nested play waits for next drain",
          "[unit][audio]") {
  NestedPlaySink sink;
  rat::QueuedAudio audio(sink);
  sink.audio = &audio;

  audio.play_sfx("jump");
  audio.drain();

  REQUIRE(sink.applied.size() == 1);
  CHECK(sink.applied[0].id == "jump");

  audio.drain();

  REQUIRE(sink.applied.size() == 2);
  CHECK(sink.applied[1].kind == rat::AudioCommandKind::PlaySfx);
  CHECK(sink.applied[1].id == "nested");
}

TEST_CASE("queue posts beyond capacity increment overflow and drop extras", "[unit][audio]") {
  rat::RecordingAudioSink sink;
  rat::QueuedAudio audio(sink);

  REQUIRE(audio.overflow_count() == 0);

  for (std::size_t i = 0; i < rat::kAudioQueueCapacity; ++i) {
    audio.play_sfx("ok");
  }
  REQUIRE(audio.overflow_count() == 0);

  audio.play_sfx("dropped");
  audio.play_music("dropped-music");
  audio.stop("dropped-stop");
  REQUIRE(audio.overflow_count() == 3);

  audio.drain();

  REQUIRE(sink.commands().size() == rat::kAudioQueueCapacity);
  CHECK(sink.commands().front().id == "ok");
  CHECK(sink.commands().back().id == "ok");
  CHECK(audio.overflow_count() == 3);
}

TEST_CASE("overflow stays observable after a later drain of in-cap posts", "[unit][audio]") {
  rat::RecordingAudioSink sink;
  rat::QueuedAudio audio(sink);

  for (std::size_t i = 0; i < rat::kAudioQueueCapacity + 1; ++i) {
    audio.play_sfx("a");
  }
  REQUIRE(audio.overflow_count() == 1);
  audio.drain();

  audio.play_sfx("later");
  audio.drain();

  REQUIRE(sink.commands().size() == rat::kAudioQueueCapacity + 1);
  CHECK(sink.commands().back().id == "later");
  CHECK(audio.overflow_count() == 1);
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
