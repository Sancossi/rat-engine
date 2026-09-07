#pragma once
#include "editor_launch_options.hpp"

namespace rat {
// Captures executable/cwd/user directories and Unicode process arguments once.
[[nodiscard]] EditorLaunchResult editor_launch_from_process(int argc, char** argv);
} // namespace rat
