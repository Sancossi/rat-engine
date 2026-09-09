#include <expedition/scene.hpp>
#include <expedition/session.hpp>
#include "../apps/game/launch_options.hpp"
#include <catch2/catch_test_macros.hpp>
#include <catch2/matchers/catch_matchers_string.hpp>
#include <nlohmann/json.hpp>
#include <atomic>
#include <chrono>
#include <fstream>
#include <filesystem>

namespace {
using Json=nlohmann::json;
using namespace rat::expedition;
struct Fixture {
  std::filesystem::path root;
  Fixture() {
    static std::atomic<unsigned> sequence=0;
    root=std::filesystem::temp_directory_path()/ ("rat-expedition-test-"+std::to_string(std::chrono::steady_clock::now().time_since_epoch().count())+"-"+std::to_string(sequence++));
    std::filesystem::create_directories(root);
    std::filesystem::copy(std::filesystem::path(RAT_TEST_DATA_DIR)/"expedition",root,std::filesystem::copy_options::recursive);
  }
  ~Fixture() { std::error_code ignored; std::filesystem::remove_all(root,ignored); }
  Json read(const char* path) { std::ifstream file(root/path); return Json::parse(file); }
  void write(const char* path,const Json& value) { std::ofstream(root/path)<<value.dump(2); }
};
}
TEST_CASE("Expedition project loads independent scene data and stable spawn", "[expedition]") {
  const auto project=load_project(std::filesystem::path(RAT_TEST_DATA_DIR)/"expedition");
  REQUIRE(project.start_scene=="expedition_courtyard");
  REQUIRE(project.scenes.size()==1);
  ExpeditionSession session(project);
  const auto before=session.snapshot();
  for(int i=0;i<120;++i)session.tick({0,0});
  const auto after=session.snapshot();
  REQUIRE(after.tick==120);
  REQUIRE(after.leader.x==before.leader.x);
  REQUIRE(after.leader.y==before.leader.y);
  REQUIRE(after.leader.z==before.leader.z);
}
TEST_CASE("Expedition movement uses fixed camera-relative simulation", "[expedition]") {
  const auto project=load_project(std::filesystem::path(RAT_TEST_DATA_DIR)/"expedition");
  ExpeditionSession first(project),second(project);
  const auto before=first.snapshot();
  for(int i=0;i<60;++i) { first.tick({1,0}); second.tick({1,0}); }
  const auto a=first.snapshot(),b=second.snapshot();
  REQUIRE(a.leader.x==b.leader.x); REQUIRE(a.leader.z==b.leader.z);
  REQUIRE(a.leader.x!=before.leader.x); REQUIRE(a.leader.z!=before.leader.z);
  REQUIRE(a.direction==2);
}
TEST_CASE("Expedition rejects path escape and missing data", "[expedition]") {
  Fixture fixture;
  for(const std::string path : {"../maps/test.json","/tmp/test.json","C:/test.json","maps\\test.json","maps/../test.json",""})
    REQUIRE_THROWS(asset_path(fixture.root,path));
  std::filesystem::remove(fixture.root/"sprites/rat.png");
  REQUIRE_THROWS_WITH(load_project(fixture.root),Catch::Matchers::ContainsSubstring("Missing sprite PNG"));
}
TEST_CASE("Expedition rejects symlink escape when supported", "[expedition]") {
  Fixture fixture;
  std::error_code error;
  std::filesystem::create_directory_symlink(std::filesystem::temp_directory_path(),fixture.root/"outside",error);
  if(error) { SKIP("Creating symlinks requires local platform privilege"); }
  REQUIRE_THROWS(asset_path(fixture.root,"outside/escaped.json"));
}
TEST_CASE("Expedition rejects malformed metadata and map identity", "[expedition]") {
  Fixture fixture;
  auto data=fixture.read("scenes/courtyard.json");
  SECTION("scene id") { data["scene_id"]="other"; }
  SECTION("entry required") { data["spawns"].erase("entry"); }
  SECTION("numeric coordinates") { data["spawns"]["entry"]["y"]="zero"; }
  SECTION("blocked spawn") { data["spawns"]["entry"]={{"x",2},{"y",0},{"z",-.5}}; }
  SECTION("unsupported spawn") { data["spawns"]["entry"]["y"]=9; }
  SECTION("wrong schema") { data["schema_version"]=2; }
  SECTION("unimplemented interaction is not silently accepted") { data["portals"].push_back({{"id","door"}}); }
  fixture.write("scenes/courtyard.json",data);
  REQUIRE_THROWS(load_project(fixture.root));
}
TEST_CASE("Expedition preserves strict duplicate-key rejection", "[expedition]") {
  Fixture fixture;
  SECTION("project") {
    std::ofstream(fixture.root/"project.json")<<"{\"schema_version\":1,\"schema_version\":1}";
  }
  SECTION("map") {
    auto text=fixture.read("maps/courtyard.json").dump();
    text.insert(1,"\"id\":\"bad\",");
    std::ofstream(fixture.root/"maps/courtyard.json")<<text;
  }
  REQUIRE_THROWS(load_project(fixture.root));
}
TEST_CASE("Game launch defaults use executable directory and validate CLI", "[expedition]") {
  const auto options=parse_launch_options({"--frames","120","--hidden","--width","1920","--height","1080"},"/package/rat-game.exe");
  REQUIRE(options.data_dir==std::filesystem::path("/package/data/expedition"));
  REQUIRE(options.hidden); REQUIRE(options.frames==120); REQUIRE(options.width==1920);
  for(const std::vector<std::string> args : {std::vector<std::string>{"--frames","-1"},{"--frames","1junk"},{"--frames","0"},{"--width","1"},{"--renderer","fake"},{"--screenshot","out.png"},{"--unknown","value"},{"--frames"}})
    REQUIRE_THROWS(parse_launch_options(args,"/package/rat-game.exe"));
}
