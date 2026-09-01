#define MINIAUDIO_IMPLEMENTATION
#include "miniaudio.h"

#include "miniaudio_sink.hpp"

#include <rat/asset.hpp>

#include <array>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <unordered_set>
#include <vector>

namespace rat {
namespace {

constexpr std::size_t kMaxVoices = 16;

}  // namespace

struct MiniaudioSink::Impl {
  ma_engine engine{};
  bool engine_ok = false;
  std::array<ma_sound, kMaxVoices> voices{};
  std::array<bool, kMaxVoices> live{};
  std::array<std::string, kMaxVoices> ids{};
  std::unordered_set<std::string> registered;
  std::unordered_map<std::string, std::vector<std::uint8_t>> encoded;
};

MiniaudioSink::MiniaudioSink(AssetRegistry& registry) : registry_(&registry) {
  impl_ = std::make_unique<Impl>();
  if (ma_engine_init(nullptr, &impl_->engine) != MA_SUCCESS) {
    impl_.reset();
    return;
  }
  impl_->engine_ok = true;
}

MiniaudioSink::~MiniaudioSink() {
  if (impl_ == nullptr || !impl_->engine_ok) {
    return;
  }
  for (std::size_t i = 0; i < kMaxVoices; ++i) {
    if (impl_->live[i]) {
      ma_sound_uninit(&impl_->voices[i]);
      impl_->live[i] = false;
    }
  }
  ma_engine_uninit(&impl_->engine);
  impl_->engine_ok = false;
}

bool MiniaudioSink::ok() const {
  return impl_ != nullptr && impl_->engine_ok;
}

void MiniaudioSink::apply(const AudioCommand& command) {
  if (!ok() || registry_ == nullptr) {
    return;
  }

  auto retire_finished = [this] {
    for (std::size_t i = 0; i < kMaxVoices; ++i) {
      if (!impl_->live[i]) {
        continue;
      }
      if (!ma_sound_is_playing(&impl_->voices[i])) {
        ma_sound_uninit(&impl_->voices[i]);
        impl_->live[i] = false;
        impl_->ids[i].clear();
      }
    }
  };

  auto stop_matching = [this](std::string_view id) {
    for (std::size_t i = 0; i < kMaxVoices; ++i) {
      if (!impl_->live[i]) {
        continue;
      }
      if (!id.empty() && impl_->ids[i] != id) {
        continue;
      }
      ma_sound_stop(&impl_->voices[i]);
      ma_sound_uninit(&impl_->voices[i]);
      impl_->live[i] = false;
      impl_->ids[i].clear();
    }
  };

  if (command.kind == AudioCommandKind::Stop) {
    stop_matching(command.id);
    return;
  }

  retire_finished();

  const AssetId asset_id = make_asset_id(command.id);
  const AssetView view = registry_->resolve(asset_id);
  const std::string compiled = registry_->compiled_path(asset_id);
  const bool have_cpu = view.state == AssetState::Ready && !view.cpu.bytes.empty();
  if (!have_cpu && compiled.empty()) {
    return;
  }

  std::size_t slot = kMaxVoices;
  for (std::size_t i = 0; i < kMaxVoices; ++i) {
    if (!impl_->live[i]) {
      slot = i;
      break;
    }
  }
  if (slot == kMaxVoices) {
    ma_sound_stop(&impl_->voices[0]);
    ma_sound_uninit(&impl_->voices[0]);
    impl_->live[0] = false;
    impl_->ids[0].clear();
    slot = 0;
  }

  const char* play_name = nullptr;
  std::string compiled_storage;
  if (have_cpu) {
    if (impl_->registered.find(command.id) == impl_->registered.end()) {
      ma_resource_manager* manager = ma_engine_get_resource_manager(&impl_->engine);
      if (manager == nullptr) {
        return;
      }
      std::vector<std::uint8_t>& keep = impl_->encoded[command.id];
      keep = view.cpu.bytes;
      const ma_result registered = ma_resource_manager_register_encoded_data(
          manager, command.id.c_str(), keep.data(), keep.size());
      if (registered != MA_SUCCESS) {
        impl_->encoded.erase(command.id);
        return;
      }
      impl_->registered.insert(command.id);
    }
    play_name = command.id.c_str();
  } else {
    compiled_storage = compiled;
    play_name = compiled_storage.c_str();
  }

  ma_uint32 flags = 0;
  if (command.kind == AudioCommandKind::PlayMusic) {
    flags |= MA_SOUND_FLAG_LOOPING;
  }
  if (ma_sound_init_from_file(&impl_->engine, play_name, flags, nullptr, nullptr,
                              &impl_->voices[slot]) != MA_SUCCESS) {
    return;
  }
  if (ma_sound_start(&impl_->voices[slot]) != MA_SUCCESS) {
    ma_sound_uninit(&impl_->voices[slot]);
    return;
  }
  impl_->live[slot] = true;
  impl_->ids[slot] = command.id;
}

std::unique_ptr<AudioSink> make_editor_audio_sink(AssetRegistry& registry, Logger& logger) {
  auto device = std::make_unique<MiniaudioSink>(registry);
  if (device->ok()) {
    return device;
  }
  log(logger, LogLevel::Warn, "audio", "miniaudio device init failed; using log sink");
  return std::make_unique<LogAudioSink>(logger);
}

}  // namespace rat
