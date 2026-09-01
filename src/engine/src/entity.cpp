#include "rat/entity.hpp"

namespace rat {

EntityId EntityRegistry::create() {
  if (!free_.empty()) {
    const std::uint32_t index = free_.back();
    free_.pop_back();
    Slot& slot = slots_[index];
    slot.occupied = true;
    return EntityId{index, slot.generation};
  }
  const std::uint32_t index = static_cast<std::uint32_t>(slots_.size());
  slots_.push_back(Slot{1, true});
  return EntityId{index, 1};
}

void EntityRegistry::destroy(EntityId id) {
  if (!alive(id)) {
    return;
  }
  Slot& slot = slots_[id.index];
  slot.occupied = false;
  ++slot.generation;
  if (slot.generation == 0) {
    slot.generation = 1;
  }
  free_.push_back(id.index);
}

bool EntityRegistry::alive(EntityId id) const {
  if (!id.valid() || id.index >= slots_.size()) {
    return false;
  }
  const Slot& slot = slots_[id.index];
  return slot.occupied && slot.generation == id.generation;
}

}  // namespace rat
