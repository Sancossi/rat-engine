add_library(rat_editor_logic
  editor_launch_options.cpp
  editor_launch_options.hpp
  editor_action_controller.cpp
  editor_action_controller.hpp
  editor_document.cpp
  editor_document.hpp
  event_graph_edit.cpp
  event_graph_edit.hpp
  frame_coordinator.cpp
  frame_coordinator.hpp
)

add_library(rat::editor_logic ALIAS rat_editor_logic)
rat_target_defaults(rat_editor_logic)

target_include_directories(rat_editor_logic
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}"
)

target_link_libraries(rat_editor_logic
  PUBLIC
    rat::core
)

target_compile_features(rat_editor_logic PUBLIC cxx_std_20)

if(WIN32)
  target_compile_definitions(rat_editor_logic PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()

if(NOT RAT_BUILD_EDITOR AND NOT RAT_BUILD_GUI_TESTS)
  return()
endif()

add_library(rat_editor_app
  platform/launch_environment.cpp
  platform/launch_environment.hpp
  editor_app.cpp
  editor_app.hpp
  imgui_bgfx.cpp
  imgui_bgfx.hpp
  platform/native_window.cpp
  platform/native_window.hpp
  platform/miniaudio_sink.cpp
  platform/miniaudio_sink.hpp
  panels/blocker_panel.cpp
  panels/blocker_panel.hpp
  panels/terrain_panel.cpp
  panels/terrain_panel.hpp
  panels/event_panel.cpp
  panels/event_panel.hpp
  panels/event_graph_canvas.cpp
  panels/event_graph_canvas.hpp
  panels/event_graph_node_ui.cpp
  panels/event_graph_node_ui.hpp
  panels/event_graph_window.cpp
  panels/event_graph_window.hpp
  panels/ladder_panel.cpp
  panels/ladder_panel.hpp
)

set(_rat_imgui_shader_out "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
file(MAKE_DIRECTORY "${_rat_imgui_shader_out}")
file(COPY
  "${RAT_BGFX_IMGUI_SHADER_DIR}/vs_ocornut_imgui.bin.h"
  "${RAT_BGFX_IMGUI_SHADER_DIR}/fs_ocornut_imgui.bin.h"
  DESTINATION "${_rat_imgui_shader_out}"
)

add_library(rat::editor_app ALIAS rat_editor_app)
rat_target_defaults(rat_editor_app)

target_include_directories(rat_editor_app
  PUBLIC "${CMAKE_CURRENT_SOURCE_DIR}"
  PRIVATE
    "${_rat_imgui_shader_out}"
)

target_link_libraries(rat_editor_app
  PUBLIC rat::editor_logic
  PRIVATE
    rat::engine
    imgui
    glfw
    bgfx
    bx
    miniaudio
)

target_compile_definitions(rat_editor_app
  PRIVATE
    GLFW_INCLUDE_NONE
)

if(WIN32)
  target_link_libraries(rat_editor_app PRIVATE gdi32 user32 shell32 ole32)
  # miniaudio.h (WAV decoder) conversion inside the amalgamated header
  set_source_files_properties(platform/miniaudio_sink.cpp PROPERTIES COMPILE_OPTIONS /wd4244)
endif()

# Every frontend links the same app/panels/platform implementation.
if(RAT_BUILD_EDITOR)
  add_executable(rat-editor main.cpp)
  target_link_libraries(rat-editor PRIVATE rat::editor_app)
  rat_target_defaults(rat-editor)
  add_custom_target(rat_editor_data ALL
    COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data" "$<TARGET_FILE_DIR:rat-editor>/data"
    COMMENT "Copy executable-adjacent editor data"
    VERBATIM)
  add_dependencies(rat-editor rat_editor_data)
  install(TARGETS rat-editor RUNTIME DESTINATION .)
  install(DIRECTORY "${CMAKE_SOURCE_DIR}/data" DESTINATION .)
  foreach(_dependency IN ITEMS bgfx_cmake glfw imgui miniaudio nlohmann_json)
    install(DIRECTORY "${${_dependency}_SOURCE_DIR}/"
      DESTINATION "licenses/${_dependency}"
      FILES_MATCHING PATTERN "LICENSE*" PATTERN "COPYING*" PATTERN ".git" EXCLUDE)
  endforeach()
endif()
