#pragma once

#include "rat/asset.hpp"
#include "rat/camera.hpp"

#include <cstdint>
#include <utility>
#include <vector>

namespace rat {

struct EntityId {
  std::uint32_t index = 0;
  std::uint32_t generation = 0;

  [[nodiscard]] bool valid() const { return generation != 0; }

  [[nodiscard]] friend bool operator==(EntityId a, EntityId b) {
    return a.index == b.index && a.generation == b.generation;
  }
  [[nodiscard]] friend bool operator!=(EntityId a, EntityId b) { return !(a == b); }
};

class EntityRegistry {
 public:
  [[nodiscard]] EntityId create();
  void destroy(EntityId id);
  [[nodiscard]] bool alive(EntityId id) const;

 private:
  struct Slot {
    std::uint32_t generation = 0;
    bool occupied = false;
  };

  std::vector<Slot> slots_;
  std::vector<std::uint32_t> free_;
};

template <typename T>
class ComponentStore {
 public:
  bool insert(const EntityRegistry& entities, EntityId id, T value) {
    if (!entities.alive(id)) {
      return false;
    }
    if (id.index >= slots_.size()) {
      slots_.resize(id.index + 1);
    }
    slots_[id.index] = Slot{id.generation, true, std::move(value)};
    return true;
  }

  bool remove(const EntityRegistry& entities, EntityId id) {
    if (!contains(entities, id)) {
      return false;
    }
    slots_[id.index].occupied = false;
    return true;
  }

  [[nodiscard]] T* get(const EntityRegistry& entities, EntityId id) {
    return const_cast<T*>(static_cast<const ComponentStore*>(this)->get(entities, id));
  }

  [[nodiscard]] const T* get(const EntityRegistry& entities, EntityId id) const {
    if (!contains(entities, id)) {
      return nullptr;
    }
    return &slots_[id.index].value;
  }

  [[nodiscard]] bool contains(const EntityRegistry& entities, EntityId id) const {
    if (!entities.alive(id) || id.index >= slots_.size()) {
      return false;
    }
    const Slot& slot = slots_[id.index];
    return slot.occupied && slot.generation == id.generation;
  }

  template <typename F>
  void for_each(const EntityRegistry& entities, F&& fn) {
    for (std::uint32_t i = 0; i < slots_.size(); ++i) {
      Slot& slot = slots_[i];
      if (!slot.occupied) {
        continue;
      }
      const EntityId id{i, slot.generation};
      if (!entities.alive(id)) {
        continue;
      }
      fn(id, slot.value);
    }
  }

  template <typename F>
  void for_each(const EntityRegistry& entities, F&& fn) const {
    for (std::uint32_t i = 0; i < slots_.size(); ++i) {
      const Slot& slot = slots_[i];
      if (!slot.occupied) {
        continue;
      }
      const EntityId id{i, slot.generation};
      if (!entities.alive(id)) {
        continue;
      }
      fn(id, slot.value);
    }
  }

 private:
  struct Slot {
    std::uint32_t generation = 0;
    bool occupied = false;
    T value{};
  };

  std::vector<Slot> slots_;
};

struct Transform {
  Vec3 position{};
  float yaw = 0.0f;
};

struct Renderable {
  AssetId mesh;
};

struct Collider {
  float radius = 0.4f;
  float height = 1.6f;
};

}  // namespace rat
