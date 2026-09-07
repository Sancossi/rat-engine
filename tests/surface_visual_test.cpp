#include <rat/surface_visual.hpp>
#include <rat/terrain_geometry.hpp>
#include <catch2/catch_approx.hpp>
#include <catch2/catch_test_macros.hpp>
#include <cmath>
#include <set>

namespace {
void append(rat::GreyboxFillMesh& mesh, const std::vector<rat::TerrainFillQuad>& quads) {
  for (const auto& q:quads) {
    const auto base=static_cast<std::uint16_t>(mesh.vertices.size());
    for (const auto p:{rat::Vec3{q.x0,q.y0,q.z0},rat::Vec3{q.x1,q.y1,q.z1},rat::Vec3{q.x2,q.y2,q.z2},rat::Vec3{q.x3,q.y3,q.z3}})
      mesh.vertices.push_back({p.x,p.y,p.z,0xff707070});
    for (auto i:{0,2,1,0,3,2}) mesh.indices.push_back(static_cast<std::uint16_t>(base+i));
  }
}
double length(const rat::SurfaceContour& e) {
  const double x=static_cast<double>(e.b.x)-e.a.x,y=static_cast<double>(e.b.y)-e.a.y,z=static_cast<double>(e.b.z)-e.a.z;
  return std::sqrt(x*x+y*y+z*z);
}
double contour_length(const rat::SurfaceVisualMesh& mesh) {
  double total=0;for(const auto& e:mesh.contours) total+=length(e);return total;
}
}

TEST_CASE("Surface contours omit cube triangle diagonals and adjacent or stacked voxel seams", "[unit][surface-visual]") {
  for (int axis:{0,1}) {
    rat::GreyboxFillMesh raw;
    append(raw,rat::build_occupancy_solid_fill_quads({0,0,0,rat::OccupancyKind::Solid},1));
    auto one=rat::build_surface_visual_mesh(raw);
    CHECK(one.vertices.size()==24);
    CHECK(one.contours.size()==12);
    CHECK(contour_length(one)==Catch::Approx(12));
    append(raw,rat::build_occupancy_solid_fill_quads({axis==0 ? 1 : 0,axis==1 ? 1 : 0,0,rat::OccupancyKind::Solid},1));
    const auto joined=rat::build_surface_visual_mesh(raw);
    CHECK(joined.vertices.size()==40); // Ten exposed quads, touching faces removed.
    CHECK(contour_length(joined)==Catch::Approx(16));
    for(const auto& e:joined.contours) {
      if(axis==0) CHECK_FALSE((e.a.x==1 && e.b.x==1));
      else CHECK_FALSE((e.a.y==1 && e.b.y==1));
    }
    CHECK(raw.vertices.size()==48); // Preparation leaves the source mesh unchanged.
  }
}

TEST_CASE("Partial slab contacts remove only hidden faces and retain underside height breaks", "[unit][surface-visual]") {
  rat::GreyboxFillMesh raw;
  append(raw,rat::build_floor_slab_fill_quads({{0,0},2,0.5f},1));
  append(raw,rat::build_floor_slab_fill_quads({{1,0},2,0.25f},1));
  const auto mesh=rat::build_surface_visual_mesh(raw);
  bool step=false;
  for(const auto& e:mesh.contours) {
    CHECK_FALSE((e.a.x==1 && e.b.x==1 && e.a.y==2 && e.b.y==2));
    if(e.a.x==1 && e.b.x==1 && e.a.y==1.75f && e.b.y==1.75f) step=true;
  }
  CHECK(step);
  for(std::size_t i=0;i<mesh.vertices.size();i+=4) {
    bool shared=true;float high=-100;
    for(std::size_t j=0;j<4;++j) { shared &= mesh.vertices[i+j].x==1;high=std::max(high,mesh.vertices[i+j].y); }
    if(shared) CHECK(high<=1.75f);
  }
}

TEST_CASE("Every ramp yaw has normalized slope normals and only prism boundary contours", "[unit][surface-visual]") {
  for(float scale:{1.0f,1e30f,3e38f}) for(int yaw=0;yaw<4;++yaw) {
    INFO(scale);INFO(yaw);
    rat::GreyboxFillMesh raw;
    append(raw,rat::build_occupancy_ramp_fill_quads({0,0,0,rat::OccupancyKind::Ramp,static_cast<rat::RampDirection>(yaw)},scale));
    const auto mesh=rat::build_surface_visual_mesh(raw,scale);
    bool slope=false,underside=false;
    for(const auto& v:mesh.vertices) {
      CHECK(std::isfinite(v.x));CHECK(std::isfinite(v.y));CHECK(std::isfinite(v.z));
      CHECK(v.nx*v.nx+v.ny*v.ny+v.nz*v.nz==Catch::Approx(1));
      if(v.ny>0.5f && v.ny<0.9f) slope=true;
      if(v.ny < -0.9f) underside=true;
    }
    CHECK(slope);CHECK(underside);
    CHECK(contour_length(mesh)/scale==Catch::Approx(7+2*std::sqrt(2.0)).epsilon(1e-5));
  }
}

TEST_CASE("Surface material preparation preserves indoor dim colors and terrain height breaks", "[unit][surface-visual]") {
  rat::MapData map;map.schema_version=4;map.width=2;map.height=1;map.tile_size=1;map.height_grid={0,0,2,1,{0,1}};
  map.indoor_volumes.push_back({{0,0,1,1},0,2});
  rat::PlayerBody player;player.x=.5f;player.y=0;player.z=.5f;
  const auto raw=rat::build_greybox_fill_mesh(map,player,true);
  const auto mesh=rat::build_surface_visual_mesh(raw);
  bool normal=false,dim=false,step=false;
  for(const auto& v:mesh.vertices) {
    normal |= v.abgr==0xff707070;
    dim |= v.abgr==rat::multiply_abgr(0xff707070,rat::kGreyboxIndoorDimFactor);
  }
  for(const auto& e:mesh.contours) if(e.a.x==1 && e.b.x==1 && e.a.y==1 && e.b.y==1) step=true;
  CHECK(normal);CHECK(dim);CHECK(step);
}

TEST_CASE("Greybox solid and slab normals point outwards despite legacy side winding", "[unit][surface-visual]") {
  for(bool slab:{false,true}) {
    rat::MapData map;map.schema_version=5;map.width=1;map.height=1;map.tile_size=1;map.height_grid={0,0,1,1,{-2}};
    if(slab) map.floor_slabs.push_back({{0,0},2,.25f});
    else map.occupancy.push_back({0,1,0,rat::OccupancyKind::Solid});
    const auto raw=rat::build_greybox_fill_mesh(map,{},false);
    const auto mesh=rat::build_surface_visual_mesh(raw);
    int sides=0;
    const float center_y=slab ? 1.875f : 1.5f;
    for(const auto& v:mesh.vertices) if(v.y>=1) {
      CHECK(v.nx*(v.x-.5f)+v.ny*(v.y-center_y)+v.nz*(v.z-.5f)>0);
      if(v.ny==0) ++sides;
    }
    CHECK(sides==16);
  }
}

TEST_CASE("Pathological partial rectangles cannot multiply the visual fill without bound", "[unit][surface-visual]") {
  rat::GreyboxFillMesh raw;
  for(int z=0;z<500;++z) append(raw,rat::build_floor_slab_fill_quads({{0,z},2,0.25f+static_cast<float>(z)*.001f},1));
  const auto mesh=rat::build_surface_visual_mesh(raw);
  CHECK_FALSE(mesh.vertices.empty());
  CHECK(mesh.vertices.size()<=raw.vertices.size()*2+4096);
  // Every authored slab retains a top face, including those on fallback planes.
  std::set<int> top_tiles;
  for(const auto& vertex:mesh.vertices) if(vertex.ny>0.9f && vertex.y==2)
    top_tiles.insert(static_cast<int>(vertex.z));
  CHECK(top_tiles.size()==501);
}

TEST_CASE("Long flat strips process contour endpoints once instead of scanning every segment", "[unit][surface-visual]") {
  for(int tiles:{500,10000}) {
    rat::MapData map;map.schema_version=5;map.width=1;map.height=tiles;map.tile_size=1;
    map.height_grid={0,0,1,tiles,std::vector<float>(static_cast<std::size_t>(tiles),0)};
    const auto raw=rat::build_greybox_fill_mesh(map,{},false);
    REQUIRE(raw.vertices.size()==static_cast<std::size_t>(tiles)*4);
    const auto mesh=rat::build_surface_visual_mesh(raw);
    CHECK(mesh.contour_endpoint_events==static_cast<std::size_t>(tiles)*8);
    CHECK(mesh.contour_normal_checks<=static_cast<std::size_t>(tiles)*4);
    CHECK(mesh.contours.size()==static_cast<std::size_t>(tiles)*2+2);
    CHECK(contour_length(mesh)==Catch::Approx(2.0*(tiles+1)));
    for(const auto& edge:mesh.contours) {
      if(edge.a.z==edge.b.z) CHECK((edge.a.z==0 || edge.a.z==tiles));
    }
  }
}
