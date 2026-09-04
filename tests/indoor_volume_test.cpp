#include <rat/indoor_volume.hpp>
#include <rat/map_data.hpp>
#include <rat/player.hpp>

#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>

#include <cmath>
#include <cstdint>

namespace {

rat::MapData make_two_tile_map_with_house() {
  rat::MapData map;
  map.schema_version = 4;
  map.id = "indoor_dim";
  map.width = 2;
  map.height = 1;
  map.tile_size = 1.0f;
  map.height_grid.origin_x = 0;
  map.height_grid.origin_z = 0;
  map.height_grid.width = 2;
  map.height_grid.height = 1;
  map.height_grid.ground_y = {0.0f, 0.0f};
  map.indoor_volumes.push_back(rat::IndoorVolume{
      .xz = {0.0f, 0.0f, 1.0f, 1.0f},
      .y_lo = 0.0f,
      .y_hi = 2.0f,
  });
  return map;
}

[[nodiscard]] bool rgb_channels_darker(std::uint32_t dimmed, std::uint32_t base) {
  const auto ch = [](std::uint32_t color, int shift) { return (color >> shift) & 0xffu; };
  return ch(dimmed, 0) < ch(base, 0) && ch(dimmed, 8) < ch(base, 8) &&
         ch(dimmed, 16) < ch(base, 16) && ch(dimmed, 24) == ch(base, 24);
}

[[nodiscard]] std::uint32_t color_at_quad_center(const std::vector<rat::GreyboxFillVertex>& verts,
                                                 float cx, float cz) {
  for (std::size_t i = 0; i + 3 < verts.size(); i += 4) {
    const float qcx =
        0.25f * (verts[i].x + verts[i + 1].x + verts[i + 2].x + verts[i + 3].x);
    const float qcz =
        0.25f * (verts[i].z + verts[i + 1].z + verts[i + 2].z + verts[i + 3].z);
    if (std::fabs(qcx - cx) < 0.05f && std::fabs(qcz - cz) < 0.05f) {
      return verts[i].abgr;
    }
  }
  return 0;
}

}  // namespace

TEST_CASE("player cylinder inside indoor AABB is indoor", "[unit][map][collision]") {
  const rat::MapData map = make_two_tile_map_with_house();
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.half_extent = 0.4f;
  REQUIRE(rat::player_inside_indoor_volume(map, body));
}

TEST_CASE("player cylinder outside indoor AABB xz is outdoor", "[unit][map][collision]") {
  const rat::MapData map = make_two_tile_map_with_house();
  rat::PlayerBody body;
  body.x = 5.0f;
  body.y = 0.0f;
  body.z = 5.0f;
  body.half_extent = 0.4f;
  REQUIRE_FALSE(rat::player_inside_indoor_volume(map, body));
}

TEST_CASE("player cylinder xz miss just beyond radius is outdoor", "[unit][map][collision]") {
  const rat::MapData map = make_two_tile_map_with_house();
  rat::PlayerBody body;
  body.x = 1.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.half_extent = 0.4f;
  REQUIRE_FALSE(rat::player_inside_indoor_volume(map, body));
}

TEST_CASE("player cylinder y range miss is outdoor", "[unit][map][collision]") {
  const rat::MapData map = make_two_tile_map_with_house();
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 10.0f;
  body.z = 0.5f;
  body.half_extent = 0.4f;
  REQUIRE_FALSE(rat::player_inside_indoor_volume(map, body));
}

TEST_CASE("player cylinder grazing indoor AABB xz is indoor", "[unit][map][collision]") {
  const rat::MapData map = make_two_tile_map_with_house();
  rat::PlayerBody body;
  body.x = 1.3f;
  body.y = 0.0f;
  body.z = 0.5f;
  body.half_extent = 0.4f;
  REQUIRE(rat::player_inside_indoor_volume(map, body));
}

TEST_CASE("empty indoor_volumes is outdoor", "[unit][map][collision]") {
  rat::MapData map = make_two_tile_map_with_house();
  map.indoor_volumes.clear();
  rat::PlayerBody body;
  body.x = 0.5f;
  body.y = 0.0f;
  body.z = 0.5f;
  REQUIRE_FALSE(rat::player_inside_indoor_volume(map, body));
}

TEST_CASE("greybox outdoor fill is darker iff player is indoor", "[unit][map][terrain]") {
  const rat::MapData map = make_two_tile_map_with_house();
  constexpr std::uint32_t kBase = 0xff707070;

  rat::PlayerBody inside;
  inside.x = 0.5f;
  inside.y = 0.0f;
  inside.z = 0.5f;

  rat::PlayerBody outside;
  outside.x = 1.5f;
  outside.y = 0.0f;
  outside.z = 0.5f;

  const std::uint32_t indoor_when_in =
      rat::greybox_fill_abgr(map, inside, 0.5f, 0.0f, 0.5f, kBase);
  const std::uint32_t outdoor_when_in =
      rat::greybox_fill_abgr(map, inside, 1.5f, 0.0f, 0.5f, kBase);
  const std::uint32_t outdoor_when_out =
      rat::greybox_fill_abgr(map, outside, 1.5f, 0.0f, 0.5f, kBase);
  const std::uint32_t indoor_when_out =
      rat::greybox_fill_abgr(map, outside, 0.5f, 0.0f, 0.5f, kBase);

  REQUIRE(indoor_when_in == kBase);
  REQUIRE(rgb_channels_darker(outdoor_when_in, kBase));
  REQUIRE(outdoor_when_out == kBase);
  REQUIRE(indoor_when_out == kBase);
}

TEST_CASE("greybox fill vertices dim outdoor quads when player is indoor",
          "[unit][map][terrain]") {
  const rat::MapData map = make_two_tile_map_with_house();

  rat::PlayerBody inside;
  inside.x = 0.5f;
  inside.y = 0.0f;
  inside.z = 0.5f;

  rat::PlayerBody outside;
  outside.x = 1.5f;
  outside.y = 0.0f;
  outside.z = 0.5f;

  const rat::GreyboxFillMesh dimmed = rat::build_greybox_fill_mesh(map, inside);
  const rat::GreyboxFillMesh undimmed = rat::build_greybox_fill_mesh(map, outside);
  REQUIRE_FALSE(dimmed.vertices.empty());
  REQUIRE(dimmed.vertices.size() == undimmed.vertices.size());

  const std::uint32_t indoor_dim = color_at_quad_center(dimmed.vertices, 0.5f, 0.5f);
  const std::uint32_t outdoor_dim = color_at_quad_center(dimmed.vertices, 1.5f, 0.5f);
  const std::uint32_t indoor_plain = color_at_quad_center(undimmed.vertices, 0.5f, 0.5f);
  const std::uint32_t outdoor_plain = color_at_quad_center(undimmed.vertices, 1.5f, 0.5f);

  REQUIRE(indoor_dim != 0);
  REQUIRE(outdoor_dim != 0);
  REQUIRE(indoor_dim == indoor_plain);
  REQUIRE(outdoor_plain == indoor_plain);
  REQUIRE(rgb_channels_darker(outdoor_dim, outdoor_plain));
}
