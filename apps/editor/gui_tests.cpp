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
  rat::GuiItemObservation find(const std::string& window, const std::string& label) {
    for (const auto& item : rat::gui_items())
      if (item.window == window && item.label == label && item.visible && item.enabled) return item;
    throw std::runtime_error("Missing visible enabled selector " + window + "/" + label);
  }
  void click(const std::string& window, const std::string& label) {
    const auto item = find(window, label);
    input.cursor_x = (item.min_x + item.max_x) * 0.5;
    input.cursor_y = (item.min_y + item.max_y) * 0.5;
    frame(2); input.mouse_buttons[0] = true; frame();
    input.mouse_buttons[0] = false; frame(2);
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
  rat::RendererMode renderer = rat::RendererMode::SoftwareD3D11;
  for (int i = 1; i < argc; i += 2) {
    if (i + 1 >= argc) { std::cerr << "Missing argument value\n"; return 2; }
    const std::string key = argv[i], value = argv[i + 1];
    if (key == "--data-root") data = fs::path(std::u8string(value.begin(), value.end()));
    else if (key == "--user-data-dir") user = fs::path(std::u8string(value.begin(), value.end()));
    else if (key == "--artifact-dir") artifacts = fs::path(std::u8string(value.begin(), value.end()));
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
    fs::copy_file(data / "maps/grey_yard.json", map, fs::copy_options::overwrite_existing);
    rat::EditorLaunchOptions options{utf8(data),utf8(user),utf8(map),utf8(user/"save.json"),
        utf8(artifacts/"editor.log"),utf8(artifacts/"snapshot.json"),utf8(user/"imgui.ini")};
    rat::EditorInitialState initial;
    initial.renderer = renderer; initial.automation_layout = true; initial.hidden_window = true;
    if (!driver.app.init(options, initial)) throw std::runtime_error("Real editor initialization failed");
    driver.frame(5);
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
