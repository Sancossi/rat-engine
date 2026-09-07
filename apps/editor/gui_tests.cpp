#include "editor_app.hpp"
#include "gui_observer.hpp"
#include <rat/map_loader.hpp>
#include <rat/save_game.hpp>
#include <rat/collision.hpp>
#include <rat/authoring_snapshot.hpp>
#if defined(_WIN32)
#include <windows.h>
#include <shellapi.h>
#endif

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
std::u32string decode_utf8(std::string_view bytes) {
  std::u32string out;
  for (std::size_t i = 0; i < bytes.size();) {
    const auto first = static_cast<unsigned char>(bytes[i++]);
    char32_t code = first; int remaining = 0;
    if (first >= 0xf0) { code = first & 7; remaining = 3; }
    else if (first >= 0xe0) { code = first & 15; remaining = 2; }
    else if (first >= 0xc0) { code = first & 31; remaining = 1; }
    while (remaining-- > 0) {
      if (i == bytes.size()) throw std::runtime_error("Invalid UTF-8 fixture");
      code = (code << 6) | (static_cast<unsigned char>(bytes[i++]) & 63);
    }
    out.push_back(code);
  }
  return out;
}
Json read_json(const fs::path& path) { std::ifstream stream(path); return Json::parse(stream); }
struct FailingMapStore final : rat::FileStore {
  std::string target;
  bool fail_once = false;
  int attempts = 0;
  rat::FileReadResult read(std::string_view path) const override { return rat::os_files().read(path); }
  rat::FileWriteResult write(std::string_view path, std::string_view bytes) override { return rat::os_files().write(path,bytes); }
  rat::FileWriteResult write_atomic(std::string_view path, std::string_view bytes) override {
    if (path == target) {
      ++attempts;
      if (fail_once) { fail_once = false; return {false,"Injected named-target save failure"}; }
    }
    return rat::os_files().write_atomic(path,bytes);
  }
};
struct Driver {
  explicit Driver(rat::FileStore& files = rat::os_files()) : app(files) {}
  rat::EditorApp app;
  rat::EditorFrameInput input;
  fs::path artifacts;
  int frames = 0;
  Json trace = Json::array();
  void frame(int count = 1) {
    for (int i = 0; i < count; ++i) {
      app.step_frame(input, 1.0f / 60.0f);
      ++frames;
      const auto& processed = app.observed_processed_input();
      trace.push_back({{"frame",frames},{"mouse",{processed.cursor_x,processed.cursor_y,processed.mouse_buttons[0]}},
        {"capture_mouse",app.observed_capture_mouse()},{"events",app.observed_document().data().events.size()},
        {"player",{app.observed_session().player().x,app.observed_session().player().y,app.observed_session().player().z}},
        {"tick",app.observed_session().tick_id()}});
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
  void drag(const rat::GuiItemObservation& from, float target_x, float target_y, bool detour = false) {
    input.cursor_x = (from.min_x+from.max_x)*0.5; input.cursor_y = (from.min_y+from.max_y)*0.5;
    frame(2); input.mouse_buttons[0] = true; frame();
    if (detour) { input.cursor_x += 35; input.cursor_y += 20; frame(2); }
    input.cursor_x = target_x; input.cursor_y = target_y; frame(2);
    input.mouse_buttons[0] = false; frame(3);
  }
  void connect(const std::string& from, const std::string& to) {
    const auto source = find("Event Graph","##out_"+from);
    const auto target = find("Event Graph","##in_"+to);
    drag(source,(target.min_x+target.max_x)*0.5f,(target.min_y+target.max_y)*0.5f);
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
  void open_path(const fs::path& path) {
    text("Inspector","Map to open",decode_utf8(utf8(path)));
    click("Inspector","Open map");
  }
  void action(const std::string& kind, const fs::path& alternative) {
    if (kind == "close") { input.close_requested = true; frame(4); }
    else if (kind == "f5") key(GLFW_KEY_F5);
    else if (kind == "reload") click("Inspector","Reload map (reset player)");
    else if (kind == "open") open_path(alternative);
    else throw std::runtime_error("Unknown guard action");
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
    const auto& session = app.observed_session();
    const auto& canvas = app.observed_canvas();
    const auto camera = app.observed_camera_pose();
    Json events = Json::array(), occupancy = Json::array();
    for (const auto& event : doc.data().events) {
      Json pages = Json::array();
      for (const auto& page : event.pages) {
        Json nodes = Json::array(), edges = Json::array();
        if (page.graph) {
          for (const auto& node : page.graph->nodes) {
            Json item{{"id",node.id},{"kind",node.kind},{"text",node.text},{"map_id",node.map_id}};
            if (node.layout) item["layout"] = {node.layout->x,node.layout->y};
            nodes.push_back(item);
          }
          for (const auto& edge : page.graph->edges) edges.push_back({{"from",edge.from},{"to",edge.to}});
        }
        pages.push_back({{"trigger",static_cast<int>(page.trigger)},{"nodes",nodes},{"edges",edges}});
      }
      Json item{{"id",event.id},{"pages",pages}};
      if (event.tile) item["tile"] = {event.tile->x,event.tile->z};
      events.push_back(item);
    }
    for (const auto& cell : doc.data().occupancy)
      occupancy.push_back({{"cell",{cell.x,cell.y,cell.z}},{"kind",static_cast<int>(cell.kind)},{"yaw",static_cast<int>(cell.yaw)}});
    Json snapshot{{"frames",frames},{"dirty",doc.dirty()},{"can_undo",doc.can_undo()},
      {"can_redo",doc.can_redo()},{"modal",app.observed_modal()},{"error",app.observed_error()},
      {"renderer",app.renderer_name()},{"items",items},{"trace",trace},
      {"map_id",doc.data().id},{"events",events},{"occupancy",occupancy},
      {"runtime_valid",app.observed_runtime_valid()},{"mode",static_cast<int>(app.observed_mode())},
      {"selected_event",doc.selected_event()},{"tick",session.tick_id()},
      {"player",{session.player().x,session.player().y,session.player().z}},
      {"jump",{{"grounded",session.jump().grounded},{"offset",session.jump().jump_offset},
        {"vertical_speed",session.jump().vertical_speed},{"climbing",session.jump().climbing}}},
      {"canvas",{{"wire_drag",canvas.dragging_wire},{"pending_from",canvas.pending_from},
        {"pan",{canvas.pan_x,canvas.pan_y}},{"zoom",canvas.zoom},{"display_scale",canvas.display_scale}}},
      {"logical_size",{input.logical_width,input.logical_height}},
      {"framebuffer_size",{input.framebuffer_width,input.framebuffer_height}}};
    snapshot["camera"] = {{"eye",{camera.eye.x,camera.eye.y,camera.eye.z}},
                          {"focus",{camera.focus.x,camera.focus.y,camera.focus.z}}};
    std::ofstream(artifacts / (name + ".json")) << snapshot.dump(2);
  }
};
}

int main(int argc, char** argv) {
  std::vector<std::string> arguments;
  fs::path executable;
#if defined(_WIN32)
  (void)argc; (void)argv;
  int argument_count = 0;
  auto** wide_arguments = CommandLineToArgvW(GetCommandLineW(), &argument_count);
  if (!wide_arguments) return 2;
  for (int i = 0; i < argument_count; ++i) arguments.push_back(utf8(fs::path(wide_arguments[i])));
  LocalFree(wide_arguments);
  std::wstring module(32768, L'\0');
  const auto length = GetModuleFileNameW(nullptr,module.data(),static_cast<DWORD>(module.size()));
  if (!length || length == module.size()) return 2;
  module.resize(length); executable = module;
#else
  for (int i = 0; i < argc; ++i) arguments.emplace_back(argv[i]);
  executable = fs::read_symlink("/proc/self/exe");
#endif
  fs::path data = executable.parent_path() / "data";
  fs::path user, artifacts;
  std::string scenario = "infrastructure";
  rat::RendererMode renderer = rat::RendererMode::SoftwareD3D11;
  for (std::size_t i = 1; i < arguments.size(); i += 2) {
    if (i + 1 >= arguments.size()) { std::cerr << "Missing argument value\n"; return 2; }
    const std::string key = arguments[i], value = arguments[i + 1];
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
  FailingMapStore files;
  files.target = utf8(user/"gui-map.json"); files.fail_once = scenario == "failed-save";
  Driver driver(files); driver.artifacts = artifacts;
  Json report{{"scenarios",Json::array()}};
  try {
    const auto map = user / "gui-map.json";
    fs::remove(user/"imgui.ini");
    if (scenario == "infrastructure") fs::copy_file(data / "maps/grey_yard.json", map, fs::copy_options::overwrite_existing);
    else {
      Json fixture{{"schema_version",5},{"id","gui_fixture"},{"width",16},{"height",16},{"tile_size",1.0},
        {"height_grid",{{"origin_x",-4},{"origin_z",-4},{"width",16},{"height",16},{"ground_y",std::vector<float>(256,0)}}},
        {"occupancy",Json::array()},{"events",Json::array()}};
      if (scenario.starts_with("graph") || scenario.starts_with("scale-") || scenario == "unsupported-touch" || scenario == "unsupported-transfer") {
        Json graph{{"nodes",{{{"id","text"},{"kind","show_text"},{"params",{{"text","Original"}}},{"layout",{{"x",180},{"y",24}}}}}},
                   {"edges",{{{"from","entry"},{"to","text"}},{{"from","text"},{"to","exit"}}}}};
        Json event{{"id","graph_event"},{"tile",{{"x",-3},{"z",0}}},
                   {"pages",{{{"trigger","action"},{"graph",graph}}}}};
        fixture["events"].push_back(event);
      }
      if (scenario.starts_with("ramp-")) fixture["occupancy"].push_back({{"x",0},{"y",2},{"z",0},{"kind","solid"}});
      if (scenario == "bridge-above") {
        fixture["occupancy"] = {{{"x",0},{"y",0},{"z",0},{"kind","ramp"},{"yaw","east"}},
                                 {{"x",1},{"y",1},{"z",0},{"kind","ramp"},{"yaw","east"}}};
        fixture["floor_slabs"] = {{{"tile",{{"x",2},{"z",0}}},{"top_y",2.0},{"thickness",0.25}}};
      }
      if (scenario == "bridge-under") fixture["floor_slabs"] = {{{"tile",{{"x",1},{"z",0}}},{"top_y",2.0},{"thickness",0.25}}};
      if (scenario == "bridge-filled") fixture["occupancy"] = {{{"x",1},{"y",0},{"z",0},{"kind","solid"}},{{"x",1},{"y",1},{"z",0},{"kind","solid"}}};
      if (scenario != "restart-add") std::ofstream(map) << fixture.dump(2);
      if (scenario.starts_with("unsupported-")) {
        auto unsupported = fixture;
        auto& page = unsupported["events"][0]["pages"][0];
        if (scenario == "unsupported-touch") page["trigger"] = "event_touch";
        else {
          page["graph"]["nodes"][0]["kind"] = "transfer_player";
          page["graph"]["nodes"][0]["params"] = {{"map_id","other_map"},{"x",0},{"y",0},{"z",0}};
        }
        std::ofstream(user/"unsupported.json") << unsupported.dump(2);
      }
      fixture["id"] = "alternative_fixture";
      std::ofstream(user/"alternative.json") << fixture.dump(2);
      fixture["id"] = "backup_fixture";
      std::ofstream(user/"gui-map.json.bak") << fixture.dump(2);
      std::ofstream(user/"malformed.json") << "{invalid";
      if (scenario == "malformed-start") std::ofstream(map) << "{invalid";
      if (scenario == "slot-backup") {
        rat::GameState slot; slot.set_map_id("gui_fixture"); slot.set_player_position(-1.5f,0,1.5f);
        std::string bytes; if (!slot.save_to_memory(bytes)) throw std::runtime_error("Slot fixture serialization failed");
        std::ofstream(user/"save.json") << bytes;
        slot.set_player_position(-2.5f,0,0.5f); slot.set_variable(7,42);
        if (!slot.save_to_memory(bytes)) throw std::runtime_error("Backup fixture serialization failed");
        std::ofstream(user/"save.json.bak") << bytes;
      }
    }
    rat::EditorLaunchOptions options{utf8(data),utf8(user),utf8(map),utf8(user/"save.json"),
        utf8(artifacts/"editor.log"),utf8(artifacts/"snapshot.json"),utf8(user/"imgui.ini")};
    rat::EditorInitialState initial;
    initial.renderer = renderer; initial.automation_layout = true; initial.hidden_window = true;
    if (scenario.starts_with("scale-")) {
      initial.ui_scale = std::stof(scenario.substr(6)) / 100.0f;
      driver.input.logical_width = static_cast<int>(1280 * initial.ui_scale);
      driver.input.logical_height = static_cast<int>(720 * initial.ui_scale);
      driver.input.framebuffer_width = driver.input.logical_width;
      driver.input.framebuffer_height = driver.input.logical_height;
    }
    if (scenario.starts_with("ramp-")) {
      const auto yaw = scenario.substr(5);
      rat::ClimbCameraPose camera;
      camera.focus = {0.5f,2.5f,0.5f}; camera.eye = camera.focus; camera.eye.y += 4.0f;
      if (yaw == "west") camera.eye.x += 6.0f;
      else if (yaw == "east") camera.eye.x -= 6.0f;
      else if (yaw == "north") camera.eye.z += 6.0f;
      else if (yaw == "south") camera.eye.z -= 6.0f;
      else throw std::runtime_error("Unknown ramp yaw fixture");
      initial.camera_pose = camera;
    }
    if (scenario.starts_with("bridge-")) {
      rat::PlayerBody player; player.x = scenario == "bridge-above" ? 0.15f : 0.5f;
      player.y = 0; player.z = 0.5f; player.speed = 4.0f; initial.player = player;
    }
    const bool initialized = driver.app.init(options, initial);
    if (scenario == "malformed-start") {
      driver.require(!initialized, "Malformed startup unexpectedly initialized");
      report["status"] = "passed";
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
      std::ofstream(artifacts/"scenario-report.json") << report.dump(2);
      return 0;
    }
    if (!initialized) throw std::runtime_error("Real editor initialization failed");
    driver.frame(5);
    const auto* assets = driver.app.observed_assets();
    driver.require(assets && assets->state(rat::make_asset_id("sfx/beep")) == rat::AssetState::Ready,
      "Executable-adjacent audio resource did not load");
    driver.require(driver.app.observed_cyrillic_font(), "Loaded UI font lacks actual Cyrillic glyphs");
    report["resources"] = {{"data_root",utf8(data)},{"audio",assets->compiled_path(rat::make_asset_id("sfx/beep"))},{"cyrillic_glyphs_loaded",true}};
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
      driver.click("Inspector","Place event##viewport_tool");
      driver.input.cursor_x = point->x; driver.input.cursor_y = point->y; driver.frame(3);
      const auto original_tile = driver.app.observed_document().data().events[0].tile;
      driver.input.events = {{rat::EditorInputEvent::Kind::MouseButton,0,true},
        {rat::EditorInputEvent::Kind::MouseButton,0,false}, {rat::EditorInputEvent::Kind::MouseButton,0,true},
        {rat::EditorInputEvent::Kind::MouseButton,0,false}};
      driver.frame();
      const auto moved = driver.app.project_world({-2.5f,0.0f,0.5f});
      driver.require(moved.has_value(), "Moved cursor projection missing");
      driver.input.cursor_x = moved->x; driver.input.cursor_y = moved->y; driver.frame(8);
      const auto& created = driver.app.observed_document().data().events;
      driver.require(created.size() == 1 && driver.app.observed_document().selected_event() == 0,
                     "Queued click selected/created at newer raw cursor instead of processed ImGui position");
      driver.require(created[0].tile->x == original_tile->x && created[0].tile->z == original_tile->z,
                     "Queued drag used newer raw cursor instead of processed ImGui position");
      driver.debug("input-order");
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "authoring-text") {
      driver.add_event();
      driver.require(driver.app.observed_document().data().events.size() == 1, "Real Add stub event failed");
      driver.text("Inspector","Event ID", U"\u0441\u043e\u0431\u044b\u0442\u0438\u0435 \u0401");
      driver.require(driver.app.observed_document().data().events[0].id == "\u0441\u043e\u0431\u044b\u0442\u0438\u0435 \u0401", "Unicode authored field failed");
      driver.capture("cyrillic-field");
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
    } else if (scenario.starts_with("guard-")) {
      const auto separator = scenario.find('-',6);
      const auto action = scenario.substr(6,separator-6), choice = scenario.substr(separator+1);
      driver.add_event();
      driver.action(action,user/"alternative.json");
      driver.require(driver.app.observed_modal(), "Dirty action did not open modal");
      const auto tick = driver.app.observed_session().tick_id();
      driver.capture("guard-modal"); driver.frame(5);
      driver.require(driver.app.observed_session().tick_id() == tick, "Modal did not pause simulation");
      driver.click("Unsaved changes",choice);
      driver.require(!driver.app.observed_modal(), "Choice did not resolve modal");
      if (choice == "Cancel") {
        driver.require(driver.app.observed_running() && driver.app.observed_document().dirty() &&
          driver.app.observed_document().data().events.size() == 1, "Cancel lost authored state");
      } else if (action == "close") driver.require(!driver.app.observed_running(), "Approved native close did not exit");
      else {
        driver.require(driver.app.observed_running() && !driver.app.observed_document().dirty(), "Reload/Open did not install clean document");
        const auto& loaded = driver.app.observed_document().data();
        driver.require(action == "open" ? loaded.id == "alternative_fixture" : loaded.events.size() == (choice == "Save" ? 1u : 0u), "Wrong document after action");
      }
      driver.require(read_json(map)["events"].size() == (choice == "Save" ? 1u : 0u), "Guard changed wrong saved bytes");
      driver.debug(scenario);
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "failed-save") {
      driver.add_event(); driver.action("close",{});
      driver.click("Unsaved changes","Save");
      driver.require(driver.app.observed_running() && driver.app.observed_modal() && driver.app.observed_document().dirty(), "Failed Save lost modal/action");
      driver.require(driver.app.observed_error().find("Injected") != std::string::npos && files.attempts == 1, "Save failure not surfaced exactly once");
      driver.require(read_json(map)["events"].empty(), "Failed save changed main");
      driver.capture("failed-save-modal"); driver.debug(scenario);
      driver.click("Unsaved changes","Save");
      driver.require(!driver.app.observed_running() && files.attempts == 2 && read_json(map)["events"].size() == 1, "Save retry did not finish retained Close once");
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "map-backup") {
      driver.add_event(); driver.click("Inspector","Restore map backup");
      driver.require(driver.app.observed_modal(), "Dirty backup restore bypassed guard");
      driver.click("Unsaved changes","Save");
      driver.require(driver.app.observed_document().data().id == "backup_fixture" && driver.app.observed_document().dirty(), "Restore lost pinned backup or clean baseline");
      driver.require(read_json(map)["id"] == "gui_fixture" && read_json(map)["events"].size() == 1 &&
        read_json(user/"gui-map.json.bak")["id"] == "gui_fixture", "Guard Save did not rotate backup as expected");
      driver.capture("restored-dirty"); driver.click("Inspector","Save current map JSON");
      driver.require(read_json(map)["id"] == "backup_fixture" && !driver.app.observed_document().dirty(), "Restore did not retain primary save path");
      driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "slot-backup") {
      const auto main = read_json(user/"save.json");
      driver.key(GLFW_KEY_ESCAPE); driver.click("Pause","Restore save slot backup");
      driver.require(driver.app.observed_session().state().get_variable(7) == 42 &&
        driver.app.observed_session().player().x == -2.5f, "Actual pause backup button did not restore slot");
      driver.require(read_json(user/"save.json") == main, "Slot restore rewrote primary slot");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "malformed-load") {
      driver.enter_edit(); const auto original = rat::authoring_snapshot(driver.app.observed_document().data());
      driver.open_path(user/"malformed.json");
      driver.require(rat::authoring_snapshot(driver.app.observed_document().data()) == original &&
        !driver.app.observed_document().dirty() && !driver.app.observed_document().can_undo(), "Malformed load changed existing document/history");
      driver.require(!driver.app.observed_error().empty(), "Malformed load error invisible");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "restart-create" || scenario == "restart-add") {
      const auto before = driver.app.observed_document().data().events;
      driver.require(before.size() == (scenario == "restart-create" ? 0u : 1u), "Restart fixture did not preserve prior process save");
      driver.add_event(); const auto& events = driver.app.observed_document().data().events;
      driver.require(events.size() == before.size()+1 && (before.empty() || events.back().id != before[0].id), "ID allocator collided after real process restart");
      driver.click("Inspector","Save current map JSON"); driver.capture(scenario); driver.debug(scenario);
      report["event_ids"] = Json::array(); for (const auto& event : events) report["event_ids"].push_back(event.id);
      driver.action("close",{}); driver.require(!driver.app.observed_running(), "Clean native close failed");
      report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "creation-and-duplicate") {
      driver.add_event();
      driver.click("Inspector","Place event##viewport_tool"); driver.world_click({-3.5f,0,0.5f});
      driver.world_click({-2.5f,0,-0.5f},1); driver.click("","Create");
      const auto original = driver.app.observed_document().data().events;
      driver.require(original.size() == 3 && original[0].id != original[1].id && original[1].id != original[2].id && original[0].id != original[2].id,
                     "Three actual creation paths did not allocate unique IDs");
      driver.text("Inspector","Event ID",decode_utf8(original[0].id));
      driver.require(driver.app.observed_document().data().events[2].id == original[2].id &&
        driver.app.observed_document().last_error().find("unique") != std::string::npos, "Duplicate ID rename did not fail visibly and transactionally");
      (void)driver.find("Inspector","Edit error");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "history-voxel") {
      driver.add_event(); const auto id = driver.app.observed_document().data().events[0].id;
      driver.click("Inspector","Ev +X"); driver.key(GLFW_KEY_Z,true);
      driver.require(driver.app.observed_document().can_redo(), "Undo did not retain redo");
      driver.text("Inspector","Event ID",decode_utf8(id));
      driver.require(driver.app.observed_document().can_redo(), "No-op field edit lost redo");
      driver.key(GLFW_KEY_Y,true); driver.key(GLFW_KEY_Z,true); driver.key(GLFW_KEY_Z,true);
      driver.require(driver.app.observed_document().data().events.empty() && !driver.app.observed_document().dirty(), "Undo to baseline did not clear dirty");
      driver.click("Inspector","Terrain"); driver.click("Inspector","Remove voxel##viewport_tool");
      driver.world_click({-3.5f,0,0.5f});
      driver.require(driver.app.observed_document().can_redo() && !driver.app.observed_document().dirty(), "Empty voxel no-op lost redo or dirtied map");
      driver.click("Inspector","Place voxel##viewport_tool");
      const auto point = driver.app.project_world({-3.5f,0,0.5f});
      driver.input.cursor_x = point->x; driver.input.cursor_y = point->y; driver.frame(2);
      driver.input.mouse_buttons[0] = true; driver.frame(2);
      driver.require(!driver.app.observed_document().data().occupancy.empty(), "Viewport voxel stroke did not place");
      driver.input.mouse_buttons[1] = true; driver.frame(2);
      driver.input.mouse_buttons[0] = driver.input.mouse_buttons[1] = false; driver.frame(3);
      driver.require(driver.app.observed_document().data().occupancy.empty() && driver.app.observed_document().can_redo() &&
        !driver.app.observed_document().dirty(), "Aborted voxel stroke lost prior redo");
      driver.world_click({-3.5f,0,0.5f});
      driver.require(driver.app.observed_document().data().occupancy.size() == 1, "Real voxel place failed");
      driver.click("Inspector","Remove voxel##viewport_tool"); driver.world_click({-3.5f,1.0f,0.5f});
      driver.require(driver.app.observed_document().data().occupancy.empty(), "Real voxel remove failed");
      driver.key(GLFW_KEY_Z,true);
      driver.require(driver.app.observed_document().data().occupancy.size() == 1, "Voxel removal undo failed");
      driver.key(GLFW_KEY_Y,true);
      driver.require(driver.app.observed_document().data().occupancy.empty(), "Voxel removal redo failed");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario.starts_with("ramp-")) {
      driver.enter_edit(); driver.click("Inspector","Place ramp voxel##viewport_tool");
      const auto yaw = scenario.substr(5);
      rat::Vec3 face{0.5f,2.5f,0.5f};
      int x = 0, z = 0;
      rat::RampDirection expected = rat::RampDirection::North;
      if (yaw == "west") { face.x = 1.0f; x = 1; expected = rat::RampDirection::West; }
      if (yaw == "east") { face.x = 0.0f; x = -1; expected = rat::RampDirection::East; }
      if (yaw == "north") { face.z = 1.0f; z = 1; expected = rat::RampDirection::North; }
      if (yaw == "south") { face.z = 0.0f; z = -1; expected = rat::RampDirection::South; }
      driver.capture("before-side-place"); driver.world_click(face);
      const auto& cells = driver.app.observed_document().data().occupancy;
      driver.require(cells.size() == 2, "Real visible side-face click did not place ramp");
      const auto& ramp = cells.back();
      driver.require(ramp.kind == rat::OccupancyKind::Ramp && ramp.x == x && ramp.y == 2 && ramp.z == z && ramp.yaw == expected,
                     "Side-face gesture placed wrong cell/layer/yaw");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario.starts_with("bridge-")) {
      driver.click_at(1000,650);
      // Top-down input basis maps A to world +X. This is the real keyboard path.
      driver.input.keys[GLFW_KEY_A] = true;
      bool crossed = false, thin_support = false;
      for (int i = 0; i < 80; ++i) {
        driver.frame(); const auto& session = driver.app.observed_session(); const auto& player = session.player();
        if (scenario == "bridge-above" && player.x >= 2.1f && player.x < 2.8f && session.jump().grounded && std::abs(player.y-2.0f)<0.06f) {
          const auto& map_data = session.events().map();
          const auto world = rat::bake_collision_world(map_data,rat::SurfaceQuery(map_data));
          const auto support = rat::query_solid_support(world,player.x,player.z,player.half_extent,player.y,0.01f);
          for (const auto& box : world.boxes)
            thin_support |= support && !support->on_ramp && player.x >= box.min_x && player.x < box.max_x &&
              player.z >= box.min_z && player.z < box.max_z && std::abs(box.y_hi-player.y)<0.01f && std::abs(box.y_hi-box.y_lo-0.25f)<0.01f;
          crossed = true; break;
        }
        if (scenario != "bridge-above" && player.x > 1.2f && player.x < 1.8f) {
          crossed = true;
          driver.require(std::abs(player.y)<0.01f && session.jump().grounded, "Underpass left ground");
        }
      }
      driver.input.keys[GLFW_KEY_A] = false; driver.frame();
      if (scenario == "bridge-above") driver.require(crossed && thin_support, "Player did not walk onto actual thin bridge support");
      else if (scenario == "bridge-under") driver.require(crossed && driver.app.observed_session().player().x > 2.0f, "Player did not progress through underpass");
      else driver.require(!crossed && driver.app.observed_session().player().x < 0.7f, "Filled-ground-to-deck negative fixture did not block identical input");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "graph-active-close") {
      driver.enter_edit(); driver.click("Inspector","Events"); driver.click("Inspector","graph_event  tile(-3,0)"); driver.click("Inspector","Open Event Graph");
      const auto original = *driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout;
      const auto header = driver.find("Event Graph","##drag_text");
      driver.input.cursor_x = (header.min_x+header.max_x)*0.5; driver.input.cursor_y = (header.min_y+header.max_y)*0.5;
      driver.frame(2); driver.input.mouse_buttons[0] = true; driver.frame();
      driver.input.cursor_x += 45; driver.input.cursor_y += 30; driver.frame(2);
      driver.input.close_requested = true; driver.frame(4);
      driver.require(driver.app.observed_modal() && driver.app.observed_document().dirty(), "Close failed to settle active layout preview before guard");
      driver.input.mouse_buttons[0] = false; driver.frame(2); driver.capture(scenario);
      driver.click("Unsaved changes","Cancel"); driver.key(GLFW_KEY_Z,true);
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout->x == original.x &&
        !driver.app.observed_document().dirty() && !driver.app.observed_document().can_undo(), "Guard-settled drag was not one undo entry");
      driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "graph") {
      driver.enter_edit(); driver.click("Inspector","Events"); driver.click("Inspector","graph_event  tile(-3,0)"); driver.click("Inspector","Open Event Graph");
      const auto baseline = rat::authoring_snapshot(driver.app.observed_document().data());
      driver.capture("graph-initial");
      driver.connect("text","text");
      driver.require(!driver.app.observed_canvas().dragging_wire && driver.app.observed_canvas().pending_from.empty() &&
        !driver.app.observed_document().can_undo() && rat::authoring_snapshot(driver.app.observed_document().data()) == baseline, "Self-pin gesture mutated graph or retained wire");
      auto source = driver.find("Event Graph","##out_text");
      driver.drag(source,(source.min_x+source.max_x)*0.5f,(source.min_y+source.max_y)*0.5f,true);
      driver.require(!driver.app.observed_canvas().dragging_wire && driver.app.observed_canvas().pending_from.empty() && !driver.app.observed_document().can_undo(), "Source-output release retained drag or history");
      source = driver.find("Event Graph","##out_text");
      driver.drag(source,source.max_x+50,source.max_y+130);
      driver.require(!driver.app.observed_canvas().dragging_wire && driver.app.observed_canvas().pending_from.empty() && !driver.app.observed_document().can_undo(), "Empty release retained drag or history");
      driver.click("Event Graph","##out_text");
      driver.require(driver.app.observed_canvas().pending_from == "text", "Plain source click lost two-click connection");
      driver.click("Event Graph","##in_exit");
      driver.require(driver.app.observed_canvas().pending_from.empty() && !driver.app.observed_document().can_undo(), "No-op existing connection recorded history");
      driver.click("Event Graph","Wait");
      const auto added = driver.app.observed_document().data().events[0].pages[0].graph->nodes.back().id;
      driver.connect("text",added); driver.connect(added,"exit");
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->edges.size() == 3, "Real pin connections failed");
      driver.click("Inspector","Save current map JSON");
      driver.require(!driver.app.observed_document().dirty(), "Connected graph did not save");
      driver.click("Event Graph","##drag_"+added); driver.key(GLFW_KEY_DELETE);
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->nodes.size() == 1, "Real graph Delete key failed");
      driver.key(GLFW_KEY_Z,true);
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->nodes.size() == 2, "Graph delete undo failed");
      const auto original = *driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout;
      auto header = driver.find("Event Graph","##drag_text");
      driver.drag(header,(header.min_x+header.max_x)*0.5f+45,(header.min_y+header.max_y)*0.5f+30);
      const auto moved = *driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout;
      driver.require(moved.x != original.x || moved.y != original.y, "Real node header drag did not author layout");
      driver.key(GLFW_KEY_Z,true);
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout->x == original.x && driver.app.observed_document().can_redo(), "Node gesture did not undo as one entry");
      header = driver.find("Event Graph","##drag_text");
      driver.drag(header,(header.min_x+header.max_x)*0.5f,(header.min_y+header.max_y)*0.5f,true);
      driver.require(driver.app.observed_document().can_redo(), "No-op node drag discarded redo");
      driver.key(GLFW_KEY_Y,true); driver.click("Inspector","Save current map JSON");
      driver.key(GLFW_KEY_F5);
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout->x == moved.x, "Authored node layout did not survive UI save/reload");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario.starts_with("scale-")) {
      driver.enter_edit(); driver.click("Inspector","Events");
      driver.input.framebuffer_width = driver.input.logical_width * 5 / 4;
      driver.input.framebuffer_height = driver.input.logical_height * 5 / 4;
      driver.frame(5); driver.click("Inspector","Place event##viewport_tool");
      driver.world_click({-3.5f,0,0.5f});
      driver.require(driver.app.observed_document().data().events.size() == 2 && driver.app.observed_document().data().events.back().tile && driver.app.observed_document().data().events.back().tile->x == -4, "Scaled framebuffer viewport picking missed world tile");
      driver.input.framebuffer_width = driver.input.logical_width;
      driver.input.framebuffer_height = driver.input.logical_height;
      driver.frame(5); driver.click("Inspector","graph_event  tile(-3,0)");
      driver.text("Inspector","Event ID",U"\u0442\u0435\u0441\u0442_\u0401");
      driver.require(driver.app.observed_document().data().events[0].id == utf8(fs::path(u8"\u0442\u0435\u0441\u0442_\u0401")), "Scaled Unicode input missed field");
      driver.click("Inspector","Open Event Graph"); driver.connect("text","exit");
      driver.require(driver.app.observed_canvas().pending_from.empty() && !driver.app.observed_canvas().dragging_wire, "Scaled pin gesture missed target");
      const auto header = driver.find("Event Graph","##drag_text");
      const auto original = *driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout;
      driver.drag(header,(header.min_x+header.max_x)*0.5f+20*initial.ui_scale,(header.min_y+header.max_y)*0.5f+10*initial.ui_scale);
      const auto moved = *driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].layout;
      driver.require(std::abs(moved.x-original.x-20)<0.1f && std::abs(moved.y-original.y-10)<0.1f, "UI scale contaminated authored layout units");
      driver.capture(scenario+"-base");
      driver.input.logical_width += 80; driver.input.logical_height += 60;
      driver.input.framebuffer_width = driver.input.logical_width * 5 / 4;
      driver.input.framebuffer_height = driver.input.logical_height * 5 / 4;
      driver.frame(5); driver.connect("text","exit");
      driver.require(driver.app.observed_canvas().pending_from.empty() && !driver.app.observed_canvas().dragging_wire, "Resized framebuffer ratio broke pin hit");
      driver.text("Event Graph","##text",U"\u041c\u0430\u0441\u0448\u0442\u0430\u0431");
      driver.click("Event Graph","##drag_text");
      driver.require(driver.app.observed_document().data().events[0].pages[0].graph->nodes[0].text == utf8(fs::path(u8"\u041c\u0430\u0441\u0448\u0442\u0430\u0431")), "Resized scaled graph text input failed");
      driver.input.close_requested = true; driver.frame(4);
      driver.require(driver.app.observed_modal(), "Scaled close did not show modal");
      driver.capture(scenario+"-ratio-modal"); driver.click("Unsaved changes","Cancel");
      driver.require(!driver.app.observed_modal() && driver.app.observed_running(), "Scaled modal click missed Cancel");
      driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario.starts_with("unsupported-")) {
      driver.enter_edit(); driver.open_path(user/"unsupported.json");
      driver.require(driver.app.observed_mode() == rat::AppMode::Edit && !driver.app.observed_runtime_valid(), "Unsupported draft did not open for repair");
      driver.click("Inspector","Apply edited map");
      driver.require(!driver.app.observed_runtime_valid() && !driver.app.observed_error().empty(), "Unsupported Apply did not surface failure");
      driver.click("Inspector","Enter Play (F2)");
      driver.require(driver.app.observed_mode() == rat::AppMode::Edit, "Unsupported draft entered Play");
      driver.click("Inspector","Events"); driver.click("Inspector","graph_event  tile(-3,0)"); driver.click("Inspector","Open Event Graph");
      if (scenario == "unsupported-touch") {
        driver.click("Event Graph","Trigger");
        const auto disabled = driver.find("","event_touch (unsupported)",true);
        driver.require(!disabled.enabled, "Unsupported EventTouch choice was enabled");
        driver.capture("unsupported-choice"); driver.click("","action");
      } else driver.text("Event Graph","##map",U"gui_fixture");
      driver.require(driver.app.observed_document().dirty(), "UI repair did not author a change");
      driver.click("Inspector","Apply edited map");
      driver.require(driver.app.observed_runtime_valid(), "Repaired draft failed Apply");
      driver.click("Inspector","Enter Play (F2)");
      driver.require(driver.app.observed_mode() == rat::AppMode::Play, "Repaired draft could not enter Play");
      driver.capture(scenario); driver.debug(scenario); report["scenarios"].push_back({{"name",scenario},{"status","passed"}});
    } else if (scenario == "play-modal") {
      driver.add_event(); driver.click("Inspector","Enter Play (F2)"); driver.click_at(1000,650);
      const auto before = driver.app.observed_session().tick_id(); driver.frame(10);
      driver.require(driver.app.observed_session().tick_id() > before && driver.app.observed_document().dirty(), "Play fixture was not advancing and dirty");
      driver.input.keys[GLFW_KEY_F5] = true; driver.frame(4);
      driver.require(driver.app.observed_modal(), "Dirty Play F5 did not show guard");
      const auto paused = driver.app.observed_session().tick_id(); driver.frame(30);
      driver.require(driver.app.observed_session().tick_id() == paused, "Previously advancing Play continued during modal");
      driver.capture(scenario); driver.click("Unsaved changes","Cancel"); driver.frame(4);
      driver.require(!driver.app.observed_modal() && driver.app.observed_session().tick_id() > paused &&
        driver.app.observed_session().tick_id()-paused <= 15, "Dismissal replayed held F5 or accumulated paused time");
      driver.input.keys[GLFW_KEY_F5] = false; driver.frame(2); driver.debug(scenario);
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
