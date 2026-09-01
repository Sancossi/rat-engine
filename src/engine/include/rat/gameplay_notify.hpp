#pragma once

#include <functional>
#include <string>
#include <vector>

namespace rat {

enum class GameplayNotifyKind { ItemPicked, DialogShown, Landed };

struct GameplayNotify {
  GameplayNotifyKind kind = GameplayNotifyKind::ItemPicked;
  std::string id;  // item id for ItemPicked; unused/empty otherwise
};

[[nodiscard]] inline const char* gameplay_notify_kind_name(GameplayNotifyKind kind) {
  switch (kind) {
    case GameplayNotifyKind::ItemPicked:
      return "ItemPicked";
    case GameplayNotifyKind::DialogShown:
      return "DialogShown";
    case GameplayNotifyKind::Landed:
      return "Landed";
  }
  return "ItemPicked";
}

class GameplayNotifyBus {
 public:
  using Handler = std::function<void(const GameplayNotify&)>;
  void subscribe(Handler handler);
  void post(const GameplayNotify& notify);  // snapshot, then call FIFO; subscribe during this post waits

 private:
  std::vector<Handler> handlers_;
};

}  // namespace rat
