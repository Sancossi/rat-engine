#include <rat/entity.hpp>

#include <catch2/catch_test_macros.hpp>

#include <vector>

TEST_CASE("EntityRegistry create returns a live generation-tagged id", "[unit][entity]") {
  rat::EntityRegistry entities;
  const rat::EntityId id = entities.create();
  REQUIRE(id.valid());
  REQUIRE(entities.alive(id));
}

TEST_CASE("destroy makes the same EntityId stale", "[unit][entity]") {
  rat::EntityRegistry entities;
  const rat::EntityId id = entities.create();
  entities.destroy(id);
  REQUIRE_FALSE(entities.alive(id));
  REQUIRE_FALSE(entities.alive(rat::EntityId{}));
}

TEST_CASE("reused index bumps generation so the old id stays dead", "[unit][entity]") {
  rat::EntityRegistry entities;
  const rat::EntityId first = entities.create();
  entities.destroy(first);
  const rat::EntityId second = entities.create();
  REQUIRE(second.index == first.index);
  REQUIRE(second.generation != first.generation);
  REQUIRE(second.generation > first.generation);
  REQUIRE(entities.alive(second));
  REQUIRE_FALSE(entities.alive(first));
}

TEST_CASE("ComponentStore insert and get Transform by live EntityId", "[unit][entity]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  const rat::EntityId id = entities.create();
  rat::Transform xf;
  xf.position.x = 1.0f;
  xf.position.y = 2.0f;
  xf.position.z = 3.0f;
  REQUIRE(transforms.insert(entities, id, xf));
  const rat::Transform* got = transforms.get(entities, id);
  REQUIRE(got != nullptr);
  CHECK(got->position.x == 1.0f);
  CHECK(got->position.y == 2.0f);
  CHECK(got->position.z == 3.0f);
}

TEST_CASE("ComponentStore rejects stale EntityId after destroy", "[unit][entity]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  const rat::EntityId id = entities.create();
  REQUIRE(transforms.insert(entities, id, rat::Transform{}));
  entities.destroy(id);
  REQUIRE(transforms.get(entities, id) == nullptr);
  REQUIRE_FALSE(transforms.insert(entities, id, rat::Transform{}));
}

TEST_CASE("generation bump hides leftover Transform from the new occupant", "[unit][entity]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  const rat::EntityId first = entities.create();
  rat::Transform old_xf;
  old_xf.position.x = 9.0f;
  REQUIRE(transforms.insert(entities, first, old_xf));
  entities.destroy(first);

  const rat::EntityId second = entities.create();
  REQUIRE(second.index == first.index);
  REQUIRE(transforms.get(entities, first) == nullptr);
  REQUIRE(transforms.get(entities, second) == nullptr);

  rat::Transform neu;
  neu.position.x = 4.0f;
  REQUIRE(transforms.insert(entities, second, neu));
  const rat::Transform* got = transforms.get(entities, second);
  REQUIRE(got != nullptr);
  CHECK(got->position.x == 4.0f);
  REQUIRE(transforms.get(entities, first) == nullptr);
}

TEST_CASE("explicit iterate walks Transform Renderable Collider stores", "[unit][entity]") {
  rat::EntityRegistry entities;
  rat::ComponentStore<rat::Transform> transforms;
  rat::ComponentStore<rat::Renderable> renderables;
  rat::ComponentStore<rat::Collider> colliders;

  const rat::EntityId a = entities.create();
  const rat::EntityId b = entities.create();
  rat::Transform xa;
  xa.position.x = 1.0f;
  rat::Transform xb;
  xb.position.x = 2.0f;
  REQUIRE(transforms.insert(entities, a, xa));
  REQUIRE(transforms.insert(entities, b, xb));

  rat::Renderable mesh;
  mesh.mesh = rat::make_asset_id("mesh/crate");
  REQUIRE(renderables.insert(entities, a, mesh));

  rat::Collider solid;
  solid.radius = 0.5f;
  solid.height = 1.0f;
  REQUIRE(colliders.insert(entities, b, solid));

  std::vector<float> xs;
  transforms.for_each(entities, [&](rat::EntityId id, const rat::Transform& xf) {
    REQUIRE(entities.alive(id));
    xs.push_back(xf.position.x);
  });
  REQUIRE(xs.size() == 2);

  int rendered = 0;
  renderables.for_each(entities, [&](rat::EntityId, const rat::Renderable& r) {
    ++rendered;
    CHECK(r.mesh.key() == "mesh/crate");
  });
  REQUIRE(rendered == 1);

  int hit = 0;
  colliders.for_each(entities, [&](rat::EntityId, const rat::Collider& c) {
    ++hit;
    CHECK(c.radius == 0.5f);
    CHECK(c.height == 1.0f);
  });
  REQUIRE(hit == 1);
}
