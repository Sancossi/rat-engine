#include "rat/save_game.hpp"

#include <string>
#include <utility>

namespace rat {

GameFileResult save_game(FileStore& files, std::string_view path, const GameState& state) {
  std::string blob;
  if (!state.save_to_memory(blob)) {
    return GameFileResult{false, "failed to serialize game state"};
  }
  const FileWriteResult written = files.write_atomic(path, blob);
  if (!written.ok) {
    return GameFileResult{false, written.error.empty()
                                     ? "failed to write save file: " + std::string(path)
                                     : written.error};
  }
  return GameFileResult{true, {}};
}

GameFileResult load_game(const FileStore& files, std::string_view path, GameState& state) {
  const FileReadResult read = files.read(path);
  if (!read.ok) {
    return GameFileResult{false, read.error.empty()
                                     ? "failed to read save file: " + std::string(path)
                                     : read.error};
  }
  GameState parsed;
  if (!parsed.load_from_memory(read.bytes.as_text())) {
    return GameFileResult{false, "corrupt save file: " + std::string(path)};
  }
  state = std::move(parsed);
  return GameFileResult{true, {}};
}

}  // namespace rat
