#pragma once

struct GLFWwindow;

namespace rat::imgui_bgfx {

bool init(int view_id = 255);
void shutdown();
void begin_frame(int display_w, int display_h);
void end_frame();  // renders ImGui draw data into bgfx view

}  // namespace rat::imgui_bgfx
