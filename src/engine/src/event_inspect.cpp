#include "rat/event_inspect.hpp"

#include <sstream>

namespace rat {

const char* trigger_kind_name(TriggerKind trigger) {
  switch (trigger) {
    case TriggerKind::Action:
      return "action";
    case TriggerKind::PlayerTouch:
      return "player_touch";
    case TriggerKind::EventTouch:
      return "event_touch";
    case TriggerKind::Autorun:
      return "autorun";
    case TriggerKind::Parallel:
      return "parallel";
  }
  return "action";
}

std::string summarize_condition(const Condition& condition) {
  std::ostringstream oss;
  switch (condition.type) {
    case ConditionType::Switch:
      oss << "SW" << condition.id << '=' << (condition.bool_value ? "ON" : "OFF");
      break;
    case ConditionType::Variable:
      oss << "VAR" << condition.id << " ? " << condition.int_value;
      break;
    case ConditionType::Item:
      oss << "ITEM " << condition.string_id << ">=" << condition.int_value;
      break;
    case ConditionType::SelfSwitch:
      oss << "SELF " << condition.self_switch << '=' << (condition.bool_value ? "ON" : "OFF");
      break;
  }
  return oss.str();
}

std::string summarize_page_conditions(const EventPage& page) {
  if (page.conditions.empty()) {
    return "(none)";
  }
  std::ostringstream oss;
  for (std::size_t i = 0; i < page.conditions.size(); ++i) {
    if (i > 0) {
      oss << "; ";
    }
    oss << summarize_condition(page.conditions[i]);
  }
  return oss.str();
}

int ensure_page_enable_switch(EventPage& page, std::uint32_t switch_id, bool required_on) {
  for (std::size_t i = 0; i < page.conditions.size(); ++i) {
    if (page.conditions[i].type == ConditionType::Switch) {
      return static_cast<int>(i);
    }
  }
  Condition condition;
  condition.type = ConditionType::Switch;
  condition.id = switch_id;
  condition.bool_value = required_on;
  page.conditions.push_back(condition);
  return static_cast<int>(page.conditions.size()) - 1;
}

int find_first_show_text(const EventPage& page) {
  for (std::size_t i = 0; i < page.commands.size(); ++i) {
    if (page.commands[i].op == CommandOp::ShowText) {
      return static_cast<int>(i);
    }
  }
  return -1;
}

}  // namespace rat
