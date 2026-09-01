#pragma once

#include <cstddef>
#include <cstdint>
#include <string>
#include <string_view>
#include <vector>

namespace rat {

class Logger;

inline constexpr std::size_t kAudioQueueCapacity = 64;

enum class AudioCommandKind { PlaySfx, PlayMusic, Stop };

struct AudioCommand {
  AudioCommandKind kind = AudioCommandKind::PlaySfx;
  std::string id;  // cue/asset id; empty id on Stop = stop all
};

class AudioSink {
 public:
  virtual ~AudioSink() = default;
  virtual void apply(const AudioCommand& command) = 0;
};

class NullAudioSink : public AudioSink {
 public:
  void apply(const AudioCommand&) override {}
};

class LogAudioSink : public AudioSink {
 public:
  explicit LogAudioSink(Logger& logger);
  void apply(const AudioCommand& command) override;

 private:
  Logger* logger_ = nullptr;
};

class RecordingAudioSink : public AudioSink {
 public:
  void apply(const AudioCommand& command) override;
  [[nodiscard]] const std::vector<AudioCommand>& commands() const { return commands_; }

 private:
  std::vector<AudioCommand> commands_;
};

class Audio {
 public:
  virtual ~Audio() = default;
  virtual void play_sfx(std::string_view id) = 0;
  virtual void play_music(std::string_view id) = 0;
  virtual void stop(std::string_view id = {}) = 0;
  virtual void drain() = 0;
};

class QueuedAudio : public Audio {
 public:
  explicit QueuedAudio(AudioSink& sink);
  void play_sfx(std::string_view id) override;
  void play_music(std::string_view id) override;
  void stop(std::string_view id = {}) override;
  // Posts from apply() stay on queue_ for the next drain(); this call does not loop until empty.
  void drain() override;
  // Cumulative dropped posts; not reset by drain() so metrics can read it later.
  [[nodiscard]] std::uint64_t overflow_count() const { return overflow_count_; }

 private:
  void enqueue(AudioCommand command);

  AudioSink* sink_ = nullptr;
  std::vector<AudioCommand> queue_;
  std::uint64_t overflow_count_ = 0;
};

}  // namespace rat
