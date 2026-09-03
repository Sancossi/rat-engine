#pragma once

#include "rat/file_store.hpp"
#include "rat/game_state.hpp"

#include <string>
#include <string_view>

namespace rat {

struct GameFileResult {
  bool ok = false;
  std::string error;
};

[[nodiscard]] GameFileResult save_game(FileStore& files, std::string_view path,
                                       const GameState& state);
[[nodiscard]] GameFileResult load_game(const FileStore& files, std::string_view path,
                                       GameState& state);

}  // namespace rat
