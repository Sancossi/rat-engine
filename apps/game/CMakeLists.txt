if(NOT RAT_BUILD_GAME)
  return()
endif()

add_executable(rat-game
  main.cpp launch_options.cpp
  ../platform/native_window.cpp ../platform/process.cpp ../platform/imgui_bgfx.cpp
  ../../src/game/expedition/scene.cpp ../../src/game/expedition/session.cpp)
rat_target_defaults(rat-game)
target_link_libraries(rat-game PRIVATE rat::engine glfw imgui)
target_include_directories(rat-game PRIVATE "${CMAKE_SOURCE_DIR}/src/game" "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
target_compile_definitions(rat-game PRIVATE GLFW_INCLUDE_NONE)
file(MAKE_DIRECTORY "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
file(COPY "${RAT_BGFX_IMGUI_SHADER_DIR}/vs_ocornut_imgui.bin.h" "${RAT_BGFX_IMGUI_SHADER_DIR}/fs_ocornut_imgui.bin.h"
  DESTINATION "${CMAKE_CURRENT_BINARY_DIR}/generated_shaders")
if(WIN32)
  target_link_libraries(rat-game PRIVATE user32 shell32)
endif()
add_custom_target(rat_game_data ALL
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data/expedition" "$<TARGET_FILE_DIR:rat-game>/data/expedition"
  COMMAND "${CMAKE_COMMAND}" -E copy_directory "${CMAKE_SOURCE_DIR}/data/fonts" "$<TARGET_FILE_DIR:rat-game>/data/fonts"
  COMMAND "${CMAKE_COMMAND}" -E make_directory "$<TARGET_FILE_DIR:rat-game>/licenses"
  COMMAND "${CMAKE_COMMAND}" -E copy "${CMAKE_SOURCE_DIR}/assets/expedition/credits.json" "$<TARGET_FILE_DIR:rat-game>/licenses/expedition-credits.json"
  VERBATIM)
add_dependencies(rat-game rat_game_data)
install(TARGETS rat-game RUNTIME DESTINATION .)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/expedition" DESTINATION data)
install(DIRECTORY "${CMAKE_SOURCE_DIR}/data/fonts" DESTINATION data)
install(FILES "${CMAKE_SOURCE_DIR}/assets/expedition/credits.json" DESTINATION licenses RENAME expedition-credits.json)
foreach(_dependency IN ITEMS bgfx_cmake glfw imgui nlohmann_json)
  install(DIRECTORY "${${_dependency}_SOURCE_DIR}/" DESTINATION "licenses/${_dependency}"
    FILES_MATCHING PATTERN "LICENSE*" PATTERN "COPYING*" PATTERN ".git" EXCLUDE)
endforeach()
