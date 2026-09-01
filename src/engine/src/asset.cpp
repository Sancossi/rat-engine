#include "rat/asset.hpp"

#include "rat/file_store.hpp"
#include "rat/map_data.hpp"

#include <utility>

namespace rat {
namespace {

[[nodiscard]] AssetView failed_unknown(const AssetId& id) {
  AssetView view;
  view.id = id;
  view.resolved_id = id;
  view.state = AssetState::Failed;
  view.error = "unknown asset";
  return view;
}

}  // namespace

void MemoryAssetLoader::set(const AssetId& id, AssetCpuData cpu) {
  Slot& slot = slots_[id.key()];
  slot.fail = false;
  slot.error.clear();
  slot.cpu = std::move(cpu);
}

void MemoryAssetLoader::set_fail(const AssetId& id, std::string error) {
  Slot& slot = slots_[id.key()];
  slot.fail = true;
  slot.error = std::move(error);
  slot.cpu = {};
}

AssetLoader::Result MemoryAssetLoader::load(const AssetId& id, AssetKind, const AssetPath&) {
  Result result;
  const auto it = slots_.find(id.key());
  if (it == slots_.end()) {
    result.ok = false;
    result.error = "asset not in memory catalog";
    return result;
  }
  if (it->second.fail) {
    result.ok = false;
    result.error = it->second.error.empty() ? "asset load failed" : it->second.error;
    return result;
  }
  result.ok = true;
  result.cpu = it->second.cpu;
  return result;
}

FileAssetLoader::FileAssetLoader(FileStore& files) : files_(&files) {}

AssetLoader::Result FileAssetLoader::load(const AssetId&, AssetKind, const AssetPath& path) {
  Result result;
  const std::string& file_path = !path.compiled.empty() ? path.compiled : path.source;
  if (file_path.empty()) {
    result.error = "asset has no compiled or source path";
    return result;
  }
  if (files_ == nullptr) {
    result.error = "asset file store is missing";
    return result;
  }
  const FileReadResult read = files_->read(file_path);
  if (!read.ok) {
    result.error = read.error.empty() ? "asset file read failed" : read.error;
    return result;
  }
  result.ok = true;
  result.cpu.bytes = read.bytes.data;
  return result;
}

AssetRegistry::AssetRegistry(AssetLoader& loader) : loader_(&loader) {}

AssetRegistry::Record* AssetRegistry::find_mut(const AssetId& id) {
  const auto it = records_.find(id.key());
  if (it == records_.end()) {
    return nullptr;
  }
  return &it->second;
}

const AssetRegistry::Record* AssetRegistry::find(const AssetId& id) const {
  const auto it = records_.find(id.key());
  if (it == records_.end()) {
    return nullptr;
  }
  return &it->second;
}

AssetView AssetRegistry::view_from(const Record& record) const {
  AssetView view;
  view.id = record.entry.id;
  view.resolved_id = record.entry.id;
  view.kind = record.entry.kind;
  view.state = record.state;
  view.debug_name = record.entry.debug_name;
  view.cpu = record.cpu;
  view.gpu = record.gpu;
  view.error = record.error;
  return view;
}

GpuHandle AssetRegistry::allocate_gpu() {
  GpuHandle handle;
  handle.generation = ++next_gpu_;
  gpu_states_[handle.generation] = GpuHandleStatus::Live;
  ++last_gpu_uploads_;
  return handle;
}

void AssetRegistry::queue_gpu_destroy(GpuHandle handle) {
  if (!handle.valid()) {
    return;
  }
  gpu_states_[handle.generation] = GpuHandleStatus::PendingDestroy;
}

void AssetRegistry::register_asset(AssetCatalogEntry entry) {
  if (!entry.id.valid()) {
    return;
  }
  Record* existing = find_mut(entry.id);
  if (existing != nullptr) {
    existing->entry.kind = entry.kind;
    if (!entry.debug_name.empty()) {
      existing->entry.debug_name = std::move(entry.debug_name);
    }
    if (!entry.path.source.empty()) {
      existing->entry.path.source = std::move(entry.path.source);
    }
    if (!entry.path.compiled.empty()) {
      existing->entry.path.compiled = std::move(entry.path.compiled);
    }
    if (entry.fallback.valid()) {
      existing->entry.fallback = std::move(entry.fallback);
    }
    return;
  }
  Record record;
  record.entry = std::move(entry);
  record.state = AssetState::Unloaded;
  const std::string key = record.entry.id.key();
  records_.emplace(key, std::move(record));
}

void AssetRegistry::request_load(const AssetId& id) {
  Record* record = find_mut(id);
  if (record == nullptr) {
    return;
  }
  if (record->state == AssetState::Loading || record->state == AssetState::Ready) {
    return;
  }
  record->state = AssetState::Loading;
  record->error.clear();
  pending_loads_.push_back(id);
}

void AssetRegistry::pump_loads() {
  last_gpu_uploads_ = 0;
  const std::vector<AssetId> batch = std::move(pending_loads_);
  pending_loads_.clear();
  for (const AssetId& id : batch) {
    Record* record = find_mut(id);
    if (record == nullptr || loader_ == nullptr) {
      continue;
    }
    const AssetLoader::Result loaded =
        loader_->load(id, record->entry.kind, record->entry.path);
    if (!loaded.ok) {
      record->state = AssetState::Failed;
      record->cpu = {};
      record->gpu = {};
      record->error = loaded.error.empty() ? "asset load failed" : loaded.error;
      continue;
    }
    record->state = AssetState::Ready;
    record->cpu = loaded.cpu;
    record->error.clear();
    record->gpu = allocate_gpu();
  }
}

void AssetRegistry::hot_reload(const AssetId& id) {
  Record* record = find_mut(id);
  if (record == nullptr) {
    return;
  }
  queue_gpu_destroy(record->gpu);
  record->gpu = {};
  record->cpu = {};
  record->error.clear();
  if (record->state != AssetState::Loading) {
    pending_loads_.push_back(id);
  }
  record->state = AssetState::Loading;
}

void AssetRegistry::end_frame() {
  for (auto& [generation, status] : gpu_states_) {
    if (status == GpuHandleStatus::PendingDestroy) {
      status = GpuHandleStatus::Destroyed;
    }
  }
}

AssetState AssetRegistry::state(const AssetId& id) const {
  const Record* record = find(id);
  if (record == nullptr) {
    return AssetState::Failed;
  }
  return record->state;
}

std::string AssetRegistry::error(const AssetId& id) const {
  const Record* record = find(id);
  if (record == nullptr) {
    return "unknown asset";
  }
  return record->error;
}

AssetPath AssetRegistry::source_path(const AssetId& id) const {
  AssetPath path;
  const Record* record = find(id);
  if (record != nullptr) {
    path.source = record->entry.path.source;
  }
  return path;
}

std::string AssetRegistry::compiled_path(const AssetId& id) const {
  const Record* record = find(id);
  if (record == nullptr) {
    return {};
  }
  return record->entry.path.compiled;
}

AssetView AssetRegistry::resolve(const AssetId& id) const {
  const Record* record = find(id);
  if (record == nullptr) {
    return failed_unknown(id);
  }
  if (record->state == AssetState::Ready) {
    return view_from(*record);
  }

  AssetView view = view_from(*record);
  const AssetId fallback_id = record->entry.fallback;
  if (!fallback_id.valid() || fallback_id == id) {
    return view;
  }
  const Record* fallback = find(fallback_id);
  if (fallback == nullptr || fallback->state != AssetState::Ready) {
    return view;
  }
  AssetView resolved = view_from(*fallback);
  resolved.id = id;
  resolved.resolved_id = fallback_id;
  return resolved;
}

GpuHandleStatus AssetRegistry::gpu_status(GpuHandle handle) const {
  if (!handle.valid()) {
    return GpuHandleStatus::Invalid;
  }
  const auto it = gpu_states_.find(handle.generation);
  if (it == gpu_states_.end()) {
    return GpuHandleStatus::Destroyed;
  }
  return it->second;
}

void bind_map_assets(AssetRegistry& registry, const MapData& map) {
  for (const MapAssetRef& ref : map.assets) {
    AssetCatalogEntry entry;
    entry.id = ref.id;
    entry.kind = ref.kind;
    entry.debug_name = ref.debug_name.empty() ? ref.id.key() : ref.debug_name;
    registry.register_asset(std::move(entry));
    registry.request_load(ref.id);
  }
}

}  // namespace rat
