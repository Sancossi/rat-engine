#pragma once

namespace rat {

enum class AppMode {
  Play,
  Edit,
};

[[nodiscard]] AppMode toggle_app_mode(AppMode mode);
[[nodiscard]] const char* app_mode_name(AppMode mode);
[[nodiscard]] bool player_control_enabled(AppMode mode);
[[nodiscard]] bool event_runtime_enabled(AppMode mode);

}  // namespace rat
