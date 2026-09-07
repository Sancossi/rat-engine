#include "editor_app.hpp"
#include "gui_observer.hpp"

#include <nlohmann/json.hpp>
#include <GLFW/glfw3.h>

#include <filesystem>
#include <fstream>
#include <iostream>
#include <stdexcept>

namespace fs = std::filesystem;
using Json = nlohmann::json;

namespace {
std::string utf8(const fs::path& path) {
  auto bytes = path.u8string(); return {bytes.begin(), bytes.end()};
}
struct Driver {
  rat::EditorApp app;
  rat::EditorFrameInput input;
  fs::path artifacts;
  int frames = 0;
  void frame(int count = 1) {
    for (int i = 0; i < count; ++i) {
      app.step_frame(input, 1.0f / 60.0f);
      ++frames;
      input.characters.clear(); input.events.clear();
      input.wheel_x = input.wheel_y = 0;
      input.close_requested = false;
    }
  }
  void key(int code, bool control = false) {
    input.keys[GLFW_KEY_LEFT_CONTROL] = control;
    input.keys[static_cast<std::size_t>(code)] = true; frame();
    input.keys[static_cast<std::size_t>(code)] = false; input.keys[GLFW_KEY_LEFT_CONTROL] = false; frame(2);
  }
  rat::GuiItemObservation find(const std::string& window, const std::string& label, bool allow_disabled = false) {
    for (int attempt = 0; attempt < 45; ++attempt) {
      std::optional<rat::GuiItemObservation> target, region;
      for (const auto& item : rat::gui_items()) {
        if (!window.empty() && item.window != window && !item.window.starts_with(window + "/")) continue;
        if (item.visible) region = item;
        if (item.label != label) continue;
        target = item;
        if (item.visible && (item.enabled || allow_disabled)) return item;
      }
      if (target && !target->enabled && !allow_disabled) throw std::runtime_error("Disabled selector " + window + "/" + label);
      if (!region) break;
      input.cursor_x = (region->clip_min_x + region->clip_max_x) * 0.5;
      input.cursor_y = (region->clip_min_y + region->clip_max_y) * 0.5;
      frame(2);
      input.wheel_y = target ? (target->min_y < target->clip_min_y ? 3.0f : -3.0f) : (attempt < 23 ? -3.0f : 3.0f);
      frame(3);
    }
    throw std::runtime_error("Missing visible enabled selector " + window + "/" + label);
  }
  void click(const std::string& window, const std::string& label) {
    const auto item = find(window, label);
    input.cursor_x = (std::max(item.min_x,item.clip_min_x) + std::min(item.max_x,item.clip_max_x)) * 0.5;
    input.cursor_y = (std::max(item.min_y,item.clip_min_y) + std::min(item.max_y,item.clip_max_y)) * 0.5;
    frame(2); input.mouse_buttons[0] = true; frame();
    input.mouse_buttons[0] = false; frame(2);
  }
  void text(const std::string& window, const std::string& label, std::u32string_view value, bool settle = true) {
    click(window,label); key(GLFW_KEY_A,true);
    input.characters.assign(value.begin(),value.end()); frame(2);
    if (settle) key(GLFW_KEY_TAB);
  }
  void click_at(float x, float y, int button = 0) {
    input.cursor_x = x; input.cursor_y = y; frame(2);
    input.mouse_buttons[static_cast<std::size_t>(button)] = true; frame();
    input.mouse_buttons[static_cast<std::size_t>(button)] = false; frame(2);
  }
  void world_click(rat::Vec3 position, int button = 0) {
    const auto pixel = app.project_world(position);
    if (!pixel) throw std::runtime_error("World point not projectable");
    click_at(pixel->x, pixel->y, button);
  }
  void require(bool condition, const std::string& message) { if (!condition) throw std::runtime_error(message); }
  void enter_edit() {
    if (app.observed_mode() != rat::AppMode::Edit) click("Inspector","Enter Edit (F2)");
  }
  void add_event() {
    enter_edit(); click("Inspector","Events"); click("Inspector","Add stub event");
  }
  void capture(const std::string& name) {
    app.request_capture(utf8(artifacts / (name + ".png")));
    for (int i = 0; i < 30; ++i) {
      frame(); const auto result = app.capture_result();
      if (result.complete) {
        if (!result.ok) throw std::runtime_error(result.error);
        return;
      }
    }
    throw std::runtime_error("Screenshot callback timeout");
  }
  void debug(const std::string& name) {
    Json items = Json::array();
    for (const auto& item : rat::gui_items()) items.push_back({{"window",item.window},{"label",item.label},
      {"visible",item.visible},{"enabled",item.enabled},{"rect",{item.min_x,item.min_y,item.max_x,item.max_y}}});
    const auto& doc = app.observed_document();
    Json snapshot{{"frames",frames},{"dirty",doc.dirty()},{"can_undo",doc.can_undo()},
      {"can_redo",doc.can_redo()},{"modal",app.observed_modal()},{"error",app.observed_error()},
      {"renderer",app.renderer_name()},{"items",items}};
    std::ofstream(artifacts / (name + ".json")) << snapshot.dump(2);
  }
};
}

int main(int argc, char** argv) {
  fs::path data = fs::absolute(fs::path(argv[0])).parent_path() / "data";
  fs::path user, artifacts;
  std::string scenario = "infrastructure";
  rat::RendererMode renderer = rat::RendererMode::SoftwareD3D11;
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 >= argc) { std::cerr << "Missing argument value\n"; return 2; }
    const std::string key = argv[i], value = argv[i + 1];
    if (key == "--data-root") data = fs::path(std::u8string(value.begin(), value.end()));
    else if (key == "--user-data-dir") user = fs::path(std::u8string(value.begin(), value.end()));
    else if (key == "--artifact-dir") artifacts = fs::path(std::u8string(value.begin(), value.end()));
    else if (key == "--scenario") scenario = value;
    else if (key == "--renderer") {
      if (value == "software-d3d11") renderer = rat::RendererMode::SoftwareD3D11;
      else if (value == "software-opengl") renderer = rat::RendererMode::SoftwareOpenGL;
      else { std::cerr << "Unknown renderer\n"; return 2; }
    } else { std::cerr << "Unknown argument " << key << '\n'; return 2; }
  }
  if (user.empty() || artifacts.empty()) { std::cerr << "Explicit --user-data-dir and --artifact-dir required\n"; return 2; }
  user = fs::absolute(user); artifacts = fs::absolute(artifacts); data = fs::absolute(data);
  fs::create_directories(user); fs::create_directories(artifacts);
  Driver driver; driver.artifacts = artifacts;
  Json report{{"scenarios",Json::array()}};
  try {
    const auto map = user / "gui-map.json";
    fs::remove(user/"imgui.ini");
    if (scenario == "infrastructure") fs::copy_file(data / "maps/grey_yard.json", map, fs::copy_options::overwrite_existing);
    else {
      Json fixture{{"schema_version",5},{"id","gui_fixture"},{"width",16},{"height",16},{"tile_size",1.0},
        {"height_grid",{{"origin_x",-4},{"origin_z",-4},{"width",16},{"height",16},{"ground_y",std::vector<float>(256,0)}}},
        {"occupancy",Json::array()},{"events",Json::array()}};
      std::ofstream(map) << fixture.dump(2);
    }
    rat::EditorLaunchOptions options{utf8(data),utf8(user),utf8(map),utf8(user/"save.json"),
        utf8(artifacts/"editor.log"),utf8(artifacts/"snapshot.json"),utf8(user/"imgui.ini")};
    rat::EditorInitialState initial;
    initial.renderer = renderer; initial.automation_layout = true; initial.hidden_window = true;
    if (!driver.app.init(options, initial)) throw std::runtime_error("Real editor initialization failed");
    driver.frame(5);
    if (scenario == "infrastructure") {
    driver.click("Inspector", "Enter Edit (F2)");
    if (driver.app.observed_mode() != rat::AppMode::Edit) throw std::runtime_error("Real mode button did not enter Edit");
    driver.click("Inspector", "Map to open");
    driver.key(GLFW_KEY_A, true);
    driver.input.characters = {'a','b','c'}; driver.frame(2);
    driver.input.events = {{rat::EditorInputEvent::Kind::Key, GLFW_KEY_BACKSPACE, true},
                          {rat::EditorInputEvent::Kind::Key, GLFW_KEY_BACKSPACE, false}};
    driver.frame(3);
    if (driver.app.observed_open_path() != "ab") throw std::runtime_error("Ordered native-style quick Backspace tap lost");
    driver.key(GLFW_KEY_TAB);
    report["scenarios"].push_back({{"name","ordered-quick-key-tap"},{"status","passed"}});
    driver.capture("first-edit-frame"); driver.debug("first-edit-frame");
    report["scenarios"].push_back({{"name","first-real-frame"},{"status","passed"}});
    } else if (scenario == "input-order") {
      driver.input.events = {{rat::EditorInputEvent::Kind::Key, GLFW_KEY_F2, true},
                             {rat::EditorInputEvent::Kind::Key, GLFW_KEY_F2, false}};
      driver.frame(5);
      driver.require(driver.app.observed_mode() == rat::AppMode::Edit, "Quick F2 was lost or repeated");
      driver.click("Inspector","Events");
      driver.click("Inspector","Place event##viewport_tool");
      const auto point = driver.app.project_world({-3.5f,0.0f,0.5f});
      driver.require(point.has_value(), "Quick-click world projection missing");
      driver.input.cursor_x = point->x; driver.input.cursor_y = point->y; driver.frame(2);
      driver.input.events = {{rat::EditorInputEvent::Kind::MouseButton,0,true},
                             {rat::EditorInputEvent::Kind::MouseButton,0,false}};
      driver.frame(5);
      driver.require(driver.app.observed_document().data().events.size() == 1, "Quick viewport click was lost or repeated");
      driver.input.events = {{rat::EditorInputEvent::Kind::Key, GLFW_KEY_F5, true},
                             {rat::EditorInputEvent::Kind::Key, GLFW_KEY_F5, false}};
      driver.frame(5);
      driver.require(driver.app.observed_modal(), "Quick F5 did not reach unsaved guard");
      driver.click("Unsaved changes","Cancel"); driver.frame(5);
      driver.require(!driver.app.observed_modal(), "Quick F5 repeated after modal dismissal");
      for (const bool close : {false,true}) {
        driver.text("Inspector","Event ID",U"queued",false);
        const auto tick = driver.app.observed_session().tick_id();
        driver.input.events = {{rat::EditorInputEvent::Kind::Key,GLFW_KEY_A,true},
          {rat::EditorInputEvent::Kind::Character,'a'}, {rat::EditorInputEvent::Kind::Key,GLFW_KEY_A,false},
          {rat::EditorInputEvent::Kind::Key,GLFW_KEY_B,true}, {rat::EditorInputEvent::Kind::Character,'b'},
          {rat::EditorInputEvent::Kind::Key,GLFW_KEY_B,false}};
        if (close) driver.input.close_requested = true;
        else {
          driver.input.events.push_back({rat::EditorInputEvent::Kind::Key,GLFW_KEY_F5,true});
          driver.input.events.push_back({rat::EditorInputEvent::Kind::Key,GLFW_KEY_F5,false});
        }
        driver.frame();
        driver.require(!driver.app.observed_modal(), "Guard opened before interleaved input suffix was consumed");
        driver.frame(8);
        driver.require(driver.app.observed_modal(), "Drained queued action did not open modal");
        driver.require(driver.app.observed_document().data().events[0].id == "queuedab", "Queued text suffix lost before action");
        driver.require(driver.app.observed_session().tick_id() == tick, "Simulation advanced while draining destructive action");
        driver.capture(close ? "queued-close" : "queued-f5");
        driver.click("Unsaved changes","Cancel"); driver.frame(4);
        driver.require(driver.app.observed_document().data().events[0].id == "queuedab", "Queued text leaked after modal dismissal");
      }
      driver.debug("input-order");
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "authoring-text") {
      driver.add_event();
      driver.require(driver.app.observed_document().data().events.size() == 1, "Real Add stub event failed");
      driver.text("Inspector","Event ID", U"??????? ?");
      driver.require(driver.app.observed_document().data().events[0].id == "??????? ?", "Unicode authored field failed");
      driver.click("Inspector","Save current map JSON");
      driver.require(!driver.app.observed_document().dirty(), "UI save did not mark clean");
      driver.text("Inspector","Event ID",U"focused",false);
      driver.input.characters = {'!'};
      driver.input.keys[GLFW_KEY_F5] = true; driver.frame();
      driver.input.keys[GLFW_KEY_F5] = false; driver.frame(2);
      driver.require(driver.app.observed_modal(), "F5 did not open unsaved modal");
      driver.require(driver.app.observed_document().data().events[0].id == "focused!", "Same-frame F5 lost focused text");
      driver.capture("focused-f5-modal");
      driver.click("Unsaved changes","Cancel");
      driver.require(!driver.app.observed_modal(), "Cancel did not dismiss modal");
      driver.text("Inspector","Event ID",U"closing",false);
      driver.input.characters = {'!'}; driver.input.close_requested = true; driver.frame(3);
      driver.require(driver.app.observed_modal(), "Native close did not open modal");
      driver.require(driver.app.observed_document().data().events[0].id == "closing!", "Same-frame close lost focused text");
      driver.click("Unsaved changes","Cancel");
      driver.capture("authoring-text"); driver.debug("authoring-text");
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else throw std::runtime_error("Unknown scenario " + scenario);
    report["renderer"] = driver.app.renderer_name();
    report["status"] = "passed";
    std::ofstream(artifacts/"scenario-report.json") << report.dump(2);
    return 0;
  } catch (const std::exception& error) {
    report["status"] = "failed"; report["error"] = error.what();
    driver.debug("failure");
    try { if (driver.app.observed_running()) driver.capture("failure"); } catch (...) {}
    std::ofstream(artifacts/"scenario-report.json") << report.dump(2);
    std::cerr << error.what() << '\n'; return 1;
  }
}
