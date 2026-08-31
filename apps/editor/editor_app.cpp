#include "editor_app.hpp"

#include "imgui_bgfx.hpp"

#include <rat/engine.hpp>
#include <rat/map_loader.hpp>

#include <imgui.h>
#include <imgui_impl_glfw.h>

#define GLFW_EXPOSE_NATIVE_WIN32
#include <GLFW/glfw3.h>
#include <GLFW/glfw3native.h>

#include <cstdio>
#include <string>

namespace rat {

EditorApp::~EditorApp() {
  shutdown();
}

bool EditorApp::init() {
  if (!glfwInit()) {
    std::fprintf(stderr, "glfwInit failed\n");
    return false;
  }

  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_TRUE);

  window_ = glfwCreateWindow(width_, height_, "rat-editor", nullptr, nullptr);
  if (window_ == nullptr) {
    std::fprintf(stderr, "glfwCreateWindow failed\n");
    glfwTerminate();
    return false;
  }

  glfwSetWindowUserPointer(window_, this);
  glfwSetFramebufferSizeCallback(window_, framebuffer_size_callback);
  glfwGetFramebufferSize(window_, &width_, &height_);

  engine_ = new Engine();

  RendererConfig config;
  config.window.nwh = glfwGetWin32Window(window_);
  config.width = static_cast<std::uint32_t>(width_ > 0 ? width_ : 1);
  config.height = static_cast<std::uint32_t>(height_ > 0 ? height_ : 1);
  config.vsync = true;

  if (!engine_->init(config)) {
    std::fprintf(stderr, "Engine::init failed\n");
    shutdown();
    return false;
  }
  engine_->set_debug_banner("rat-engine");

#ifndef RAT_DATA_DIR
#error RAT_DATA_DIR must be defined
#endif
  const std::string map_path = std::string(RAT_DATA_DIR) + "/maps/grey_yard.json";
  const auto loaded = load_map_from_file(map_path);
  if (!loaded.ok) {
    std::fprintf(stderr, "Failed to load map %s: %s\n", map_path.c_str(), loaded.error.c_str());
    shutdown();
    return false;
  }
  events_.load(loaded.map);
  game_state_.set_map_id(loaded.map.id);
  engine_->set_blockers(loaded.map.blockers);
  player_ = engine_->player();

  IMGUI_CHECKVERSION();
  ImGui::CreateContext();
  ImGuiIO& io = ImGui::GetIO();
  io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
  io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
  ImGui::StyleColorsDark();

  if (!ImGui_ImplGlfw_InitForOther(window_, true)) {
    std::fprintf(stderr, "ImGui_ImplGlfw_InitForOther failed\n");
    shutdown();
    return false;
  }

  if (!imgui_bgfx::init(255)) {
    std::fprintf(stderr, "imgui_bgfx::init failed\n");
    shutdown();
    return false;
  }

  last_time_ = glfwGetTime();
  running_ = true;
  return true;
}

int EditorApp::run() {
  if (!running_) {
    return 1;
  }

  while (!glfwWindowShouldClose(window_)) {
    glfwPollEvents();

    const double now = glfwGetTime();
    float dt = static_cast<float>(now - last_time_);
    last_time_ = now;
    if (dt < 0.0f) {
      dt = 0.0f;
    }
    if (dt > 0.1f) {
      dt = 0.1f;
    }
    update_simulation(dt);

    ImGui_ImplGlfw_NewFrame();
    imgui_bgfx::begin_frame(width_, height_);
    draw_ui();

    engine_->begin_frame();
    imgui_bgfx::end_frame();
    engine_->end_frame();
  }

  shutdown();
  return 0;
}

void EditorApp::shutdown() {
  if (!running_ && window_ == nullptr && engine_ == nullptr) {
    return;
  }
  running_ = false;

  imgui_bgfx::shutdown();
  ImGui_ImplGlfw_Shutdown();
  if (ImGui::GetCurrentContext() != nullptr) {
    ImGui::DestroyContext();
  }

  if (engine_ != nullptr) {
    engine_->shutdown();
    delete engine_;
    engine_ = nullptr;
  }

  if (window_ != nullptr) {
    glfwDestroyWindow(window_);
    window_ = nullptr;
  }
  glfwTerminate();
}

void EditorApp::on_framebuffer_resize(int width, int height) {
  width_ = width > 0 ? width : 1;
  height_ = height > 0 ? height : 1;
  if (engine_ != nullptr && engine_->is_initialized()) {
    engine_->resize(static_cast<std::uint32_t>(width_),
                    static_cast<std::uint32_t>(height_));
  }
}

void EditorApp::framebuffer_size_callback(GLFWwindow* window, int width, int height) {
  auto* self = static_cast<EditorApp*>(glfwGetWindowUserPointer(window));
  if (self != nullptr) {
    self->on_framebuffer_resize(width, height);
  }
}

void EditorApp::update_simulation(float dt) {
  if (engine_ == nullptr || window_ == nullptr) {
    return;
  }

  const ImGuiIO& io = ImGui::GetIO();
  const bool interact_down =
      !io.WantCaptureKeyboard &&
      (glfwGetKey(window_, GLFW_KEY_E) == GLFW_PRESS ||
       glfwGetKey(window_, GLFW_KEY_SPACE) == GLFW_PRESS);
  const bool interact_pressed = interact_down && !interact_was_down_;
  interact_was_down_ = interact_down;

  if (events_.active_message().has_value() && interact_pressed) {
    events_.acknowledge_message();
  }

  MoveInput input;
  if (!io.WantCaptureKeyboard && !events_.player_input_blocked()) {
    if (glfwGetKey(window_, GLFW_KEY_W) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_UP) == GLFW_PRESS) {
      input.axis_z -= 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_S) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_DOWN) == GLFW_PRESS) {
      input.axis_z += 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_A) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_LEFT) == GLFW_PRESS) {
      input.axis_x -= 1.0f;
    }
    if (glfwGetKey(window_, GLFW_KEY_D) == GLFW_PRESS ||
        glfwGetKey(window_, GLFW_KEY_RIGHT) == GLFW_PRESS) {
      input.axis_x += 1.0f;
    }
  }

  player_ = integrate_player(player_, input, dt, engine_->blockers());
  game_state_.set_player_position(player_.x, player_.y, player_.z);
  engine_->set_player(player_);

  events_.update(game_state_, player_, interact_pressed, dt);

  // Transfer stub: sync player body if an event moved GameState.
  player_.x = game_state_.player_x();
  player_.y = game_state_.player_y();
  player_.z = game_state_.player_z();
  engine_->set_player(player_);
}

void EditorApp::draw_ui() {
  ImGuiWindowFlags dock_flags = ImGuiWindowFlags_MenuBar | ImGuiWindowFlags_NoDocking;
  const ImGuiViewport* viewport = ImGui::GetMainViewport();
  ImGui::SetNextWindowPos(viewport->WorkPos);
  ImGui::SetNextWindowSize(viewport->WorkSize);
  ImGui::SetNextWindowViewport(viewport->ID);
  dock_flags |= ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse |
                ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoMove |
                ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus |
                ImGuiWindowFlags_NoBackground;

  ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.0f);
  ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.0f, 0.0f));
  ImGui::Begin("DockSpace", nullptr, dock_flags);
  ImGui::PopStyleVar(3);

  if (ImGui::BeginMenuBar()) {
    if (ImGui::BeginMenu("File")) {
      if (ImGui::MenuItem("Exit")) {
        glfwSetWindowShouldClose(window_, GLFW_TRUE);
      }
      ImGui::EndMenu();
    }
    ImGui::EndMenuBar();
  }

  const ImGuiID dockspace_id = ImGui::GetID("RatDockSpace");
  ImGui::DockSpace(dockspace_id, ImVec2(0.0f, 0.0f), ImGuiDockNodeFlags_PassthruCentralNode);

  ImGui::Begin("Hierarchy");
  ImGui::Text("Map: %s", game_state_.map_id().c_str());
  ImGui::Text("Player: (%.2f, %.2f)", player_.x, player_.z);
  ImGui::Text("Events: %zu", events_.map().events.size());
  ImGui::Text("Parallel: %d", events_.active_parallel_count());
  ImGui::End();

  ImGui::Begin("Inspector");
  ImGui::TextUnformatted("WASD / arrows: move");
  ImGui::TextUnformatted("E / Space: interact / advance text");
  if (ImGui::Button("Snap player to grid")) {
    const auto snapped = snap_to_grid(player_.x, player_.y, player_.z, 1.0f);
    player_.x = snapped.x;
    player_.y = snapped.y;
    player_.z = snapped.z;
    game_state_.set_player_position(player_.x, player_.y, player_.z);
    engine_->set_player(player_);
  }
  ImGui::Separator();
  ImGui::Text("Switch1: %s", game_state_.get_switch(1) ? "ON" : "OFF");
  ImGui::Text("Var0: %d", game_state_.get_variable(0));
  ImGui::Text("rusty_cog: %d", game_state_.item_quantity("rusty_cog"));
  ImGui::End();

  if (events_.active_message().has_value()) {
    ImGui::SetNextWindowPos(ImVec2(viewport->WorkPos.x + 80.0f,
                                   viewport->WorkPos.y + viewport->WorkSize.y - 160.0f));
    ImGui::SetNextWindowSize(ImVec2(viewport->WorkSize.x - 160.0f, 120.0f));
    ImGui::Begin("Dialog", nullptr,
                 ImGuiWindowFlags_NoResize | ImGuiWindowFlags_NoCollapse |
                     ImGuiWindowFlags_NoMove);
    ImGui::TextWrapped("%s", events_.active_message()->c_str());
    ImGui::Spacing();
    ImGui::TextUnformatted("[E/Space] continue");
    ImGui::End();
  }

  ImGui::End();
}

}  // namespace rat
