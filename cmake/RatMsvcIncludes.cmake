# Ninja must match the compiler's /showIncludes prefix byte for byte. Some MSVC
# installations have no English language resources; VSLANG does not change their
# output, and CMake's initial ABI probe can decode UTF-8 using the console OEM page.
# A directly redirected cl probe uses the Windows ANSI code page. Decode that
# output to Unicode; the Ninja generator then encodes the prefix for its console.
if(MSVC AND CMAKE_GENERATOR MATCHES "Ninja")
  set(_rat_probe_dir "${CMAKE_BINARY_DIR}/CMakeFiles/rat-msvc-includes")
  file(MAKE_DIRECTORY "${_rat_probe_dir}")
  file(WRITE "${_rat_probe_dir}/rat_msvc_include_probe.hpp" "// include prefix probe\n")
  file(WRITE "${_rat_probe_dir}/probe.cpp" "#include <rat_msvc_include_probe.hpp>\n")
  execute_process(
    COMMAND "${CMAKE_CXX_COMPILER}" /nologo /showIncludes /EP
      "/I${_rat_probe_dir}" "${_rat_probe_dir}/probe.cpp"
    OUTPUT_VARIABLE _rat_probe_stdout ERROR_VARIABLE _rat_probe_stderr
    RESULT_VARIABLE _rat_probe_result ENCODING ANSI
  )
  if(NOT _rat_probe_result EQUAL 0)
    message(FATAL_ERROR "MSVC include dependency probe failed: ${_rat_probe_stderr}")
  endif()
  string(REGEX MATCH "[^\r\n]*rat_msvc_include_probe[.]hpp" _rat_probe_line
    "${_rat_probe_stdout}${_rat_probe_stderr}")
  string(REGEX REPLACE "([A-Za-z]:[/\\\\]|[/\\\\][/\\\\]).*$" ""
    _rat_include_prefix "${_rat_probe_line}")
  if(NOT _rat_include_prefix OR _rat_include_prefix MATCHES "rat_msvc_include_probe")
    message(FATAL_ERROR "Cannot determine the actual MSVC include dependency prefix")
  endif()
  set(CMAKE_CXX_CL_SHOWINCLUDES_PREFIX "${_rat_include_prefix}")
  set(CMAKE_CL_SHOWINCLUDES_PREFIX "${_rat_include_prefix}")
endif()
