add_library(rat_engine
  src/engine.cpp
  src/renderer.cpp
)

add_library(rat::engine ALIAS rat_engine)

target_include_directories(rat_engine
  PUBLIC
    "${CMAKE_CURRENT_SOURCE_DIR}/include"
)

target_link_libraries(rat_engine
  PUBLIC
    bgfx
    bx
    bimg
)

target_compile_features(rat_engine PUBLIC cxx_std_20)

if(WIN32)
  target_compile_definitions(rat_engine PUBLIC NOMINMAX WIN32_LEAN_AND_MEAN)
endif()
