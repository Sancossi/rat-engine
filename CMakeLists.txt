cmake_minimum_required(VERSION 3.24)

project(rat-engine
  VERSION 0.1.0
  DESCRIPTION "Game engine (bgfx + GLFW + ImGui)"
  LANGUAGES CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(MSVC)
  add_compile_options(/W4 /permissive-)
else()
  add_compile_options(-Wall -Wextra -Wpedantic)
endif()

option(RAT_BUILD_EDITOR "Build GLFW + ImGui rat-editor application" ON)
option(RAT_BUILD_TESTS "Build unit/mechanic tests (Catch2)" ON)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake")

add_subdirectory(src/engine)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RatCoreLinkCheck.cmake")
rat_assert_no_platform_graphics(rat_core)

add_subdirectory(apps/editor)

if(RAT_BUILD_TESTS)
  enable_testing()
  add_subdirectory(tests)
  rat_add_core_isolation_ctest()
endif()
