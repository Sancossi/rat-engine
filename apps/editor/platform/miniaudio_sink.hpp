#pragma once

#include <rat/audio.hpp>
#include <rat/log.hpp>

#include <memory>

namespace rat {

class AssetRegistry;

class MiniaudioSink : public AudioSink {
 public:
  explicit MiniaudioSink(AssetRegistry& registry);
  ~MiniaudioSink() override;

  MiniaudioSink(const MiniaudioSink&) = delete;
  MiniaudioSink& operator=(const MiniaudioSink&) = delete;

  [[nodiscard]] bool ok() const;

  void apply(const AudioCommand& command) override;

 private:
  struct Impl;
  AssetRegistry* registry_ = nullptr;
  std::unique_ptr<Impl> impl_;
};

// Composition-root helper: device sink when init succeeds, otherwise LogAudioSink.
[[nodiscard]] std::unique_ptr<AudioSink> make_editor_audio_sink(AssetRegistry& registry,
                                                                Logger& logger);

}  // namespace rat
