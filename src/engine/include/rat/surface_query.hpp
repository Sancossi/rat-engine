#pragma once

#include "rat/map_data.hpp"

#include <memory>

namespace rat {

struct SurfaceSample {
  float y = 0.0f;
  int surface_id = 0;
  int ramp_index = -1;
  bool walkable = true;
  bool on_ramp = false;
};

class SurfaceQuery {
 public:
  explicit SurfaceQuery(const MapData& map);
  ~SurfaceQuery();

  SurfaceQuery(SurfaceQuery&&) noexcept;
  SurfaceQuery& operator=(SurfaceQuery&&) noexcept;

  SurfaceQuery(const SurfaceQuery&) = delete;
  SurfaceQuery& operator=(const SurfaceQuery&) = delete;

  [[nodiscard]] SurfaceSample sample(float world_x, float world_z) const;
  [[nodiscard]] float tile_size() const;

 private:
  struct Impl;
  std::unique_ptr<Impl> impl_;
};

}  // namespace rat
