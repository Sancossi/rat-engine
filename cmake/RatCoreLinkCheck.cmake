# Enforces rat_core isolation from platform graphics.
# Forbidden: GLFW, ImGui, bgfx (plus bx/bimg), Win32 user32/gdi32, and miniaudio.
#
# Dual-mode file:
# - include() from CMakeLists.txt: walk LINK_LIBRARIES at configure time
# - cmake -P with RAT_LINK_MANIFEST: re-check the written closure (ctest / custom target)

set(_RAT_FORBIDDEN_LINK_NAMES glfw glfw3 imgui bgfx bx bimg gdi32 user32 miniaudio)

if(CMAKE_SCRIPT_MODE_FILE)
  if(NOT DEFINED RAT_LINK_MANIFEST OR RAT_LINK_MANIFEST STREQUAL "")
    message(FATAL_ERROR "RAT_LINK_MANIFEST is required")
  endif()
  if(NOT EXISTS "${RAT_LINK_MANIFEST}")
    message(FATAL_ERROR "rat_core link manifest missing: ${RAT_LINK_MANIFEST}")
  endif()
  file(STRINGS "${RAT_LINK_MANIFEST}" _rat_manifest_names)
  foreach(_name IN LISTS _rat_manifest_names)
    string(STRIP "${_name}" _name)
    string(TOLOWER "${_name}" _name)
    if(_name STREQUAL "")
      continue()
    endif()
    foreach(_forbidden IN LISTS _RAT_FORBIDDEN_LINK_NAMES)
      if(_name STREQUAL _forbidden)
        message(FATAL_ERROR
          "rat_core links forbidden library '${_name}' "
          "(must not link GLFW, ImGui, bgfx, Win32 user32/gdi32, or miniaudio)")
      endif()
    endforeach()
  endforeach()
  message(STATUS "rat_core link closure has no GLFW/ImGui/bgfx/Win32/miniaudio")
  return()
endif()

set(RAT_CORE_LINK_CHECK_FILE "${CMAKE_CURRENT_LIST_DIR}/RatCoreLinkCheck.cmake")

function(rat_normalize_link_token item out_var)
  set(token "${item}")
  if(token MATCHES "^\\$<LINK_ONLY:(.+)>$")
    set(token "${CMAKE_MATCH_1}")
  elseif(token MATCHES "^\\$<BUILD_INTERFACE:(.+)>$")
    set(token "${CMAKE_MATCH_1}")
  elseif(token MATCHES "^\\$<INSTALL_INTERFACE:(.+)>$")
    set(token "${CMAKE_MATCH_1}")
  endif()
  if(token MATCHES "^-l(.+)$")
    set(token "${CMAKE_MATCH_1}")
  endif()
  get_filename_component(fname "${token}" NAME)
  if(NOT fname STREQUAL "")
    set(token "${fname}")
  endif()
  string(REGEX REPLACE "^lib" "" token "${token}")
  string(REGEX REPLACE "\\.(lib|a|so|dll|dylib)$" "" token "${token}")
  if(token MATCHES "::([^:]+)$")
    set(token "${CMAKE_MATCH_1}")
  endif()
  string(TOLOWER "${token}" token)
  set(${out_var} "${token}" PARENT_SCOPE)
endfunction()

function(rat_token_is_forbidden token out_var)
  string(TOLOWER "${token}" t)
  string(STRIP "${t}" t)
  if(t STREQUAL "")
    set(${out_var} FALSE PARENT_SCOPE)
    return()
  endif()
  foreach(_forbidden IN ITEMS glfw glfw3 imgui bgfx bx bimg gdi32 user32 miniaudio)
    if(t STREQUAL _forbidden)
      set(${out_var} TRUE PARENT_SCOPE)
      return()
    endif()
  endforeach()
  # Catch glfw3dll / glfw3_mt style filenames if someone links a path instead of the target.
  if(t MATCHES "^glfw")
    set(${out_var} TRUE PARENT_SCOPE)
    return()
  endif()
  set(${out_var} FALSE PARENT_SCOPE)
endfunction()

function(rat_collect_link_closure target visited_var names_var)
  set(_visited "${${visited_var}}")
  set(_names "${${names_var}}")

  if(target STREQUAL "" OR target STREQUAL "debug" OR target STREQUAL "optimized"
      OR target STREQUAL "general")
    set(${visited_var} "${_visited}" PARENT_SCOPE)
    set(${names_var} "${_names}" PARENT_SCOPE)
    return()
  endif()

  if(TARGET "${target}")
    get_target_property(_aliased "${target}" ALIASED_TARGET)
    if(_aliased)
      set(target "${_aliased}")
    endif()

    if(target IN_LIST _visited)
      set(${visited_var} "${_visited}" PARENT_SCOPE)
      set(${names_var} "${_names}" PARENT_SCOPE)
      return()
    endif()
    list(APPEND _visited "${target}")

    rat_normalize_link_token("${target}" _tok)
    list(APPEND _names "${_tok}")

    set(_items "")
    foreach(_prop LINK_LIBRARIES INTERFACE_LINK_LIBRARIES)
      get_target_property(_val "${target}" ${_prop})
      if(_val)
        list(APPEND _items ${_val})
      endif()
    endforeach()

    foreach(_item IN LISTS _items)
      rat_collect_link_closure("${_item}" _visited _names)
    endforeach()
  else()
    rat_normalize_link_token("${target}" _tok)
    if(NOT _tok STREQUAL "")
      list(APPEND _names "${_tok}")
    endif()
    string(TOLOWER "${target}" _raw)
    foreach(_forbidden IN ITEMS glfw glfw3 imgui bgfx bx bimg gdi32 user32 miniaudio)
      if(_raw MATCHES "(^|[^a-z0-9_])${_forbidden}([^a-z0-9_]|$)")
        list(APPEND _names "${_forbidden}")
      endif()
    endforeach()
  endif()

  list(REMOVE_DUPLICATES _names)
  set(${visited_var} "${_visited}" PARENT_SCOPE)
  set(${names_var} "${_names}" PARENT_SCOPE)
endfunction()

function(rat_assert_no_platform_graphics target)
  if(NOT TARGET "${target}")
    message(FATAL_ERROR "rat_assert_no_platform_graphics: unknown target '${target}'")
  endif()

  set(_visited "")
  set(_names "")
  rat_collect_link_closure("${target}" _visited _names)

  set(_manifest "${CMAKE_BINARY_DIR}/${target}_link_closure.txt")
  list(JOIN _names "\n" _manifest_text)
  file(WRITE "${_manifest}" "${_manifest_text}\n")

  set(_hit "")
  foreach(_name IN LISTS _names)
    rat_token_is_forbidden("${_name}" _bad)
    if(_bad)
      set(_hit "${_name}")
      break()
    endif()
  endforeach()

  if(NOT _hit STREQUAL "")
    message(FATAL_ERROR
      "${target} links forbidden library '${_hit}' "
      "(must not link GLFW, ImGui, bgfx, Win32 user32/gdi32, or miniaudio). "
      "Closure:\n  ${_manifest_text}")
  endif()

  add_custom_target(${target}_link_check
    COMMAND "${CMAKE_COMMAND}"
      "-DRAT_LINK_MANIFEST=${_manifest}"
      -P "${RAT_CORE_LINK_CHECK_FILE}"
    COMMENT "Verify ${target} does not link GLFW, ImGui, bgfx, Win32, or miniaudio"
    VERBATIM
  )

  set(RAT_CORE_LINK_MANIFEST "${_manifest}" PARENT_SCOPE)
endfunction()

function(rat_add_core_isolation_ctest)
  if(NOT DEFINED RAT_CORE_LINK_MANIFEST)
    message(FATAL_ERROR "rat_add_core_isolation_ctest: run rat_assert_no_platform_graphics first")
  endif()
  add_test(
    NAME rat_core_no_platform_graphics
    COMMAND "${CMAKE_COMMAND}"
      "-DRAT_LINK_MANIFEST=${RAT_CORE_LINK_MANIFEST}"
      -P "${RAT_CORE_LINK_CHECK_FILE}"
  )
endfunction()
