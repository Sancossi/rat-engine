#include "rat/gameplay_notify.hpp"

namespace rat {

void GameplayNotifyBus::subscribe(Handler handler) {
  handlers_.push_back(std::move(handler));
}

void GameplayNotifyBus::post(const GameplayNotify& notify) {
  // Snapshot so subscribe during this post cannot invalidate iteration or run this turn.
  const std::vector<Handler> snapshot = handlers_;
  for (const Handler& handler : snapshot) {
    handler(notify);
  }
}

}  // namespace rat
