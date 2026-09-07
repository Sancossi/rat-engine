#include "editor_action_controller.hpp"

namespace rat {
bool EditorActionController::request(PendingEditorAction action) {
  if (pending_) return false;
  pending_ = std::move(action);
  awaiting_decision_ = false;
  error_.clear();
  return true;
}
bool EditorActionController::settle_edit() {
  const auto result = settle ? settle() : EditorActionResult{};
  error_ = result.error;
  return result.ok;
}
void EditorActionController::pump() {
  if (!pending_ || awaiting_decision_) return;
  if (!settle_edit() || (dirty && dirty())) {
    awaiting_decision_ = true;
    return;
  }
  execute_pending();
}
void EditorActionController::choose(UnsavedChoice choice) {
  if (!pending_ || !awaiting_decision_) return;
  if (choice == UnsavedChoice::Cancel) { dismiss(); return; }
  if (choice == UnsavedChoice::Save) {
    if (!settle_edit()) return;
    const auto result = save ? save() : EditorActionResult{false, "Save is unavailable"};
    error_ = result.error;
    if (!result.ok) return;
  }
  execute_pending();
}
void EditorActionController::execute_pending() {
  const auto result = perform ? perform(*pending_) : EditorActionResult{false, "Action is unavailable"};
  if (!result.ok) {
    error_ = result.error;
    awaiting_decision_ = true;
    return;
  }
  dismiss();
}
void EditorActionController::dismiss() {
  pending_.reset();
  awaiting_decision_ = false;
  error_.clear();
  if (reset_input) reset_input();
}
}
