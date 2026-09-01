#pragma once

#include "rat/map_data.hpp"

#include <string>

namespace rat {

[[nodiscard]] const char* trigger_kind_name(TriggerKind trigger);
[[nodiscard]] std::string summarize_condition(const Condition& condition);
[[nodiscard]] std::string summarize_page_conditions(const EventPage& page);

// Ensures page has a Switch enable condition; returns its index.
[[nodiscard]] int ensure_page_enable_switch(EventPage& page, std::uint32_t switch_id = 1,
                                            bool required_on = true);

// First ShowText command index, or -1.
[[nodiscard]] int find_first_show_text(const EventPage& page);

}  // namespace rat
