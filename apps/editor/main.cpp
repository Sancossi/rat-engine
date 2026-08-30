#include "editor_app.hpp"

int main() {
  rat::EditorApp app;
  if (!app.init()) {
    return 1;
  }
  return app.run();
}
