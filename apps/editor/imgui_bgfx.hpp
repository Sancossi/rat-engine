#pragma once

struct GLFWwindow;

namespace rat::imgui_bgfx {

bool init(int view_id = 255);
void shutdown();
void begin_frame();
void end_frame();  // renders ImGui draw data into bgfx view

}  // namespace rat::imgui_bgfx
