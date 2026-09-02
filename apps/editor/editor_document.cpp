#include "editor_document.hpp"

#include <utility>

namespace rat {

void EditorDocument::note_changed() {
  changed_ = true;
}

void EditorDocument::clamp_selection() {
  const int n_blockers = static_cast<int>(map_.data().blockers.size());
  const int n_events = static_cast<int>(map_.data().events.size());
  if (n_blockers <= 0) {
    selected_blocker_ = -1;
  } else if (selected_blocker_ >= n_blockers) {
    selected_blocker_ = n_blockers - 1;
  }
  if (n_events <= 0) {
    selected_event_ = -1;
  } else if (selected_event_ >= n_events) {
    selected_event_ = n_events - 1;
  }
  if (selected_event_ < 0) {
    selected_page_ = 0;
    return;
  }
  const int n_pages =
      static_cast<int>(map_.data().events[static_cast<std::size_t>(selected_event_)].pages.size());
  if (n_pages <= 0 || selected_page_ >= n_pages) {
    selected_page_ = 0;
  }
}

void EditorDocument::apply_map(MapData map, const EditApplyResult& result) {
  if (!result.applied) {
    return;
  }
  discard_preview();
  map_.replace(std::move(map));
  dirty_ = true;
  last_mutated_elevation_ = result.mutates_elevation;
  clamp_selection();
  note_changed();
}

void EditorDocument::load(MapData data) {
  discard_preview();
  history_.clear();
  map_.replace(std::move(data));
  dirty_ = false;
  last_mutated_elevation_ = false;
  selected_blocker_ = map_.data().blockers.empty() ? -1 : 0;
  selected_event_ = map_.data().events.empty() ? -1 : 0;
  selected_page_ = 0;
  note_changed();
}

EditApplyResult EditorDocument::execute(std::unique_ptr<EditCommand> command) {
  if (command == nullptr) {
    return {};
  }
  discard_preview();
  MapData map = map_.data();
  const EditApplyResult result = history_.execute(map, std::move(command));
  apply_map(std::move(map), result);
  return result;
}

EditApplyResult EditorDocument::undo() {
  discard_preview();
  MapData map = map_.data();
  const EditApplyResult result = history_.undo(map);
  apply_map(std::move(map), result);
  return result;
}

EditApplyResult EditorDocument::redo() {
  discard_preview();
  MapData map = map_.data();
  const EditApplyResult result = history_.redo(map);
  apply_map(std::move(map), result);
  return result;
}

void EditorDocument::begin_stroke() {
  history_.begin_stroke();
}

void EditorDocument::end_stroke() {
  history_.end_stroke();
}

EditApplyResult EditorDocument::abort_stroke() {
  discard_preview();
  MapData map = map_.data();
  const EditApplyResult result = history_.abort_stroke(map);
  apply_map(std::move(map), result);
  return result;
}

void EditorDocument::mark_clean() {
  dirty_ = false;
}

void EditorDocument::clear_selection() {
  selected_blocker_ = -1;
  selected_event_ = -1;
  selected_page_ = 0;
}

void EditorDocument::select_blocker(int index) {
  selected_blocker_ = index;
  selected_event_ = -1;
  selected_page_ = 0;
  clamp_selection();
}

void EditorDocument::select_event(int index) {
  selected_blocker_ = -1;
  selected_event_ = index;
  selected_page_ = 0;
  clamp_selection();
}

void EditorDocument::set_selected_page(int page) {
  selected_page_ = page;
  clamp_selection();
}

bool EditorDocument::preview_blocker(int index, BlockerDef next) {
  if (index < 0 || static_cast<std::size_t>(index) >= map_.data().blockers.size()) {
    return false;
  }
  MapData preview = map_.data();
  preview.blockers[static_cast<std::size_t>(index)] = std::move(next);
  preview_ = std::move(preview);
  note_changed();
  return true;
}

bool EditorDocument::preview_event(int index, EventDef next) {
  if (index < 0 || static_cast<std::size_t>(index) >= map_.data().events.size()) {
    return false;
  }
  MapData preview = map_.data();
  preview.events[static_cast<std::size_t>(index)] = std::move(next);
  preview_ = std::move(preview);
  note_changed();
  return true;
}

void EditorDocument::discard_preview() {
  if (!preview_) {
    return;
  }
  preview_.reset();
  note_changed();
}

const MapData& EditorDocument::visible_data() const {
  return preview_ ? *preview_ : map_.data();
}

bool EditorDocument::consume_changed() {
  const bool changed = changed_;
  changed_ = false;
  return changed;
}

}  // namespace rat
