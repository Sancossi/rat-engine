#include "rat/audio.hpp"
#include "rat/log.hpp"

#include <utility>

namespace rat {

namespace {

const char* audio_command_kind_name(AudioCommandKind kind) {
  switch (kind) {
    case AudioCommandKind::PlaySfx:
      return "PlaySfx";
    case AudioCommandKind::PlayMusic:
      return "PlayMusic";
    case AudioCommandKind::Stop:
      return "Stop";
  }
  return "PlaySfx";
}

std::string format_audio_command(const AudioCommand& command) {
  std::string message = audio_command_kind_name(command.kind);
  if (!command.id.empty()) {
    message += ' ';
    message += command.id;
  }
  return message;
}

}  // namespace

LogAudioSink::LogAudioSink(Logger& logger) : logger_(&logger) {}

void LogAudioSink::apply(const AudioCommand& command) {
  if (logger_ == nullptr) {
    return;
  }
  log(*logger_, LogLevel::Info, "audio", format_audio_command(command));
}

void RecordingAudioSink::apply(const AudioCommand& command) {
  commands_.push_back(command);
}

QueuedAudio::QueuedAudio(AudioSink& sink) : sink_(&sink) {}

void QueuedAudio::play_sfx(std::string_view id) {
  queue_.push_back(AudioCommand{AudioCommandKind::PlaySfx, std::string(id)});
}

void QueuedAudio::play_music(std::string_view id) {
  queue_.push_back(AudioCommand{AudioCommandKind::PlayMusic, std::string(id)});
}

void QueuedAudio::stop(std::string_view id) {
  queue_.push_back(AudioCommand{AudioCommandKind::Stop, std::string(id)});
}

void QueuedAudio::drain() {
  std::vector<AudioCommand> batch = std::move(queue_);
  for (const AudioCommand& command : batch) {
    if (sink_ != nullptr) {
      sink_->apply(command);
    }
  }
}

}  // namespace rat
