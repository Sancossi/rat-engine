#include "rat/render_world.hpp"

#include "rat/simulation_session.hpp"

#include <algorithm>

namespace rat {
namespace {

constexpr const char* kPassNames[kRenderPassCount] = {"Depth", "Opaque", "Debug"};

[[nodiscard]] std::uint32_t hash_catalog_key(std::string_view key) {
  std::uint32_t hash = 2166136261u;
  for (unsigned char c : key) {
    hash ^= c;
    hash *= 16777619u;
  }
  return hash;
}

[[nodiscard]] MaterialHandle material_from_mesh(const AssetId& mesh) {
  MaterialHandle material;
  material.id = make_asset_id("mat/default");
  material.sort_key = hash_catalog_key(mesh.key());
  return material;
}

void append_main_pass_names(RenderWorld& world) {
  world.pass_names.clear();
  world.pass_names.reserve(kRenderPassCount);
  for (std::uint8_t i = 0; i < kRenderPassCount; ++i) {
    world.pass_names.emplace_back(kPassNames[i]);
  }
}

}  // namespace

const char* render_pass_name(RenderPass pass) {
  const auto index = static_cast<std::uint8_t>(pass);
  if (index >= kRenderPassCount) {
    return "Opaque";
  }
  return kPassNames[index];
}

std::uint16_t render_pass_view_id(RenderPass pass) {
  return static_cast<std::uint16_t>(1u + static_cast<std::uint8_t>(pass));
}

void* FrameAllocator::allocate(std::size_t bytes, std::size_t alignment) {
  if (bytes == 0) {
    bytes = 1;
  }
  if (alignment < 1) {
    alignment = 1;
  }
  std::size_t offset = used_;
  const std::size_t mask = alignment - 1;
  if ((alignment & mask) == 0) {
    offset = (offset + mask) & ~mask;
  }
  const std::size_t end = offset + bytes;
  if (end > storage_.size()) {
    storage_.resize(end);
  }
  used_ = end;
  ++allocation_count_;
  return storage_.data() + offset;
}

void FrameAllocator::reset() {
  used_ = 0;
  allocation_count_ = 0;
}

void DeferredGpuFreeQueue::queue(GpuHandle handle) {
  if (!handle.valid()) {
    return;
  }
  statuses_[handle.generation] = GpuHandleStatus::PendingDestroy;
}

void DeferredGpuFreeQueue::retire() {
  for (auto& entry : statuses_) {
    if (entry.second == GpuHandleStatus::PendingDestroy) {
      entry.second = GpuHandleStatus::Destroyed;
    }
  }
}

GpuHandleStatus DeferredGpuFreeQueue::status(GpuHandle handle) const {
  if (!handle.valid()) {
    return GpuHandleStatus::Invalid;
  }
  const auto it = statuses_.find(handle.generation);
  if (it == statuses_.end()) {
    return GpuHandleStatus::Invalid;
  }
  return it->second;
}

void RenderDocHook::begin_capture() {
  capturing_ = available();
}

void RenderDocHook::end_capture() {
  capturing_ = false;
}

Transform interpolate_transform(const Transform& previous, const Transform& current, float alpha) {
  if (alpha >= 1.0f) {
    return current;
  }
  if (alpha <= 0.0f) {
    return previous;
  }
  Transform out;
  out.position.x = previous.position.x + (current.position.x - previous.position.x) * alpha;
  out.position.y = previous.position.y + (current.position.y - previous.position.y) * alpha;
  out.position.z = previous.position.z + (current.position.z - previous.position.z) * alpha;
  out.yaw = previous.yaw + (current.yaw - previous.yaw) * alpha;
  return out;
}

void sort_render_packets(std::vector<RenderPacket>& packets) {
  std::sort(packets.begin(), packets.end(), [](const RenderPacket& a, const RenderPacket& b) {
    if (a.pass != b.pass) {
      return static_cast<std::uint8_t>(a.pass) < static_cast<std::uint8_t>(b.pass);
    }
    if (a.material.sort_key != b.material.sort_key) {
      return a.material.sort_key < b.material.sort_key;
    }
    if (a.mesh.key() != b.mesh.key()) {
      return a.mesh.key() < b.mesh.key();
    }
    return a.entity.index < b.entity.index;
  });
  for (RenderPacket& packet : packets) {
    packet.sort_key = (static_cast<std::uint64_t>(static_cast<std::uint8_t>(packet.pass)) << 32) |
                      packet.material.sort_key;
  }
}

RenderWorld build_render_world(const RenderWorldBuildInput& input) {
  RenderWorld world;
  world.tick_id = input.tick_id;
  world.interpolation_alpha = input.interpolation_alpha;
  world.cpu_markers.emplace_back("build_render_world");
  append_main_pass_names(world);

  if (input.entities == nullptr || input.current == nullptr || input.renderables == nullptr) {
    return world;
  }

  input.renderables->for_each(*input.entities, [&](EntityId id, const Renderable& drawable) {
    const Transform* current = input.current->get(*input.entities, id);
    if (current == nullptr) {
      return;
    }
    Transform xf = *current;
    if (input.previous != nullptr) {
      const Transform* previous = input.previous->get(*input.entities, id);
      if (previous != nullptr) {
        xf = interpolate_transform(*previous, *current, input.interpolation_alpha);
      }
    }
    RenderPacket packet;
    packet.entity = id;
    packet.transform = xf;
    packet.mesh = drawable.mesh;
    packet.material = material_from_mesh(drawable.mesh);
    packet.pass = RenderPass::Opaque;
    world.packets.push_back(std::move(packet));
  });

  sort_render_packets(world.packets);
  world.cpu_markers.emplace_back("sort_packets");
  return world;
}

void fill_render_stores(RenderStoreFill& out, const PlayerBody& player, const MapData& map) {
  const EntityId player_id = out.entities.create();
  Transform player_xf;
  player_xf.position = {player.x, player.y, player.z};
  out.transforms.insert(out.entities, player_id, player_xf);
  Renderable player_draw;
  player_draw.mesh = make_asset_id("mesh/player");
  out.renderables.insert(out.entities, player_id, player_draw);

  for (const BlockerDef& blocker : map.blockers) {
    const EntityId id = out.entities.create();
    Transform xf;
    xf.position.x = 0.5f * (blocker.bounds.min_x + blocker.bounds.max_x);
    xf.position.z = 0.5f * (blocker.bounds.min_z + blocker.bounds.max_z);
    if (blocker.base_y.has_value() && blocker.top_y.has_value()) {
      xf.position.y = 0.5f * (*blocker.base_y + *blocker.top_y);
    }
    out.transforms.insert(out.entities, id, xf);
    Renderable drawable;
    drawable.mesh = make_asset_id("mesh/blocker");
    out.renderables.insert(out.entities, id, drawable);
  }
}

RenderWorld capture_render_world(const SimulationSession& session, float interpolation_alpha) {
  RenderStoreFill stores;
  fill_render_stores(stores, session.player(), session.events().map());
  RenderWorldBuildInput input;
  input.entities = &stores.entities;
  input.current = &stores.transforms;
  input.renderables = &stores.renderables;
  input.interpolation_alpha = interpolation_alpha;
  input.tick_id = session.tick_id();
  return build_render_world(input);
}

}  // namespace rat
