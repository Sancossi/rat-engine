add_library(rat_core
  src/app_mode.cpp
  src/asset.cpp
  src/audio.cpp
  src/buffered_press.cpp
  src/blocker_edit.cpp
  src/camera.cpp
  src/clock.cpp
  src/collision.cpp
  src/file_store.cpp
  src/debug_snapshot.cpp
  src/edit_history.cpp
  src/event_edit.cpp
  src/event_graph.cpp
  src/event_inspect.cpp
  src/event_runtime.cpp
  src/entity.cpp
  src/game_state.cpp
  src/gameplay_notify.cpp
  src/height_edit.cpp
  src/hot_apply.cpp
  src/input.cpp
  src/input_bindings.cpp
  src/input_sequence.cpp
  src/locomotion.cpp
  src/log.cpp
  src/map_document.cpp
  src/map_loader.cpp
  src/player.cpp
  src/player_jump.cpp
  src/replay.cpp
  src/render_world.cpp
  src/save_game.cpp
  src/simulation_session.cpp
  src/surface_query.cpp
  src/terrain_geometry.cpp
  src/viewport_edit.cpp
)

add_library(rat::core ALIAS rat_core)

target_include_directories(rat_core
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(rat_core
  PUBLIC
    nlohmann_json::nlohmann_json
)

target_compile_features(rat_core PUBLIC cxx_std_20)

if(WIN32)
  target_compile_definitions(rat_core PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

add_library(rat_engine
  src/engine.cpp
  src/greybox.cpp
  src/renderer.cpp
)

add_library(rat::engine ALIAS rat_engine)

set(_rat_debugdraw_shader_out "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
file(MAKE_DIRECTORY "${_rat_debugdraw_shader_out}")
file(COPY
  "${RAT_BGFX_DEBUGDRAW_SHADER_DIR}/vs_debugdraw_lines.bin.h"
  "${RAT_BGFX_DEBUGDRAW_SHADER_DIR}/fs_debugdraw_lines.bin.h"
  DESTINATION "${_rat_debugdraw_shader_out}"
)

target_include_directories(rat_engine
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
  PRIVATE
    "${_rat_debugdraw_shader_out}"
)

target_link_libraries(rat_engine
  PUBLIC
    rat::core
    bgfx
    bx
    bimg
)

target_compile_features(rat_engine PUBLIC cxx_std_20)

if(WIN32)
  target_compile_definitions(rat_engine PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
