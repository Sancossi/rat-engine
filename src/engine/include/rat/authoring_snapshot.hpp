#pragma once

#include "rat/map_data.hpp"
#include <string>

namespace rat {
// Canonical authored content, including incomplete drafts. Never validates or
// compiles a graph. Derived commands are excluded when an authored graph exists.
[[nodiscard]] std::string authoring_snapshot(const MapData& map);
}
