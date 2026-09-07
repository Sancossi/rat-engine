#include "editor_app.hpp"
#include "platform/launch_environment.hpp"
#include <iostream>

int main(int argc, char** argv) {
  const auto launch = rat::editor_launch_from_process(argc, argv);
  if (!launch.ok) {
    std::cerr << launch.error << '\n' << rat::editor_launch_usage();
    return 2;
  }
  if (launch.help) { std::cout << rat::editor_launch_usage(); return 0; }
  rat::EditorApp app;
  if (!app.init(launch.options)) return 1;
  return app.run();
}
