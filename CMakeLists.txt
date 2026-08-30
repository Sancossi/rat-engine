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

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake")

add_subdirectory(src/engine)

option(RAT_BUILD_EDITOR "Build GLFW + ImGui rat-editor application" ON)
if(RAT_BUILD_EDITOR)
  add_subdirectory(apps/editor)
endif()
