add_executable(rat_tests
  game_state_test.cpp
  headless_mechanics_stub_test.cpp
)

target_link_libraries(rat_tests
  PRIVATE
    rat::core
    Catch2::Catch2WithMain
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(rat_tests)
