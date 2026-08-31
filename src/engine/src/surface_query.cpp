#include "rat/surface_query.hpp"

#include <algorithm>
#include <cmath>
#include <utility>
#include <vector>

namespace rat {
namespace {

struct RampRuntime {
  int ramp_index = -1;
  int tile_x = 0;
  int tile_z = 0;
  RampDirection direction = RampDirection::North;
  float low_y = 0.0f;
  float high_y = 0.0f;
};

float clamp01(float value) {
  return std::clamp(value, 0.0f, 1.0f);
}

}  // namespace

struct SurfaceQuery::Impl {
  int origin_x = 0;
  int origin_z = 0;
  int width = 0;
  int height = 0;
  float tile_size = 1.0f;
  std::vector<float> ground_y;
  std::vector<RampRuntime> ramps;
};

SurfaceQuery::SurfaceQuery(const MapData& map) : impl_(std::make_unique<Impl>()) {
  impl_->origin_x = map.height_grid.origin_x;
  impl_->origin_z = map.height_grid.origin_z;
  impl_->width = map.height_grid.width;
  impl_->height = map.height_grid.height;
  impl_->tile_size = map.tile_size > 0.0f ? map.tile_size : 1.0f;
  impl_->ground_y = map.height_grid.ground_y;
  impl_->ramps.reserve(map.ramps.size());
  int ramp_index = 0;
  for (const RampDef& ramp : map.ramps) {
    impl_->ramps.push_back(RampRuntime{ramp_index++, ramp.tile.x, ramp.tile.z, ramp.direction,
                                       ramp.low_y, ramp.high_y});
  }
}

SurfaceQuery::~SurfaceQuery() = default;
SurfaceQuery::SurfaceQuery(SurfaceQuery&&) noexcept = default;
SurfaceQuery& SurfaceQuery::operator=(SurfaceQuery&&) noexcept = default;

float SurfaceQuery::tile_size() const {
  if (!impl_) {
    return 1.0f;
  }
  return impl_->tile_size;
}

SurfaceSample SurfaceQuery::sample(float world_x, float world_z) const {
  SurfaceSample out;
  if (!impl_) {
    return out;
  }
  const Impl& impl = *impl_;
  if (impl.width <= 0 || impl.height <= 0) {
    return out;
  }
  const std::size_t expected_cells = static_cast<std::size_t>(impl.width) * impl.height;
  if (impl.ground_y.size() != expected_cells) {
    return out;
  }

  const float origin_world_x = static_cast<float>(impl.origin_x) * impl.tile_size;
  const float origin_world_z = static_cast<float>(impl.origin_z) * impl.tile_size;
  const float local_x = (world_x - origin_world_x) / impl.tile_size;
  const float local_z = (world_z - origin_world_z) / impl.tile_size;
  const int tile_x = static_cast<int>(std::floor(local_x));
  const int tile_z = static_cast<int>(std::floor(local_z));
  if (tile_x < 0 || tile_z < 0 || tile_x >= impl.width || tile_z >= impl.height) {
    return out;
  }

  const std::size_t index = static_cast<std::size_t>(tile_z) * impl.width + tile_x;
  out.y = impl.ground_y[index];

  for (const RampRuntime& ramp : impl.ramps) {
    if (ramp.tile_x != tile_x + impl.origin_x || ramp.tile_z != tile_z + impl.origin_z) {
      continue;
    }

    const float frac_x = clamp01(local_x - static_cast<float>(tile_x));
    const float frac_z = clamp01(local_z - static_cast<float>(tile_z));
    float t = 0.0f;
    switch (ramp.direction) {
      case RampDirection::North:
        t = 1.0f - frac_z;
        break;
      case RampDirection::East:
        t = frac_x;
        break;
      case RampDirection::South:
        t = frac_z;
        break;
      case RampDirection::West:
        t = 1.0f - frac_x;
        break;
    }
    out.y = ramp.low_y + (ramp.high_y - ramp.low_y) * clamp01(t);
    out.ramp_index = ramp.ramp_index;
    out.on_ramp = true;
    break;
  }

  return out;
}

}  // namespace rat
