#include "rat/app_mode.hpp"

namespace rat {

AppMode toggle_app_mode(AppMode mode) {
  return mode == AppMode::Play ? AppMode::Edit : AppMode::Play;
}

const char* app_mode_name(AppMode mode) {
  switch (mode) {
    case AppMode::Play:
      return "PLAY";
    case AppMode::Edit:
      return "EDIT";
  }
  return "PLAY";
}

bool player_control_enabled(AppMode mode) {
  return mode == AppMode::Play;
}

bool event_runtime_enabled(AppMode mode) {
  return mode == AppMode::Play;
}

}  // namespace rat
