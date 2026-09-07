#pragma once

#include "rat/indoor_volume.hpp"
#include "rat/camera.hpp"

namespace rat {

struct SurfaceVertex {
  float x, y, z;
  float nx, ny, nz;
  std::uint32_t abgr;
};
struct SurfaceContour {
  Vec3 a, b;
};
struct SurfaceVisualMesh {
  std::vector<SurfaceVertex> vertices;
  std::vector<std::uint32_t> indices;
  std::vector<SurfaceContour> contours;
};

// Render-only preparation. Cancels touching opposed rectangles (including
// partial contacts), preserves exposed faces and omits coplanar interior edges.
// Oblique collinear edges weld within 1e-6 tile units; gameplay is unaffected.
[[nodiscard]] SurfaceVisualMesh build_surface_visual_mesh(const GreyboxFillMesh& fill,
                                                          float tile_size = 1.0f);
}  // namespace rat
