add_executable(rat_tests
  app_mode_test.cpp
  audio_queue_test.cpp
  buffered_press_test.cpp
  blocker_edit_test.cpp
  camera_test.cpp
  debug_snapshot_test.cpp
  elevation_mechanics_test.cpp
  edit_history_test.cpp
  event_edit_test.cpp
  event_inspect_test.cpp
  event_runtime_test.cpp
  event_why_not_test.cpp
  game_state_test.cpp
  gameplay_notify_test.cpp
  height_edit_test.cpp
  headless_mechanics_stub_test.cpp
  input_sequence_probe_test.cpp
  locomotion_test.cpp
  log_test.cpp
  hot_apply_test.cpp
  input_test.cpp
  map_loader_test.cpp
  player_jump_test.cpp
  player_test.cpp
  play_se_test.cpp
  surface_query_test.cpp
  terrain_geometry_test.cpp
  viewport_edit_test.cpp
)

target_link_libraries(rat_tests
  PRIVATE
    rat::core
    Catch2::Catch2WithMain
)

target_compile_definitions(rat_tests
  PRIVATE
    RAT_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(rat_tests)
