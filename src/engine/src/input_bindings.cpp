#include "rat/input_bindings.hpp"

#include <nlohmann/json.hpp>

#include <cstddef>
#include <initializer_list>
#include <string>
#include <string_view>

namespace rat {

namespace {

[[nodiscard]] ActionSources keys_only(std::initializer_list<const char*> names) {
  ActionSources sources;
  sources.keys.reserve(names.size());
  for (const char* name : names) {
    sources.keys.push_back(KeyboardChord{std::string(name), std::nullopt, std::nullopt});
  }
  return sources;
}

[[nodiscard]] bool key_held(const KeyboardState& keyboard, std::string_view name) {
  for (const std::string& down : keyboard.keys_down) {
    if (down == name) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool chord_matches(const KeyboardState& keyboard, const KeyboardChord& chord) {
  if (!key_held(keyboard, chord.key)) {
    return false;
  }
  if (chord.ctrl.has_value() && keyboard.ctrl != *chord.ctrl) {
    return false;
  }
  if (chord.shift.has_value() && keyboard.shift != *chord.shift) {
    return false;
  }
  return true;
}

[[nodiscard]] bool keyboard_active(const KeyboardState& keyboard, const ActionSources& sources) {
  for (const KeyboardChord& chord : sources.keys) {
    if (chord_matches(keyboard, chord)) {
      return true;
    }
  }
  return false;
}

[[nodiscard]] bool button_held(const GamepadState& gamepad, int button) {
  return button >= 0 && button < kGamepadButtonCount && gamepad.buttons[static_cast<std::size_t>(button)];
}

[[nodiscard]] bool axis_active(const GamepadState& gamepad, const GamepadAxisBinding& binding) {
  if (binding.axis < 0 || binding.axis >= kGamepadAxisCount) {
    return false;
  }
  const float value = gamepad.axes[static_cast<std::size_t>(binding.axis)];
  if (binding.negative) {
    return value <= -binding.threshold;
  }
  return value >= binding.threshold;
}

[[nodiscard]] bool gamepad_active(const GamepadState& gamepad, const ActionSources& sources) {
  if (!gamepad.connected) {
    return false;
  }
  for (int button : sources.gamepad_buttons) {
    if (button_held(gamepad, button)) {
      return true;
    }
  }
  for (const GamepadAxisBinding& axis : sources.gamepad_axes) {
    if (axis_active(gamepad, axis)) {
      return true;
    }
  }
  return false;
}

void bind_move(ActionSources& sources, std::initializer_list<const char*> keys, int dpad,
               GamepadAxisBinding stick) {
  sources = keys_only(keys);
  sources.gamepad_buttons.push_back(dpad);
  sources.gamepad_axes.push_back(stick);
}

template <typename Probe>
[[nodiscard]] InputButtons map_with(const InputBindings& bindings, Probe&& probe) {
  InputButtons buttons;
  buttons.move_up = probe(bindings.move_up);
  buttons.move_down = probe(bindings.move_down);
  buttons.move_left = probe(bindings.move_left);
  buttons.move_right = probe(bindings.move_right);
  buttons.jump = probe(bindings.jump);
  buttons.interact = probe(bindings.interact);
  buttons.toggle_mode = probe(bindings.toggle_mode);
  buttons.hot_apply = probe(bindings.hot_apply);
  buttons.cycle_camera = probe(bindings.cycle_camera);
  buttons.debug_snapshot = probe(bindings.debug_snapshot);
  buttons.undo = probe(bindings.undo);
  buttons.redo = probe(bindings.redo);
  return buttons;
}

}  // namespace

InputBindings default_input_bindings() {
  InputBindings bindings;
  bind_move(bindings.move_up, {"W", "Up"}, kGamepadButtonDpadUp,
            GamepadAxisBinding{kGamepadAxisLeftY, true});
  bind_move(bindings.move_down, {"S", "Down"}, kGamepadButtonDpadDown,
            GamepadAxisBinding{kGamepadAxisLeftY, false});
  bind_move(bindings.move_left, {"A", "Left"}, kGamepadButtonDpadLeft,
            GamepadAxisBinding{kGamepadAxisLeftX, true});
  bind_move(bindings.move_right, {"D", "Right"}, kGamepadButtonDpadRight,
            GamepadAxisBinding{kGamepadAxisLeftX, false});
  bindings.jump = keys_only({"Space"});
  bindings.jump.gamepad_buttons.push_back(kGamepadButtonA);
  bindings.interact = keys_only({"E"});
  bindings.interact.gamepad_buttons.push_back(kGamepadButtonX);
  bindings.toggle_mode = keys_only({"F2"});
  bindings.hot_apply = keys_only({"F5"});
  bindings.cycle_camera = keys_only({"C"});
  bindings.debug_snapshot = keys_only({"F3"});
  bindings.undo.keys.push_back(KeyboardChord{"Z", true, false});
  bindings.redo.keys.push_back(KeyboardChord{"Y", true, std::nullopt});
  bindings.redo.keys.push_back(KeyboardChord{"Z", true, true});
  return bindings;
}

InputButtons map_keyboard_buttons(const KeyboardState& keyboard, const InputBindings& bindings) {
  return map_with(bindings, [&](const ActionSources& sources) {
    return keyboard_active(keyboard, sources);
  });
}

InputButtons map_gamepad_buttons(const GamepadState& gamepad, const InputBindings& bindings) {
  return map_with(bindings, [&](const ActionSources& sources) {
    return gamepad_active(gamepad, sources);
  });
}

InputButtons merge_input_buttons(const InputButtons& a, const InputButtons& b) {
  InputButtons buttons;
  buttons.move_up = a.move_up || b.move_up;
  buttons.move_down = a.move_down || b.move_down;
  buttons.move_left = a.move_left || b.move_left;
  buttons.move_right = a.move_right || b.move_right;
  buttons.jump = a.jump || b.jump;
  buttons.interact = a.interact || b.interact;
  buttons.toggle_mode = a.toggle_mode || b.toggle_mode;
  buttons.hot_apply = a.hot_apply || b.hot_apply;
  buttons.cycle_camera = a.cycle_camera || b.cycle_camera;
  buttons.debug_snapshot = a.debug_snapshot || b.debug_snapshot;
  buttons.undo = a.undo || b.undo;
  buttons.redo = a.redo || b.redo;
  return buttons;
}

namespace {

using json = nlohmann::json;

struct ActionField {
  const char* name;
  ActionSources InputBindings::* member;
};

constexpr ActionField kActionFields[] = {
    {"move_up", &InputBindings::move_up},
    {"move_down", &InputBindings::move_down},
    {"move_left", &InputBindings::move_left},
    {"move_right", &InputBindings::move_right},
    {"jump", &InputBindings::jump},
    {"interact", &InputBindings::interact},
    {"toggle_mode", &InputBindings::toggle_mode},
    {"hot_apply", &InputBindings::hot_apply},
    {"cycle_camera", &InputBindings::cycle_camera},
    {"debug_snapshot", &InputBindings::debug_snapshot},
    {"undo", &InputBindings::undo},
    {"redo", &InputBindings::redo},
};

[[nodiscard]] json dump_chord(const KeyboardChord& chord) {
  if (!chord.ctrl.has_value() && !chord.shift.has_value()) {
    return chord.key;
  }
  json node = {{"key", chord.key}};
  if (chord.ctrl.has_value()) {
    node["ctrl"] = *chord.ctrl;
  }
  if (chord.shift.has_value()) {
    node["shift"] = *chord.shift;
  }
  return node;
}

[[nodiscard]] json dump_sources(const ActionSources& sources) {
  json keys = json::array();
  for (const KeyboardChord& chord : sources.keys) {
    keys.push_back(dump_chord(chord));
  }
  json buttons = json::array();
  for (int button : sources.gamepad_buttons) {
    buttons.push_back(button);
  }
  json axes = json::array();
  for (const GamepadAxisBinding& axis : sources.gamepad_axes) {
    json node = {{"axis", axis.axis}, {"negative", axis.negative}};
    if (axis.threshold != kGamepadAxisThreshold) {
      node["threshold"] = axis.threshold;
    }
    axes.push_back(std::move(node));
  }
  json node = json::object();
  if (!keys.empty()) {
    node["keys"] = std::move(keys);
  }
  if (!buttons.empty()) {
    node["gamepad_buttons"] = std::move(buttons);
  }
  if (!axes.empty()) {
    node["gamepad_axes"] = std::move(axes);
  }
  return node;
}

[[nodiscard]] KeyboardChord load_chord(const json& node) {
  KeyboardChord chord;
  if (node.is_string()) {
    chord.key = node.get<std::string>();
    return chord;
  }
  if (!node.is_object()) {
    return chord;
  }
  chord.key = node.value("key", std::string{});
  if (node.contains("ctrl")) {
    chord.ctrl = node.at("ctrl").get<bool>();
  }
  if (node.contains("shift")) {
    chord.shift = node.at("shift").get<bool>();
  }
  return chord;
}

[[nodiscard]] ActionSources load_sources(const json& node) {
  ActionSources sources;
  if (!node.is_object()) {
    return sources;
  }
  if (node.contains("keys") && node["keys"].is_array()) {
    for (const json& key : node["keys"]) {
      sources.keys.push_back(load_chord(key));
    }
  }
  if (node.contains("gamepad_buttons") && node["gamepad_buttons"].is_array()) {
    for (const json& button : node["gamepad_buttons"]) {
      if (button.is_number_integer()) {
        sources.gamepad_buttons.push_back(button.get<int>());
      }
    }
  }
  if (node.contains("gamepad_axes") && node["gamepad_axes"].is_array()) {
    for (const json& axis_node : node["gamepad_axes"]) {
      if (!axis_node.is_object()) {
        continue;
      }
      GamepadAxisBinding axis;
      axis.axis = axis_node.value("axis", -1);
      axis.negative = axis_node.value("negative", false);
      axis.threshold = axis_node.value("threshold", kGamepadAxisThreshold);
      sources.gamepad_axes.push_back(axis);
    }
  }
  return sources;
}

}  // namespace

std::string write_input_bindings_json(const InputBindings& bindings) {
  json actions = json::object();
  for (const ActionField& field : kActionFields) {
    actions[field.name] = dump_sources(bindings.*(field.member));
  }
  const json root = {{"schema_version", 1}, {"actions", std::move(actions)}};
  return root.dump(2);
}

std::optional<InputBindings> read_input_bindings_json(std::string_view text) {
  json root;
  try {
    root = json::parse(text.begin(), text.end());
  } catch (const json::parse_error&) {
    return std::nullopt;
  }
  if (!root.is_object() || !root.contains("actions") || !root["actions"].is_object()) {
    return std::nullopt;
  }
  const json& actions = root["actions"];
  InputBindings bindings;
  for (const ActionField& field : kActionFields) {
    if (actions.contains(field.name)) {
      bindings.*(field.member) = load_sources(actions[field.name]);
    }
  }
  return bindings;
}

}  // namespace rat
