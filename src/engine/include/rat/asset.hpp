#pragma once

#include <cstdint>
#include <string>
#include <string_view>
#include <unordered_map>
#include <vector>

namespace rat {

enum class AssetKind {
  Texture,
  AudioClip,
  MeshDescriptor,
  MaterialDescriptor,
};

enum class AssetState {
  Unloaded,
  Loading,
  Ready,
  Failed,
};

enum class GpuHandleStatus {
  Invalid,
  Live,
  PendingDestroy,
  Destroyed,
};

struct AssetId {
  std::string catalog_key;

  [[nodiscard]] bool valid() const { return !catalog_key.empty(); }
  [[nodiscard]] const std::string& key() const { return catalog_key; }

  [[nodiscard]] friend bool operator==(const AssetId& a, const AssetId& b) {
    return a.catalog_key == b.catalog_key;
  }
  [[nodiscard]] friend bool operator!=(const AssetId& a, const AssetId& b) {
    return !(a == b);
  }
};

[[nodiscard]] inline AssetId make_asset_id(std::string_view catalog_key) {
  AssetId id;
  id.catalog_key.assign(catalog_key.data(), catalog_key.size());
  return id;
}

struct AssetPath {
  std::string source;
  std::string compiled;
};

struct AssetCpuData {
  std::vector<std::uint8_t> bytes;
};

struct GpuHandle {
  std::uint64_t generation = 0;

  [[nodiscard]] bool valid() const { return generation != 0; }

  [[nodiscard]] friend bool operator==(GpuHandle a, GpuHandle b) {
    return a.generation == b.generation;
  }
  [[nodiscard]] friend bool operator!=(GpuHandle a, GpuHandle b) { return !(a == b); }
};

struct AssetCatalogEntry {
  AssetId id;
  AssetKind kind = AssetKind::Texture;
  std::string debug_name;
  AssetPath path;
  AssetId fallback;
};

struct AssetView {
  AssetId id;
  AssetId resolved_id;
  AssetKind kind = AssetKind::Texture;
  AssetState state = AssetState::Unloaded;
  std::string debug_name;
  AssetCpuData cpu;
  GpuHandle gpu{};
  AssetPath path;
  std::string error;
};

class AssetLoader {
 public:
  struct Result {
    bool ok = false;
    AssetCpuData cpu;
    std::string error;
  };

  virtual ~AssetLoader() = default;
  virtual Result load(const AssetId& id, AssetKind kind, const AssetPath& path) = 0;
};

class MemoryAssetLoader : public AssetLoader {
 public:
  void set(const AssetId& id, AssetCpuData cpu);
  void set_fail(const AssetId& id, std::string error);
  Result load(const AssetId& id, AssetKind kind, const AssetPath& path) override;

 private:
  struct Slot {
    bool fail = false;
    std::string error;
    AssetCpuData cpu;
  };

  std::unordered_map<std::string, Slot> slots_;
};

class AssetRegistry {
 public:
  explicit AssetRegistry(AssetLoader& loader);

  void register_asset(AssetCatalogEntry entry);
  void request_load(const AssetId& id);
  void pump_loads();
  void hot_reload(const AssetId& id);
  void end_frame();

  [[nodiscard]] AssetState state(const AssetId& id) const;
  [[nodiscard]] std::string error(const AssetId& id) const;
  [[nodiscard]] AssetPath source_path(const AssetId& id) const;
  [[nodiscard]] std::string compiled_path(const AssetId& id) const;
  [[nodiscard]] AssetView resolve(const AssetId& id) const;
  [[nodiscard]] GpuHandleStatus gpu_status(GpuHandle handle) const;

 private:
  struct Record {
    AssetCatalogEntry entry;
    AssetState state = AssetState::Unloaded;
    AssetCpuData cpu;
    GpuHandle gpu{};
    std::string error;
  };

  [[nodiscard]] Record* find_mut(const AssetId& id);
  [[nodiscard]] const Record* find(const AssetId& id) const;
  [[nodiscard]] AssetView view_from(const Record& record) const;
  void queue_gpu_destroy(GpuHandle handle);
  GpuHandle allocate_gpu();

  AssetLoader* loader_ = nullptr;
  std::unordered_map<std::string, Record> records_;
  std::vector<AssetId> pending_loads_;
  std::uint64_t next_gpu_ = 0;
  std::unordered_map<std::uint64_t, GpuHandleStatus> gpu_states_;
};

struct MapData;

class FileStore;

class FileAssetLoader : public AssetLoader {
 public:
  explicit FileAssetLoader(FileStore& files);
  Result load(const AssetId& id, AssetKind kind, const AssetPath& path) override;

 private:
  FileStore* files_ = nullptr;
};

void bind_map_assets(AssetRegistry& registry, const MapData& map);

}  // namespace rat
