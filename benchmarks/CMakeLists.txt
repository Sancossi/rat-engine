add_executable(rat-benchmark main.cpp)
rat_target_defaults(rat-benchmark)
target_link_libraries(rat-benchmark PRIVATE rat::core rat::editor_logic)
target_compile_definitions(rat-benchmark PRIVATE
  RAT_BENCHMARK_COMPILER="${CMAKE_CXX_COMPILER_ID}-${CMAKE_CXX_COMPILER_VERSION}"
  RAT_BENCHMARK_BUILD_TYPE="${CMAKE_BUILD_TYPE}")
