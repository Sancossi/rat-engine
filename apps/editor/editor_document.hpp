#pragma once

#include "event_graph_edit.hpp"

#include <rat/edit_history.hpp>
#include <rat/map_data.hpp>
#include <rat/map_document.hpp>

#include <memory>
#include <optional>

namespace rat {

class EditorDocument {
 public:
  EditorDocument() = default;

  void load(MapData data);
  void restore(MapData data);
  [[nodiscard]] EditApplyResult commit_preview();
  [[nodiscard]] const std::string& last_error() const { return last_error_; }
  [[nodiscard]] EditApplyResult execute(std::unique_ptr<EditCommand> command);
  [[nodiscard]] EditApplyResult undo();
  [[nodiscard]] EditApplyResult redo();
  void begin_stroke();
  void end_stroke();
  [[nodiscard]] EditApplyResult abort_stroke();
  void mark_clean();

  void clear_selection();
  void select_blocker(int index);
  void select_event(int index);
  void set_selected_page(int page);

  [[nodiscard]] bool preview_blocker(int index, BlockerDef next);
  [[nodiscard]] bool preview_event(int index, EventDef next);
  void discard_preview();

  [[nodiscard]] EventGraphApplyResult compile_graphs_for_apply() const;

  [[nodiscard]] bool consume_changed();
  [[nodiscard]] bool last_mutated_elevation() const { return last_mutated_elevation_; }

  [[nodiscard]] const MapDocument& map() const { return map_; }
  [[nodiscard]] const MapData& data() const { return map_.data(); }
  [[nodiscard]] const MapData& visible_data() const;
  [[nodiscard]] bool dirty() const { return dirty_; }
  [[nodiscard]] bool preview_active() const { return preview_.has_value(); }
  [[nodiscard]] bool can_undo() const { return history_.can_undo(); }
  [[nodiscard]] bool can_redo() const { return history_.can_redo(); }
  [[nodiscard]] bool in_stroke() const { return history_.in_stroke(); }
  [[nodiscard]] int selected_blocker() const { return selected_blocker_; }
  [[nodiscard]] int selected_event() const { return selected_event_; }
  [[nodiscard]] int selected_page() const { return selected_page_; }

 private:
  void apply_map(MapData map, const EditApplyResult& result);
  void clamp_selection();
  void note_changed();

  MapDocument map_{};
  EditHistory history_{};
  std::optional<MapData> preview_{};
  int preview_blocker_ = -1;
  int preview_event_ = -1;
  std::string last_error_;
  bool committing_preview_ = false;
  std::string clean_snapshot_;
  bool dirty_ = false;
  bool changed_ = false;
  bool last_mutated_elevation_ = false;
  int selected_blocker_ = -1;
  int selected_event_ = -1;
  int selected_page_ = 0;
};

}  // namespace rat
