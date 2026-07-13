cmake_minimum_required(VERSION 3.30)
project(DuetScreen C CXX)
include(FetchContent)
include(ExternalProject)
include(CTest) # Enables BUILD_TESTING and CTest integration

# Enable Interprocedural Optimization (LTO) when supported
if(POLICY CMP0069)
  cmake_policy(SET CMP0069 NEW)
endif()
include(CheckIPOSupported)
check_ipo_supported(RESULT ipo_supported OUTPUT ipo_output)
if(ipo_supported)
  message(STATUS "LTO/IPO supported: enabling for Release and RelWithDebInfo")
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELEASE ON)
  set(CMAKE_INTERPROCEDURAL_OPTIMIZATION_RELWITHDEBINFO ON)
else()
  message(STATUS "LTO/IPO not supported: ${ipo_output}")
endif()

# Set C and C++ standards
set(CMAKE_C_STANDARD 23)
set(CMAKE_CXX_STANDARD 26)
set(CMAKE_C_STANDARD_REQUIRED ON)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
# CMake 3.30+ may enable C++ module scanning for C++26 builds, which adds
# -fmodules-ts/-fmodule-mapper and breaks GCC 15 + macOS SDK header parsing in
# this non-modules codebase. Keep scanning disabled unless named modules are used.
set(CMAKE_CXX_SCAN_FOR_MODULES OFF)
set(WORKING_DIRECTORY ${PROJECT_SOURCE_DIR})
set(SRC_DIR ${PROJECT_SOURCE_DIR}/src)
set(INCLUDE_DIR ${PROJECT_SOURCE_DIR}/include)
set(LIBRARIES_DIR ${PROJECT_SOURCE_DIR}/libraries)
# set(EXECUTABLE_OUTPUT_PATH ${PROJECT_SOURCE_DIR}/bin)

# ----------------------------------------------------------------------------------
# Build acceleration helpers (ccache, split DWARF, debug info level, unity, PCH)
# ----------------------------------------------------------------------------------

# Prefer ccache if available (speeds up rebuilds substantially)
option(ENABLE_CCACHE "Enable ccache if available" ON)
if (ENABLE_CCACHE)
  find_program(CCACHE_PROGRAM ccache)
  if(CCACHE_PROGRAM)
    message(STATUS "Enabling ccache: ${CCACHE_PROGRAM}")
    set(CMAKE_C_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
    set(CMAKE_CXX_COMPILER_LAUNCHER "${CCACHE_PROGRAM}")
  endif()
endif()

# Remove ARM-specific compile and linker options
if(PROCESSOR STREQUAL "Simulation")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -march=native")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=native")
endif()

# Set optimization level based on build type
if(CMAKE_BUILD_TYPE STREQUAL "Release")
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O3")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
elseif(CMAKE_BUILD_TYPE STREQUAL "RelWithDebInfo")
  if(CMAKE_CXX_COMPILER_ID MATCHES "Clang")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -O2 -gline-tables-only")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -gline-tables-only")
  elseif(CMAKE_CXX_COMPILER_ID MATCHES "GNU")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -O2 -g1")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2 -g1")
  endif()
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
  # Use DWARF 4 for broad GDB compatibility and -g2 (the default for -g) for
  # full variable/type info without macro tables (-g3), keeping binary size in check.
  # Suppress -gpubnames to avoid the huge .debug_gnu_pub{names,types} sections
  # that inflate the binary and slow down GDB loading.
  set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -O0 -g2 -gdwarf-4 -gno-pubnames")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O0 -g2 -gdwarf-4 -gno-pubnames")
endif()

# Reduce debug info size and speed up links by using split DWARF or similar features when available.
set(PIPE ON)
option(DUETSCREEN_SPLIT_DWARF "Split DWARF debug info for faster links" OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  if(DUETSCREEN_SPLIT_DWARF AND CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU")
    set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -gsplit-dwarf")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gsplit-dwarf")
    set(PIPE OFF)
  endif()
endif()

# Use pipes instead of temp files when compiling (reduces disk IO on GCC/Clang)
if(CMAKE_CXX_COMPILER_ID MATCHES "Clang|GNU" AND PIPE AND NOT DUETSCREEN_SAVE_TEMPS)
  set(CMAKE_C_FLAGS   "${CMAKE_C_FLAGS} -pipe")
  set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -pipe")
endif()

# When using a non-system GCC toolchain, make the resulting binaries prefer that
# toolchain's runtime libraries so they do not pick an older system libstdc++.
set(_DUETSCREEN_GCC_RUNTIME_DIRS "")
if(CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  foreach(_duetscreen_runtime_lib IN ITEMS libstdc++.so libgcc_s.so.1)
    execute_process(
      COMMAND ${CMAKE_CXX_COMPILER} -print-file-name=${_duetscreen_runtime_lib}
      OUTPUT_VARIABLE _duetscreen_runtime_path
      OUTPUT_STRIP_TRAILING_WHITESPACE
    )

    if(_duetscreen_runtime_path AND NOT _duetscreen_runtime_path STREQUAL _duetscreen_runtime_lib)
      get_filename_component(_duetscreen_runtime_dir "${_duetscreen_runtime_path}" DIRECTORY)
      list(APPEND _DUETSCREEN_GCC_RUNTIME_DIRS "${_duetscreen_runtime_dir}")
    endif()
  endforeach()

  list(REMOVE_DUPLICATES _DUETSCREEN_GCC_RUNTIME_DIRS)
endif()

# --------------------------------------------------------------------------------------------------------
# Options. These are mirrored as PUBLIC DuetScreen.lib compile definitions without the DUETSCREEN_ prefix.
# --------------------------------------------------------------------------------------------------------
set(PROCESSOR "T113" CACHE STRING "Target processor, 'T113' or 'Simulation'")
option(DUETSCREEN_DEBUG_BORDERS "Enable debug borders" OFF)
option(DUETSCREEN_LOG_TIMESTAMPS "Enable timestamps in log messages" ON)
option(DUETSCREEN_LOG_THREAD "Enable logging thread id in log messages" OFF)
option(DUETSCREEN_ASYNC_RR_MODEL "Enable asynchronous rr_model requests" OFF)
option(DUETSCREEN_HARDWARE_TEST "Enable hardware test" OFF)
set(DUETSCREEN_HARDWARE_TEST_SERIAL_NUMBER "\"\"" CACHE STRING "Default serial number to use in hardware test mode")
option(DUETSCREEN_BURNIN_TEST "Enable burnin test" OFF)
option(DUETSCREEN_ENABLE_CONSOLE_SHELL "Enable sending shell commands via the GUI console" OFF)
option(DUETSCREEN_LV_NESTED_SHOW_HIDE "Enable nested show/hide events for LvObj" ON)
option(DUETSCREEN_ENABLE_PROFILING "Enable profiling using Tracy" OFF)
option(DUETSCREEN_ENABLE_LV_PROFILING "Enable lvgl profiling" ${DUETSCREEN_ENABLE_PROFILING})
option(DUETSCREEN_USE_FIXED_TEST_STRINGS "Use fixed strings for UI labels like build date/time for unit tests" OFF)
option(DUETSCREEN_USE_LV_ANIMATION "Enable LVGL animations" ON)
option(DUETSCREEN_CONSOLE_SIDE_PANEL "Show console in side panel next to main window" OFF)

set(_dev_default OFF)
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  set(_dev_default ON)
endif()
set(DUETSCREEN_DEVELOPER_MODE ${_dev_default} CACHE BOOL "Enable developer mode features")
if (DUETSCREEN_DEVELOPER_MODE)
  message(STATUS "Developer mode enabled: enabling SSH/ADB settings and console shell")
  set(DUETSCREEN_ENABLE_CONSOLE_SHELL ON CACHE BOOL "Enable sending shell commands via the GUI console" FORCE)
endif()

# Hardware test requires developer mode to enable ADB
if (DUETSCREEN_HARDWARE_TEST)
  set(DUETSCREEN_DEVELOPER_MODE ON CACHE BOOL "Enable developer mode features" FORCE)
endif()

# UI Options
set(DUETSCREEN_UI_SETTINGS_TOGGLE_WIDGET "LvCheckbox" CACHE STRING "Widget type for settings toggles: 'LvCheckbox' or 'LvSwitch'")
set_property(CACHE DUETSCREEN_UI_SETTINGS_TOGGLE_WIDGET PROPERTY STRINGS LvCheckbox LvSwitch) # constrain valid values
option(DUETSCREEN_UI_MODAL_CLOSE_BUTTON "Enable close button on modals" OFF)

if(NOT DEFINED PROCESSOR)
  message(
    FATAL_ERROR
      "PROCESSOR variable is not defined. Please set it to 'T113' or 'Simulation'."
  )
endif()

if (NOT PROCESSOR STREQUAL "T113" AND NOT PROCESSOR STREQUAL "Simulation")
  message(
    FATAL_ERROR
      "Invalid PROCESSOR value: ${PROCESSOR}. Please set it to 'T113' or 'Simulation'."
  )
endif()


add_library(DuetScreen.lib)
add_executable(DuetScreen)

if(_DUETSCREEN_GCC_RUNTIME_DIRS)
  set_target_properties(
    DuetScreen.lib
    PROPERTIES BUILD_RPATH "${_DUETSCREEN_GCC_RUNTIME_DIRS}"
               INSTALL_RPATH "${_DUETSCREEN_GCC_RUNTIME_DIRS}")
  set_target_properties(
    DuetScreen
    PROPERTIES BUILD_RPATH "${_DUETSCREEN_GCC_RUNTIME_DIRS}"
               INSTALL_RPATH "${_DUETSCREEN_GCC_RUNTIME_DIRS}")
endif()

# Optionally save preprocessed/assembly temps for investigation (OFF by default)
option(DUETSCREEN_SAVE_TEMPS "Save intermediate compile outputs for analysis" OFF)
if (DUETSCREEN_SAVE_TEMPS AND DUETSCREEN_USE_PCH)
  message(FATAL_ERROR "DUETSCREEN_SAVE_TEMPS must be disabled when DUETSCREEN_USE_PCH is enabled")
endif()
if(DUETSCREEN_SAVE_TEMPS AND NOT DUETSCREEN_USE_PCH)
  target_compile_options(DuetScreen.lib PUBLIC --save-temps)
  # add_compile_options(-ftime-report)
  # add_compile_options(-H)
endif()

# Optional unity build to reduce translation unit count (CMake >= 3.16)
option(DUETSCREEN_UNITY_BUILD "Enable CMake unity build for faster compiles" OFF)
if(DUETSCREEN_UNITY_BUILD AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
  set_target_properties(DuetScreen.lib PROPERTIES UNITY_BUILD ON)
  set_target_properties(DuetScreen     PROPERTIES UNITY_BUILD ON)
endif()

# Optional precompiled headers for common libc++ headers (CMake >= 3.16)
option(DUETSCREEN_USE_PCH "Enable precompiled headers for common includes" OFF)
if(DUETSCREEN_USE_PCH AND CMAKE_VERSION VERSION_GREATER_EQUAL "3.16")
  # Keep this conservative (std headers only) to avoid macro/ODR surprises.
  # Apply PCH only to C++ compilations to avoid injecting C++ headers into C units.
    set(_DUETSCREEN_PCH_CXX
      "$<$<COMPILE_LANGUAGE:CXX>:<array$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<chrono$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<cstdint$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<functional$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<memory$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<optional$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<map$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<span$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<string$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<string_view$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<utility$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<variant$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:<vector$<ANGLE-R>>"
      "$<$<COMPILE_LANGUAGE:CXX>:Debug.h>"
      "\"lvgl/lvgl.h\""
    )
  target_precompile_headers(DuetScreen.lib PRIVATE ${_DUETSCREEN_PCH_CXX})
  target_precompile_headers(DuetScreen     PRIVATE ${_DUETSCREEN_PCH_CXX})
endif()

# ----------------------------------------------------------------------------
# Libraries
# ----------------------------------------------------------------------------

# Color manipulation library
include (${LIBRARIES_DIR}/colorm.cmake)

# option(BUILD_SHARED_LIBS "Set default library type to shared" OFF)

# Networking
include(${LIBRARIES_DIR}/hv.cmake)

# Reflection
include(${LIBRARIES_DIR}/nameof.cmake)
include(${LIBRARIES_DIR}/magic_enum.cmake)

# Find libusb package
find_package(PkgConfig REQUIRED)
pkg_check_modules(LIBUSB REQUIRED libusb-1.0)

# Find and include SDL2 library
find_package(SDL2 REQUIRED)

# Find json library
include(${LIBRARIES_DIR}/json.cmake)

# Logging
include(${LIBRARIES_DIR}/spdlog.cmake)
include(${LIBRARIES_DIR}/tracy.cmake)

# LVGL
include(${LIBRARIES_DIR}/lvgl.cmake)

# Include libusb headers
target_include_directories(DuetScreen.lib SYSTEM PUBLIC ${LIBUSB_INCLUDE_DIRS})
if(LIBUSB_LIBRARY_DIRS)
  # Ensure libusb library directory is on the link line (needed on macOS)
  target_link_directories(DuetScreen.lib PUBLIC ${LIBUSB_LIBRARY_DIRS})
endif()

# ----------------------------------------------------------------------------
# Set source files for executable
# ----------------------------------------------------------------------------
file(GLOB_RECURSE DUET_LIB_SOURCES ${LIBRARIES_DIR}/Duet3D/**.c
${LIBRARIES_DIR}/Duet3D/**.cpp)

target_sources(DuetScreen.lib PRIVATE ${DUET_LIB_SOURCES})

# Update DuetScreen version.h
add_custom_target(
  update_version_h
  COMMAND "${CMAKE_SOURCE_DIR}/scripts/update_version.sh"
  WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
  COMMENT "Updating version.h"
)
add_dependencies(DuetScreen.lib update_version_h)

# Find clang-format-19
include(${LIBRARIES_DIR}/clang-format.cmake)
add_dependencies(DuetScreen.lib clang-format)
  
  # Touch BuildDate.cpp to ensure it is always compiled
  if(CMAKE_BUILD_TYPE STREQUAL "Release")
  add_custom_command(
    TARGET DuetScreen
    PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E touch ${SRC_DIR}/BuildDate.cpp
    COMMENT "Touching BuildDate.cpp to force recompilation")
    endif()
    
target_include_directories(DuetScreen.lib PUBLIC ${INCLUDE_DIR})
target_include_directories(DuetScreen.lib PUBLIC ${SRC_DIR})
target_include_directories(DuetScreen.lib SYSTEM PUBLIC ${LIBRARIES_DIR})

add_subdirectory(src)

# ----------------------------------------------------------------------------
# Add Definitions
# ----------------------------------------------------------------------------
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  target_compile_definitions(DuetScreen.lib PUBLIC DEBUG=1)
else()
  target_compile_definitions(DuetScreen.lib PUBLIC DEBUG=0)
  set_target_properties(DuetScreen.lib PROPERTIES CMAKE_INTERPROCEDURAL_OPTIMIZATION
                                              TRUE)
endif()

target_compile_definitions(DuetScreen.lib PUBLIC MULTITHREADED)

if(PROCESSOR STREQUAL "T113")
  pkg_check_modules(XKBCOMMON xkbcommon)
  target_compile_definitions(lvgl PUBLIC 
                                  LV_USE_LINUX_FBDEV=1
                                  LV_USE_EVDEV=1
                                  LV_LINUX_FBDEV_RENDER_MODE=LV_DISPLAY_RENDER_MODE_DIRECT
                                  LV_LINUX_FBDEV_BUFFER_COUNT=2)
  if(XKBCOMMON_FOUND)
    message(STATUS "xkbcommon found: enabling XKB support for evdev")
    target_compile_definitions(lvgl PUBLIC LV_EVDEV_XKB=1)
    target_link_libraries(lvgl PUBLIC ${XKBCOMMON_LIBRARIES})
    target_include_directories(lvgl PUBLIC ${XKBCOMMON_INCLUDE_DIRS})
  else()
    message(STATUS "xkbcommon not found: evdev keyboard support will be disabled")
  endif()
                                  # add_compile_definitions(T113=1)
  
  target_compile_definitions(DuetScreen.lib PUBLIC T113=1)
  target_compile_definitions(DuetScreen.lib PUBLIC SIMULATION=0)
elseif(PROCESSOR STREQUAL "Simulation")
  target_compile_definitions(lvgl PUBLIC 
                                  LV_USE_SDL=1
                                  LV_SDL_BUF_COUNT=2
                                  LV_SDL_RENDER_MODE=LV_DISPLAY_RENDER_MODE_DIRECT)
  
  target_compile_definitions(DuetScreen.lib PUBLIC T113=0)
  target_compile_definitions(DuetScreen.lib PUBLIC SIMULATION=1)
endif()

target_compile_options(
  DuetScreen.lib
  PRIVATE -pedantic-errors
          -Wall
          -Wdeprecated
          -Wextra
          -Wconversion
          -Wunused
          -Wdouble-promotion
          -Wempty-body
          -Wformat-security
          -Wpointer-arith
          -Wmultichar
          -Wreturn-type
          -Warray-bounds
          # There are issues with lambdas giving a shadow warning
          # even when they don't capture the thing they are supposedly shadowing
          # -Wshadow
          -Wshift-negative-value
          -Wsizeof-pointer-memaccess
          -Wtype-limits
          -Wundef
          -Wuninitialized
          -Wunreachable-code
          -Wfloat-conversion
          -Wstrict-aliasing
          -Wswitch)

if(NOT APPLE)
  # I don't have a Mac and the compiler seems to generate additional warnings so it seems unfair to make Ian suffer
  # -Wclobbered and -Wmaybe-uninitialized are not supported on macOS
  # -Werror is disabled on macOS to avoid build failures due to warnings
  target_compile_options(
    DuetScreen.lib
    PRIVATE 
            -Werror
            -Wclobbered
            -Wmaybe-uninitialized
            )
endif()

if(PROCESSOR STREQUAL "T113" AND CMAKE_BUILD_TYPE STREQUAL "Debug")
	target_compile_options(DuetScreen.lib PRIVATE -Wno-error=cpp) # required because of fortify_source warnings
endif()

# Set DUETSCREEN_* cache options as compile definitions without the prefix
get_cmake_property(_cache_vars CACHE_VARIABLES)
foreach(_var ${_cache_vars})
  if(_var MATCHES "^DUETSCREEN_")
    string(REPLACE "DUETSCREEN_" "" _short "${_var}")
    # Determine cache entry type to handle BOOL vs STRING/paths
    get_property(_type CACHE "${_var}" PROPERTY TYPE)
    set(_val "${${_var}}")

    if(_type STREQUAL "BOOL")
      # Normalize ON/OFF to 1/0
      string(TOUPPER "${_val}" _val_upper)
      if(_val_upper STREQUAL "ON" OR _val_upper STREQUAL "TRUE" OR _val_upper STREQUAL "1" OR _val_upper STREQUAL "YES")
        set(_int 1)
      else()
        set(_int 0)
      endif()

      # Mirror to local variable and define for compilation
      set(${_short} ${_int})
      message("Setting DuetScreen compile definition ${_short} to ${_int}")
      target_compile_definitions(DuetScreen.lib PUBLIC ${_short}=${_int})
    else()
      # Treat non-BOOL cache types (STRING, PATH, FILEPATH, etc.) as string values
      set(${_short} "${_val}")
      message("Setting DuetScreen compile definition ${_short} to ${_val}")
      target_compile_definitions(DuetScreen.lib PUBLIC ${_short}=${_val})
    endif()
  endif()
endforeach()

if (!DUETSCREEN_USE_PCH)
  # Helper function to add preprocesor definition of FILE_BASENAME to pass the
  # filename without directory path for debugging use.
  #
  # Note that in header files this is not consistent with __FILE__ and __LINE__
  # since FILE_BASENAME will be the compilation unit source file name (.c/.cpp).
  #
  # Example:
  #
  # define_file_basename_for_sources(my_target)
  #
  # Will add -DFILE_BASENAME="filename" for each source file depended on by
  # my_target, where filename is the name of the file.
  #
  function(define_file_relpath_for_sources targetname)
    get_target_property(source_files "${targetname}" SOURCES)
    foreach(sourcefile ${source_files})
      # Add the __FILE_RELPATH__=relative-path compile definition to the list for each TU.
      # This yields the main source file path for use in logging/macros.
      file(RELATIVE_PATH relpath "${PROJECT_SOURCE_DIR}" "${sourcefile}")
      set_property(
        SOURCE "${sourcefile}"
        APPEND
        PROPERTY COMPILE_DEFINITIONS "__FILE_RELPATH__=\"${relpath}\"")
    endforeach()
  endfunction()
endif()

# Use compiler-provided __BASE_FILE__ which expands to the primary source file.
# Also remap the project source prefix to produce relative paths in macros.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(DuetScreen.lib PRIVATE -fmacro-prefix-map=${PROJECT_SOURCE_DIR}/=)
  target_compile_options(DuetScreen     PRIVATE -fmacro-prefix-map=${PROJECT_SOURCE_DIR}/=)
endif()
target_compile_definitions(DuetScreen.lib PUBLIC __FILE_RELPATH__=__BASE_FILE__)
target_compile_definitions(DuetScreen     PUBLIC __FILE_RELPATH__=__BASE_FILE__)

# define_file_relpath_for_sources(DuetScreen.lib)
# define_file_relpath_for_sources(DuetScreen)

# Define LVGL configuration as a simple include
target_compile_definitions(DuetScreen.lib PUBLIC LV_CONF_INCLUDE_SIMPLE)
target_link_libraries(
  DuetScreen.lib
  PUBLIC lvgl
          ${SDL2_LIBRARIES}
          ${LIBUSB_LIBRARIES} # Link libusb library
          m
          pthread
          hv_static
          nlohmann_json::nlohmann_json
          nameof::nameof
          magic_enum::magic_enum
          spdlog::spdlog
          fmt::fmt
          Tracy::TracyClient)

if(PROCESSOR STREQUAL "T113")
  target_link_libraries(DuetScreen.lib PRIVATE gpiod wpa_client)
endif()

if (ENABLE_COVERAGE)
  target_compile_options(DuetScreen.lib PUBLIC --coverage)
  target_link_options(DuetScreen.lib PUBLIC --coverage)

  find_program(FIND_CMD find REQUIRED)

  add_custom_target(coverage_clean
    COMMAND ${CMAKE_COMMAND} -E echo "Removing existing .gcda files"
    COMMAND ${FIND_CMD} ${CMAKE_BINARY_DIR} -name "*.gcda" -type f -delete
    COMMAND ${FIND_CMD} ${CMAKE_BINARY_DIR} -wholename "*/src/main.cpp.gcno" -type f -delete
    COMMENT "Cleaning coverage data (*.gcda) from ${CMAKE_BINARY_DIR}"
    VERBATIM
  )

  add_dependencies(DuetScreen.lib coverage_clean)
endif()

# Link the executable with the DuetScreen core library.
# Force-load only the themes archive so static theme registration objects are retained
# without whole-archiving the full DuetScreen.lib.
if(APPLE)
  target_link_libraries(DuetScreen PRIVATE DuetScreen.lib DuetScreen.themes)
  target_link_options(DuetScreen PRIVATE "-Wl,-force_load,$<TARGET_FILE:DuetScreen.themes>")
elseif(MSVC)
  target_link_libraries(DuetScreen PRIVATE DuetScreen.lib DuetScreen.themes)
  target_link_options(DuetScreen PRIVATE "/WHOLEARCHIVE:$<TARGET_FILE:DuetScreen.themes>")
else()
  target_link_libraries(DuetScreen PRIVATE
      DuetScreen.lib
      -Wl,--whole-archive DuetScreen.themes -Wl,--no-whole-archive
  )
endif()

# Link Tracy client at the final executable level so its symbols are available
# when resolving references from static archives like lvgl.
target_link_libraries(DuetScreen PRIVATE Tracy::TracyClient)

# Custom target to show binary size (GNU vs macOS compatibility)
if(APPLE)
  add_custom_target(
    binary_size ALL
    DEPENDS DuetScreen
    COMMAND size ${PROJECT_BINARY_DIR}/DuetScreen
    COMMENT "Running size on DuetScreen (macOS)")
else()
  add_custom_target(
    binary_size ALL
    DEPENDS DuetScreen
    COMMAND size -A ${PROJECT_BINARY_DIR}/DuetScreen
    COMMENT "Running size on DuetScreen")
endif()

# Conditionally include and link SDL2_image if LV_USE_DRAW_SDL is enabled
if(CONFIG_LV_USE_DRAW_SDL)
  set(CMAKE_MODULE_PATH ${CMAKE_MODULE_PATH} "${CMAKE_SOURCE_DIR}/cmake")
  find_package(SDL2_image REQUIRED)
  target_include_directories(lvgl SYSTEM PUBLIC ${SDL2_IMAGE_INCLUDE_DIRS})
  target_link_libraries(DuetScreen.lib PRIVATE ${SDL2_IMAGE_LIBRARIES})
endif()

# Conditionally include and link libpng if LV_USE_LIBPNG is enabled
if(CONFIG_LV_USE_LIBPNG)
  find_package(PNG REQUIRED)
  target_include_directories(lvgl SYSTEM PUBLIC ${PNG_INCLUDE_DIRS})
  target_link_libraries(DuetScreen.lib PRIVATE ${PNG_LIBRARIES})
endif()

# Conditionally include and link libjpeg-turbo if LV_USE_LIBJPEG_TURBO is
# enabled
if(CONFIG_LV_USE_LIBJPEG_TURBO)
  find_package(JPEG REQUIRED)
  target_include_directories(lvgl SYSTEM PUBLIC ${JPEG_INCLUDE_DIRS})
  target_link_libraries(DuetScreen.lib PRIVATE ${JPEG_LIBRARIES})
endif()

# Conditionally include and link FFmpeg libraries if LV_USE_FFMPEG is enabled
if(CONFIG_LV_USE_FFMPEG)
  pkg_check_modules(FFMPEG REQUIRED libavformat libavcodec libavutil libswscale)

  if(FFMPEG_LIBRARY_DIRS)
    # Ensure FFmpeg library directory is on the link line (needed on macOS)
    target_link_directories(DuetScreen.lib PUBLIC ${FFMPEG_LIBRARY_DIRS})
  elseif(APPLE)
    target_link_directories(DuetScreen.lib PUBLIC /opt/homebrew/lib)
  endif()

  target_link_libraries(DuetScreen.lib PRIVATE ${FFMPEG_LIBRARIES} z)
endif()

# Conditionally include and link FreeType if LV_USE_FREETYPE is enabled
if(CONFIG_LV_USE_FREETYPE)
  find_package(Freetype REQUIRED)
  target_include_directories(lvgl SYSTEM PUBLIC ${FREETYPE_INCLUDE_DIRS})

  if (FREETYPE_LIBRARY_DIRS)
    # Ensure FreeType library directory is on the link line (needed on macOS)
    target_link_directories(DuetScreen.lib PUBLIC ${FREETYPE_LIBRARY_DIRS})
  endif()
  target_link_libraries(DuetScreen.lib PRIVATE ${FREETYPE_LIBRARIES})
endif()

# Apply additional compile options if the build type is Debug
if(CMAKE_BUILD_TYPE STREQUAL "Debug")
  message(STATUS "Debug mode enabled")

  if(ASAN)
    message(STATUS "AddressSanitizer enabled")

    # Add AddressSanitizer flags
    set(CMAKE_CXX_FLAGS
        "${CMAKE_CXX_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_C_FLAGS
        "${CMAKE_C_FLAGS} -fsanitize=address -fno-omit-frame-pointer")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -fsanitize=address")
    set(CMAKE_SHARED_LINKER_FLAGS
        "${CMAKE_SHARED_LINKER_FLAGS} -fsanitize=address")
  else()
    message(STATUS "AddressSanitizer disabled")
  endif()
endif()

# ----------------------------------------------------------------------------
# Tests
# ----------------------------------------------------------------------------
if(BUILD_TESTING)
  add_subdirectory(tests)
endif()
