#include <rat/asset.hpp>
#include <rat/map_document.hpp>
#include <rat/map_loader.hpp>

#include <catch2/catch_test_macros.hpp>

#include <cstdint>
#include <string>
#include <vector>

TEST_CASE("catalog keys produce stable comparable AssetIds", "[unit][asset]") {
  const rat::AssetId a = rat::make_asset_id("tex/ground");
  const rat::AssetId b = rat::make_asset_id("tex/ground");
  const rat::AssetId other = rat::make_asset_id("sfx/jump");

  REQUIRE(a.valid());
  REQUIRE(a == b);
  REQUIRE(a != other);
  REQUIRE(a.key() == "tex/ground");
}

TEST_CASE("unregistered AssetId resolve is Failed and does not throw", "[unit][asset]") {
  rat::MemoryAssetLoader loader;
  rat::AssetRegistry registry(loader);
  const rat::AssetId missing = rat::make_asset_id("missing/tex");

  REQUIRE(registry.state(missing) == rat::AssetState::Failed);

  rat::AssetView view;
  REQUIRE_NOTHROW(view = registry.resolve(missing));
  CHECK(view.state == rat::AssetState::Failed);
  CHECK_FALSE(view.gpu.valid());
  CHECK(view.cpu.bytes.empty());
  CHECK_FALSE(view.error.empty());
}

TEST_CASE("memory loader loads registered texture to Ready without filesystem", "[unit][asset]") {
  rat::MemoryAssetLoader loader;
  const rat::AssetId id = rat::make_asset_id("tex/ground");
  rat::AssetCpuData cpu;
  cpu.bytes = {1, 2, 3};
  loader.set(id, cpu);

  rat::AssetRegistry registry(loader);
  rat::AssetCatalogEntry entry;
  entry.id = id;
  entry.kind = rat::AssetKind::Texture;
  entry.debug_name = "ground";
  entry.path.source = "never/opened.png";
  registry.register_asset(entry);

  REQUIRE(registry.state(id) == rat::AssetState::Unloaded);
  registry.request_load(id);
  REQUIRE(registry.state(id) == rat::AssetState::Loading);
  registry.pump_loads();
  REQUIRE(registry.state(id) == rat::AssetState::Ready);

  const rat::AssetView view = registry.resolve(id);
  CHECK(view.state == rat::AssetState::Ready);
  CHECK(view.debug_name == "ground");
  CHECK(view.kind == rat::AssetKind::Texture);
  CHECK(view.cpu.bytes == std::vector<std::uint8_t>{1, 2, 3});
  CHECK(view.gpu.valid());
  CHECK(view.resolved_id == id);
  CHECK(view.id == id);
}

TEST_CASE("failed load uses fallback and does not abort", "[unit][asset]") {
  rat::MemoryAssetLoader loader;
  const rat::AssetId broken = rat::make_asset_id("tex/broken");
  const rat::AssetId fallback = rat::make_asset_id("tex/fallback");
  rat::AssetCpuData cpu;
  cpu.bytes = {9};
  loader.set(fallback, cpu);
  loader.set_fail(broken, "corrupt png");

  rat::AssetRegistry registry(loader);

  rat::AssetCatalogEntry fb;
  fb.id = fallback;
  fb.kind = rat::AssetKind::Texture;
  fb.debug_name = "fallback";
  registry.register_asset(fb);

  rat::AssetCatalogEntry bad;
  bad.id = broken;
  bad.kind = rat::AssetKind::Texture;
  bad.debug_name = "broken";
  bad.fallback = fallback;
  registry.register_asset(bad);

  registry.request_load(fallback);
  registry.request_load(broken);
  registry.pump_loads();

  REQUIRE(registry.state(broken) == rat::AssetState::Failed);
  REQUIRE(registry.error(broken).find("corrupt") != std::string::npos);

  const rat::AssetView view = registry.resolve(broken);
  CHECK(view.state == rat::AssetState::Ready);
  CHECK(view.resolved_id == fallback);
  CHECK(view.id == broken);
  CHECK(view.cpu.bytes == std::vector<std::uint8_t>{9});
  CHECK(view.debug_name == "fallback");
}

TEST_CASE("audio mesh and material descriptors load like textures", "[unit][asset]") {
  rat::MemoryAssetLoader loader;
  const rat::AssetId clip = rat::make_asset_id("sfx/jump");
  const rat::AssetId mesh = rat::make_asset_id("mesh/crate");
  const rat::AssetId material = rat::make_asset_id("mat/default");
  loader.set(clip, rat::AssetCpuData{{10}});
  loader.set(mesh, rat::AssetCpuData{{11}});
  loader.set(material, rat::AssetCpuData{{12}});

  rat::AssetRegistry registry(loader);
  registry.register_asset({clip, rat::AssetKind::AudioClip, "jump", {}, {}});
  registry.register_asset({mesh, rat::AssetKind::MeshDescriptor, "crate", {}, {}});
  registry.register_asset({material, rat::AssetKind::MaterialDescriptor, "default", {}, {}});

  registry.request_load(clip);
  registry.request_load(mesh);
  registry.request_load(material);
  registry.pump_loads();

  CHECK(registry.resolve(clip).kind == rat::AssetKind::AudioClip);
  CHECK(registry.resolve(mesh).kind == rat::AssetKind::MeshDescriptor);
  CHECK(registry.resolve(material).kind == rat::AssetKind::MaterialDescriptor);
  CHECK(registry.resolve(clip).state == rat::AssetState::Ready);
  CHECK(registry.resolve(mesh).state == rat::AssetState::Ready);
  CHECK(registry.resolve(material).state == rat::AssetState::Ready);
}

TEST_CASE("hot reload updates CPU and retires old GPU after end_frame", "[unit][asset]") {
  rat::MemoryAssetLoader loader;
  const rat::AssetId id = rat::make_asset_id("tex/ground");
  loader.set(id, rat::AssetCpuData{{1}});

  rat::AssetRegistry registry(loader);
  rat::AssetCatalogEntry entry;
  entry.id = id;
  entry.kind = rat::AssetKind::Texture;
  entry.debug_name = "ground";
  registry.register_asset(entry);
  registry.request_load(id);
  registry.pump_loads();

  const rat::GpuHandle old_gpu = registry.resolve(id).gpu;
  REQUIRE(old_gpu.valid());
  REQUIRE(registry.gpu_status(old_gpu) == rat::GpuHandleStatus::Live);

  loader.set(id, rat::AssetCpuData{{2, 2}});
  registry.hot_reload(id);
  REQUIRE(registry.state(id) == rat::AssetState::Loading);
  registry.pump_loads();

  const rat::AssetView view = registry.resolve(id);
  REQUIRE(view.state == rat::AssetState::Ready);
  CHECK(view.cpu.bytes == std::vector<std::uint8_t>{2, 2});
  CHECK(view.gpu.valid());
  CHECK(view.gpu != old_gpu);
  CHECK(registry.gpu_status(old_gpu) == rat::GpuHandleStatus::PendingDestroy);

  registry.end_frame();
  CHECK(registry.gpu_status(old_gpu) == rat::GpuHandleStatus::Destroyed);
  CHECK(registry.gpu_status(view.gpu) == rat::GpuHandleStatus::Live);
}

TEST_CASE("resolve view has no source path; gameplay uses AssetId only", "[unit][asset]") {
  rat::MemoryAssetLoader loader;
  const rat::AssetId id = rat::make_asset_id("tex/ground");
  loader.set(id, rat::AssetCpuData{{1}});

  rat::AssetRegistry registry(loader);
  rat::AssetCatalogEntry entry;
  entry.id = id;
  entry.kind = rat::AssetKind::Texture;
  entry.debug_name = "ground";
  entry.path.source = "editor/only/ground.png";
  registry.register_asset(entry);
  registry.request_load(id);
  registry.pump_loads();

  const rat::AssetView view = registry.resolve(id);
  CHECK(view.path.source.empty());
  CHECK(registry.source_path(id).source == "editor/only/ground.png");
}

TEST_CASE("map JSON round-trips stable AssetIds for gameplay kinds", "[unit][asset][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "asset_map",
    "width": 2,
    "height": 2,
    "assets": [
      { "id": "tex/ground", "kind": "texture", "debug_name": "ground" },
      { "id": "sfx/jump", "kind": "audio_clip" },
      { "id": "mesh/crate", "kind": "mesh" },
      { "id": "mat/default", "kind": "material" }
    ]
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);
  REQUIRE(loaded.map.assets.size() == 4);
  CHECK(loaded.map.assets[0].id == rat::make_asset_id("tex/ground"));
  CHECK(loaded.map.assets[0].kind == rat::AssetKind::Texture);
  CHECK(loaded.map.assets[0].debug_name == "ground");
  CHECK(loaded.map.assets[1].kind == rat::AssetKind::AudioClip);
  CHECK(loaded.map.assets[2].kind == rat::AssetKind::MeshDescriptor);
  CHECK(loaded.map.assets[3].kind == rat::AssetKind::MaterialDescriptor);

  const rat::MapSerializeResult serialized = rat::serialize_map_to_string(loaded.map);
  REQUIRE(serialized.ok);
  const rat::MapLoadResult again = rat::load_map_from_string(serialized.json_text);
  REQUIRE(again.ok);
  REQUIRE(again.map.assets.size() == 4);
  CHECK(again.map.assets[0].id == loaded.map.assets[0].id);
  CHECK(again.map.assets[1].id == loaded.map.assets[1].id);
}

TEST_CASE("gameplay binds map AssetIds through a headless registry", "[unit][asset][map]") {
  constexpr const char* kJson = R"({
    "schema_version": 1,
    "id": "asset_map",
    "width": 2,
    "height": 2,
    "assets": [
      { "id": "tex/ground", "kind": "texture", "debug_name": "ground" },
      { "id": "sfx/jump", "kind": "audio_clip" }
    ]
  })";

  const rat::MapLoadResult loaded = rat::load_map_from_string(kJson);
  REQUIRE(loaded.ok);

  rat::MemoryAssetLoader loader;
  loader.set(rat::make_asset_id("tex/ground"), rat::AssetCpuData{{4}});
  loader.set(rat::make_asset_id("sfx/jump"), rat::AssetCpuData{{5}});
  rat::AssetRegistry registry(loader);

  rat::bind_map_assets(registry, loaded.map);
  registry.pump_loads();

  const rat::AssetId ground = loaded.map.assets[0].id;
  const rat::AssetView view = registry.resolve(ground);
  CHECK(view.state == rat::AssetState::Ready);
  CHECK(view.cpu.bytes == std::vector<std::uint8_t>{4});
  CHECK(registry.resolve(loaded.map.assets[1].id).kind == rat::AssetKind::AudioClip);
}

TEST_CASE("empty map asset id fails document validation", "[unit][asset][mapdoc]") {
  rat::MapData map;
  map.schema_version = 1;
  map.id = "bad_assets";
  map.width = 2;
  map.height = 2;
  map.height_grid.width = 2;
  map.height_grid.height = 2;
  map.height_grid.ground_y.assign(4, 0.0f);
  map.assets.push_back(rat::MapAssetRef{});

  const std::vector<rat::MapIssue> issues = rat::validate_map_document(map);
  REQUIRE(rat::map_issues_have_errors(issues));
  bool found = false;
  for (const rat::MapIssue& issue : issues) {
    if (issue.json_path == "/assets/0/id") {
      found = true;
      CHECK(issue.severity == rat::MapIssueSeverity::Error);
    }
  }
  CHECK(found);
}
