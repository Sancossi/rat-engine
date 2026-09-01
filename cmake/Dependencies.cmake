include(FetchContent)

set(BGFX_BUILD_TOOLS OFF CACHE BOOL "Build bgfx tools" FORCE)
set(BGFX_BUILD_EXAMPLES OFF CACHE BOOL "Build bgfx examples" FORCE)
set(BGFX_CUSTOM_TARGETS OFF CACHE BOOL "bgfx custom targets" FORCE)
set(BGFX_INSTALL OFF CACHE BOOL "Install bgfx" FORCE)

FetchContent_Declare(
  bgfx_cmake
  GIT_REPOSITORY https://github.com/bkaradzic/bgfx.cmake.git
  GIT_TAG        v1.129.8940-496
  GIT_SUBMODULES "bgfx;bimg;bx"
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(bgfx_cmake)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)
set(GLFW_INSTALL OFF CACHE BOOL "" FORCE)

FetchContent_Declare(
  glfw
  GIT_REPOSITORY https://github.com/glfw/glfw.git
  GIT_TAG        3.4
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(glfw)

FetchContent_Declare(
  imgui
  GIT_REPOSITORY https://github.com/ocornut/imgui.git
  GIT_TAG        v1.91.8-docking
  GIT_SHALLOW    TRUE
)

FetchContent_MakeAvailable(imgui)

if(NOT TARGET imgui)
  add_library(imgui STATIC
    "${imgui_SOURCE_DIR}/imgui.cpp"
    "${imgui_SOURCE_DIR}/imgui_draw.cpp"
    "${imgui_SOURCE_DIR}/imgui_tables.cpp"
    "${imgui_SOURCE_DIR}/imgui_widgets.cpp"
    "${imgui_SOURCE_DIR}/imgui_demo.cpp"
    "${imgui_SOURCE_DIR}/backends/imgui_impl_glfw.cpp"
  )

  target_include_directories(imgui
    PUBLIC
      "${imgui_SOURCE_DIR}"
      "${imgui_SOURCE_DIR}/backends"
  )

  target_link_libraries(imgui PUBLIC glfw)
  target_compile_definitions(imgui PUBLIC IMGUI_DEFINE_MATH_OPERATORS)
endif()

# Embedded imgui shaders shipped with bgfx examples (for imgui_bgfx).
set(RAT_BGFX_IMGUI_SHADER_DIR "${bgfx_cmake_SOURCE_DIR}/bgfx/examples/common/imgui"
    CACHE INTERNAL "bgfx imgui shader headers")
set(RAT_BGFX_DEBUGDRAW_SHADER_DIR "${bgfx_cmake_SOURCE_DIR}/bgfx/examples/common/debugdraw"
    CACHE INTERNAL "bgfx debugdraw shader headers")

FetchContent_Declare(
  nlohmann_json
  GIT_REPOSITORY https://github.com/nlohmann/json.git
  GIT_TAG        v3.11.3
  GIT_SHALLOW    TRUE
)
FetchContent_MakeAvailable(nlohmann_json)

if(RAT_BUILD_TESTS)
  set(CATCH_INSTALL_DOCS OFF CACHE BOOL "" FORCE)
  set(CATCH_INSTALL_EXTRAS OFF CACHE BOOL "" FORCE)
  FetchContent_Declare(
    Catch2
    GIT_REPOSITORY https://github.com/catchorg/Catch2.git
    GIT_TAG        v3.7.1
    GIT_SHALLOW    TRUE
  )
  FetchContent_MakeAvailable(Catch2)
endif()
