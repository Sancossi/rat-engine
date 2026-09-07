#include "editor_document.hpp"
#include <rat/retained_memory.hpp>

#include <utility>
#include <rat/authoring_snapshot.hpp>

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
  if (!result.ok || !result.changed) {
    return;
  }
  discard_preview();
  map_.replace(std::move(map));
  dirty_ = authoring_snapshot(map_.data()) != clean_snapshot_;
  last_mutated_elevation_ = result.mutates_elevation;
  clamp_selection();
  note_changed();
}

void EditorDocument::load(MapData data) {
  discard_preview();
  history_.clear();
  last_error_.clear();
  map_.replace(std::move(data));
  clean_snapshot_ = authoring_snapshot(map_.data());
  dirty_ = false;
  last_mutated_elevation_ = false;
  selected_blocker_ = map_.data().blockers.empty() ? -1 : 0;
  selected_event_ = map_.data().events.empty() ? -1 : 0;
  selected_page_ = 0;
  note_changed();
}

void EditorDocument::restore(MapData data) {
  const auto baseline = clean_snapshot_;
  load(std::move(data));
  clean_snapshot_ = baseline;
  dirty_ = authoring_snapshot(map_.data()) != clean_snapshot_;
}

EditApplyResult EditorDocument::commit_preview() {
  if (!preview_) return {true, false};
  const auto issues = validate_map_structure(*preview_);
  if (map_issues_have_errors(issues)) {
    last_error_ = format_map_issues(issues);
    return {false, false, false, false, false, last_error_};
  }
  committing_preview_ = true;
  struct ResetCommitFlag { bool& flag; ~ResetCommitFlag() { flag = false; } } reset{committing_preview_};
  if (preview_event_ >= 0)
    return execute(make_replace_event_command(static_cast<std::size_t>(preview_event_),
      preview_->events[static_cast<std::size_t>(preview_event_)]));
  if (preview_blocker_ >= 0)
    return execute(make_replace_blocker_command(static_cast<std::size_t>(preview_blocker_),
      preview_->blockers[static_cast<std::size_t>(preview_blocker_)]));
  return {false, false, false, false, false, "preview has no edit target"};
}

EventGraphApplyResult EditorDocument::compile_graphs_for_apply() const {
  return compile_event_graphs_for_apply(map_.data());
}

EditApplyResult EditorDocument::execute(std::unique_ptr<EditCommand> command) {
  if (!command) { last_error_ = "missing edit command"; return {false, false, false, false, false, last_error_}; }
  if (!committing_preview_ && preview_) {
    const auto settled = commit_preview();
    if (!settled.ok) return settled;
  }
  MapData map = map_.data();
  const EditApplyResult result = history_.execute(map, std::move(command));
  last_error_ = result.error;
  if (result.ok) discard_preview();
  apply_map(std::move(map), result);
  return result;
}

EditApplyResult EditorDocument::undo() {
  if (const auto settled = commit_preview(); !settled.ok) return settled;
  MapData map = map_.data();
  const EditApplyResult result = history_.undo(map);
  apply_map(std::move(map), result);
  return result;
}

EditApplyResult EditorDocument::redo() {
  if (const auto settled = commit_preview(); !settled.ok) return settled;
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
  clean_snapshot_ = authoring_snapshot(map_.data());
  dirty_ = false;
}

void EditorDocument::clear_selection() {
  if (!commit_preview().ok) return;
  selected_blocker_ = -1;
  selected_event_ = -1;
  selected_page_ = 0;
}

void EditorDocument::select_blocker(int index) {
  if (!commit_preview().ok) return;
  selected_blocker_ = index;
  selected_event_ = -1;
  selected_page_ = 0;
  clamp_selection();
}

void EditorDocument::select_event(int index) {
  if (!commit_preview().ok) return;
  selected_blocker_ = -1;
  selected_event_ = index;
  selected_page_ = 0;
  clamp_selection();
}

void EditorDocument::set_selected_page(int page) {
  if (page != selected_page_ && !commit_preview().ok) return;
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
  preview_blocker_ = index;
  preview_event_ = -1;
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
  preview_event_ = index;
  preview_blocker_ = -1;
  note_changed();
  return true;
}

void EditorDocument::discard_preview() {
  if (!preview_) {
    return;
  }
  preview_.reset();
  preview_blocker_ = preview_event_ = -1;
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

EditorDocumentMemory EditorDocument::estimated_retained_memory() const {
  return {history_.estimated_retained_memory(), sizeof(*this) - sizeof(history_),
      retained_dynamic_bytes(map_.data()), preview_ ? retained_dynamic_bytes(*preview_) : 0,
      retained_dynamic_bytes(clean_snapshot_), retained_dynamic_bytes(last_error_)};
}

}  // namespace rat
