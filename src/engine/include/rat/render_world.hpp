#pragma once

#include "rat/asset.hpp"
#include "rat/entity.hpp"
#include "rat/map_data.hpp"
#include "rat/player.hpp"

#include <cstddef>
#include <cstdint>
#include <string>
#include <unordered_map>
#include <vector>

namespace rat {

class SimulationSession;

enum class RenderPass : std::uint8_t {
  Depth = 0,
  Opaque = 1,
  Debug = 2,
};

inline constexpr std::uint8_t kRenderPassCount = 3;

[[nodiscard]] const char* render_pass_name(RenderPass pass);
[[nodiscard]] std::uint16_t render_pass_view_id(RenderPass pass);

struct MaterialHandle {
  AssetId id;
  std::uint32_t sort_key = 0;
};

struct RenderPacket {
  EntityId entity{};
  Transform transform{};
  AssetId mesh;
  MaterialHandle material;
  RenderPass pass = RenderPass::Opaque;
  std::uint64_t sort_key = 0;
};

struct RenderWorld {
  std::uint64_t tick_id = 0;
  float interpolation_alpha = 1.0f;
  std::vector<RenderPacket> packets;
  std::vector<std::string> pass_names;
  std::vector<std::string> cpu_markers;
};

struct RenderWorldBuildInput {
  const EntityRegistry* entities = nullptr;
  const ComponentStore<Transform>* previous = nullptr;
  const ComponentStore<Transform>* current = nullptr;
  const ComponentStore<Renderable>* renderables = nullptr;
  float interpolation_alpha = 1.0f;
  std::uint64_t tick_id = 0;
};

struct RenderStoreFill {
  EntityRegistry entities;
  ComponentStore<Transform> transforms;
  ComponentStore<Renderable> renderables;
};

class FrameAllocator {
 public:
  void* allocate(std::size_t bytes, std::size_t alignment = alignof(std::max_align_t));
  void reset();
  [[nodiscard]] std::size_t used() const { return used_; }
  [[nodiscard]] std::size_t allocation_count() const { return allocation_count_; }

 private:
  std::vector<std::uint8_t> storage_;
  std::size_t used_ = 0;
  std::size_t allocation_count_ = 0;
};

class DeferredGpuFreeQueue {
 public:
  void queue(GpuHandle handle);
  void retire();
  [[nodiscard]] GpuHandleStatus status(GpuHandle handle) const;

 private:
  std::unordered_map<std::uint64_t, GpuHandleStatus> statuses_;
};

class RenderDocHook {
 public:
  [[nodiscard]] bool available() const { return false; }
  void begin_capture();
  void end_capture();
  [[nodiscard]] bool capturing() const { return capturing_; }

 private:
  bool capturing_ = false;
};

[[nodiscard]] Transform interpolate_transform(const Transform& previous, const Transform& current,
                                              float alpha);

[[nodiscard]] RenderWorld build_render_world(const RenderWorldBuildInput& input);

void sort_render_packets(std::vector<RenderPacket>& packets);

void fill_render_stores(RenderStoreFill& out, const PlayerBody& player, const MapData& map);

[[nodiscard]] RenderWorld capture_render_world(const SimulationSession& session,
                                               float interpolation_alpha = 1.0f);

}  // namespace rat
