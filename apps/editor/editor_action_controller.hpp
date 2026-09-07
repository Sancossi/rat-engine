#pragma once

#include <rat/map_data.hpp>
#include <functional>
#include <optional>
#include <string>

namespace rat {
enum class EditorActionKind { Close, LoadMap, RestoreMapBackup };
enum class UnsavedChoice { Save, Discard, Cancel };
struct PendingEditorAction {
  EditorActionKind kind = EditorActionKind::Close;
  std::string path;
  bool preserve_player = true;
  // Pin a validated backup before prompting: Save may rotate the .bak file.
  std::optional<MapData> candidate;
};
struct EditorActionResult {
  bool ok = true;
  std::string error;
};

class EditorActionController {
 public:
  std::function<EditorActionResult()> settle;
  std::function<bool()> dirty;
  std::function<EditorActionResult()> save;
  std::function<EditorActionResult(const PendingEditorAction&)> perform;
  std::function<void()> reset_input;

  bool request(PendingEditorAction action);
  // Pump after the UI has processed field deactivation for this frame.
  void pump();
  void choose(UnsavedChoice choice);
  [[nodiscard]] bool pending() const { return pending_.has_value(); }
  [[nodiscard]] bool awaiting_decision() const { return awaiting_decision_; }
  [[nodiscard]] const std::string& error() const { return error_; }
 private:
  bool settle_edit();
  void execute_pending();
  void dismiss();
  std::optional<PendingEditorAction> pending_;
  bool awaiting_decision_ = false;
  std::string error_;
};
}
