add_executable(rat_tests
  benchmark_quantiles_test.cpp
  retained_memory_test.cpp
  app_mode_test.cpp
  asset_registry_test.cpp
  audio_queue_test.cpp
  buffered_press_test.cpp
  blocker_edit_test.cpp
  camera_test.cpp
  clock_test.cpp
  collision_test.cpp
  debug_snapshot_test.cpp
  elevation_mechanics_test.cpp
  edit_history_test.cpp
  editor_launch_options_test.cpp
  editor_document_test.cpp
  event_edit_test.cpp
  event_graph_edit_test.cpp
  event_graph_test.cpp
  event_inspect_test.cpp
  event_runtime_test.cpp
  event_why_not_test.cpp
  entity_test.cpp
  file_store_test.cpp
  game_state_test.cpp
  gameplay_notify_test.cpp
  height_edit_test.cpp
  headless_mechanics_stub_test.cpp
  input_sequence_probe_test.cpp
  input_bindings_test.cpp
  locomotion_test.cpp
  log_test.cpp
  hot_apply_test.cpp
  indoor_volume_test.cpp
  input_test.cpp
  inventory_list_test.cpp
  map_document_test.cpp
  map_loader_test.cpp
  move_route_test.cpp
  player_jump_test.cpp
  player_test.cpp
  play_se_test.cpp
  replay_test.cpp
  render_world_test.cpp
  simulation_session_test.cpp
  smoke_test.cpp
  surface_query_test.cpp
  terrain_geometry_test.cpp
  viewport_edit_test.cpp
)

rat_target_defaults(rat_tests)

target_link_libraries(rat_tests
  PRIVATE
    rat::core
    rat::editor_logic
    Catch2::Catch2WithMain
)

target_compile_definitions(rat_tests
  PRIVATE
    RAT_TEST_DATA_DIR="${CMAKE_SOURCE_DIR}/data"
)

list(APPEND CMAKE_MODULE_PATH "${Catch2_SOURCE_DIR}/extras")
include(Catch)
catch_discover_tests(rat_tests PROPERTIES TIMEOUT 120)
