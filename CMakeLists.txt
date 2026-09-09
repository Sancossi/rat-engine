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

option(RAT_ENABLE_SANITIZERS "Instrument own targets with ASan and UBSan (Linux GCC/Clang)" OFF)
if(RAT_ENABLE_SANITIZERS AND (NOT CMAKE_SYSTEM_NAME STREQUAL "Linux" OR NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang"))
  message(FATAL_ERROR "RAT_ENABLE_SANITIZERS requires Linux GCC or Clang")
endif()

option(RAT_BUILD_RENDERER "Build bgfx renderer library" ON)
option(RAT_BUILD_GUI_TESTS "Build real-editor GUI acceptance runner" OFF)
option(RAT_BUILD_GAME "Build standalone Rat Expedition application" OFF)
option(RAT_BUILD_EDITOR "Build GLFW + ImGui rat-editor application" ON)
option(RAT_BUILD_BENCHMARKS "Build headless performance baseline executable" OFF)
option(RAT_BUILD_TESTS "Build unit/mechanic tests (Catch2)" ON)

if((RAT_BUILD_EDITOR OR RAT_BUILD_GUI_TESTS OR RAT_BUILD_GAME) AND NOT RAT_BUILD_RENDERER)
  message(FATAL_ERROR "Frontends require RAT_BUILD_RENDERER=ON")
endif()

# Apply execution/source UTF-8 only to our targets, not dependency projects.
function(rat_target_defaults target)
  if(RAT_ENABLE_SANITIZERS)
    target_compile_options(${target} PRIVATE -fsanitize=address,undefined -fno-omit-frame-pointer -fno-sanitize-recover=all)
    target_link_options(${target} PRIVATE -fsanitize=address,undefined -fno-sanitize-recover=all)
  endif()
  if(MSVC)
    target_compile_options(${target} PRIVATE /utf-8)
  endif()
endfunction()

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/Dependencies.cmake")

add_subdirectory(src/engine)

include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RatCoreLinkCheck.cmake")
rat_assert_no_platform_graphics(rat_core)

if(RAT_BUILD_TESTS OR RAT_BUILD_GUI_TESTS)
  enable_testing()
endif()
add_subdirectory(apps/editor)
add_subdirectory(apps/game)

if(RAT_BUILD_BENCHMARKS)
  add_subdirectory(benchmarks)
endif()

if(RAT_BUILD_TESTS)
  add_subdirectory(tests)
  rat_add_core_isolation_ctest()
endif()

# Nested dependency projects may reload the cached compiler prefix. Set it last.
include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RatMsvcIncludes.cmake")

if(NOT RAT_BUILD_RENDERER AND NOT RAT_BUILD_EDITOR AND NOT RAT_BUILD_GUI_TESTS AND NOT RAT_BUILD_GAME)
  include("${CMAKE_CURRENT_SOURCE_DIR}/cmake/RatHeadlessCheck.cmake")
  rat_assert_headless_dependencies()
endif()

if(RAT_BUILD_EDITOR OR RAT_BUILD_GUI_TESTS OR RAT_BUILD_GAME)
  install(FILES "${CMAKE_SOURCE_DIR}/LICENSE" DESTINATION licenses RENAME rat-engine-LICENSE)
  if(MSVC)
    set(CMAKE_INSTALL_SYSTEM_RUNTIME_DESTINATION ".")
    include(InstallRequiredSystemLibraries)
  endif()
  set(CPACK_PACKAGE_NAME "rat-engine")
  set(CPACK_PACKAGE_VERSION "${PROJECT_VERSION}")
  set(CPACK_PACKAGE_FILE_NAME "rat-engine-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-${CMAKE_SYSTEM_PROCESSOR}")
  if(RAT_BUILD_GAME AND NOT RAT_BUILD_EDITOR AND NOT RAT_BUILD_GUI_TESTS)
    set(CPACK_PACKAGE_NAME "rat-expedition")
    set(CPACK_PACKAGE_FILE_NAME "rat-expedition-${PROJECT_VERSION}-${CMAKE_SYSTEM_NAME}-x64")
  endif()
  set(CPACK_GENERATOR "ZIP")
  set(CPACK_INCLUDE_TOPLEVEL_DIRECTORY ON)
  include(CPack)
endif()
