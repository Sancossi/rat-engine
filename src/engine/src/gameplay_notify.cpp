#include "rat/gameplay_notify.hpp"

namespace rat {

void GameplayNotifyBus::subscribe(Handler handler) {
  handlers_.push_back(std::move(handler));
}

void GameplayNotifyBus::post(const GameplayNotify& notify) {
  for (const Handler& handler : handlers_) {
    handler(notify);
  }
}

}  // namespace rat
