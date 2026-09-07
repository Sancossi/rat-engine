#include "gui_observer.hpp"

#include <imgui.h>
#include <imgui_internal.h>

#include <algorithm>

namespace {
std::vector<rat::GuiItemObservation> items;
rat::GuiItemObservation& item(ImGuiContext* ctx, ImGuiID id) {
  const std::string window = ctx->CurrentWindow ? ctx->CurrentWindow->Name : "";
  auto found = std::find_if(items.begin(), items.end(), [&](const auto& entry) {
    return entry.id == id && entry.window == window;
  });
  if (found != items.end()) return *found;
  items.push_back({});
  items.back().id = id;
  items.back().window = window;
  return items.back();
}
}

namespace rat {
void begin_gui_observation() {
  items.clear();
  ImGui::GetCurrentContext()->TestEngineHookItems = true;
}
const std::vector<GuiItemObservation>& gui_items() { return items; }
}

void ImGuiTestEngineHook_ItemAdd(ImGuiContext* ctx, ImGuiID id, const ImRect& bb,
                               const ImGuiLastItemData* data) {
  auto& entry = item(ctx, id);
  entry.min_x = bb.Min.x; entry.min_y = bb.Min.y;
  entry.max_x = bb.Max.x; entry.max_y = bb.Max.y;
  entry.visible = ctx->CurrentWindow && bb.Overlaps(ctx->CurrentWindow->ClipRect);
  entry.enabled = !((data ? data->ItemFlags : ctx->CurrentItemFlags) & ImGuiItemFlags_Disabled);
}
void ImGuiTestEngineHook_ItemInfo(ImGuiContext* ctx, ImGuiID id, const char* label,
                                ImGuiItemStatusFlags) {
  item(ctx, id).label = label ? label : "";
}
void ImGuiTestEngineHook_Log(ImGuiContext*, const char*, ...) {}
const char* ImGuiTestEngine_FindItemDebugLabel(ImGuiContext*, ImGuiID id) {
  const auto found = std::find_if(items.begin(), items.end(), [=](const auto& entry) { return entry.id == id; });
  return found == items.end() ? nullptr : found->label.c_str();
}
