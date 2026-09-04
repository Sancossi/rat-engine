#pragma once

#include "rat/map_data.hpp"
#include "rat/player.hpp"

#include <cstdint>
#include <vector>

namespace rat {

inline constexpr float kGreyboxIndoorDimFactor = 0.4f;

struct GreyboxFillVertex {
  float x = 0.0f;
  float y = 0.0f;
  float z = 0.0f;
  std::uint32_t abgr = 0xffffffff;
};

struct GreyboxFillMesh {
  std::vector<GreyboxFillVertex> vertices;
  std::vector<std::uint16_t> indices;
};

[[nodiscard]] bool player_inside_indoor_volume(const MapData& map, const PlayerBody& body);
[[nodiscard]] bool point_inside_indoor_volume(const MapData& map, float x, float y, float z);
[[nodiscard]] std::uint32_t multiply_abgr(std::uint32_t abgr, float factor);
[[nodiscard]] std::uint32_t greybox_fill_abgr(const MapData& map, const PlayerBody& body, float cx,
                                              float cy, float cz, std::uint32_t base_abgr);
[[nodiscard]] GreyboxFillMesh build_greybox_fill_mesh(const MapData& map, const PlayerBody& body,
                                                      bool apply_indoor_dim = true);

}  // namespace rat
