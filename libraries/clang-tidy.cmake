# Find or bootstrap clang-tidy
option(ENABLE_CLANG_TIDY "Enable clang-tidy static analysis during compilation" OFF)

if(NOT ENABLE_CLANG_TIDY)
  return()
endif()

find_program(CLANG_TIDY
  NAMES clang-tidy
  DOC "Path to clang-tidy"
)

if(NOT CLANG_TIDY)
  message(STATUS "clang-tidy not found in PATH. Looking in Python venv.")
  set(_venv_clang_tidy "${CMAKE_SOURCE_DIR}/env/bin/clang-tidy")
  if(EXISTS "${_venv_clang_tidy}")
    set(CLANG_TIDY "${_venv_clang_tidy}" CACHE FILEPATH "Path to clang-tidy" FORCE)
  endif()
endif()

if(NOT CLANG_TIDY)
  message(WARNING "ENABLE_CLANG_TIDY is ON but clang-tidy was not found. Install it via: pip install clang-tidy")
  return()
endif()

message(STATUS "clang-tidy enabled: ${CLANG_TIDY}")

# Set CXX_CLANG_TIDY on DuetScreen.lib so analysis runs during compilation.
# Extra arguments after the binary path are forwarded to clang-tidy.
set(CMAKE_CXX_CLANG_TIDY
  "${CLANG_TIDY}"
  "--header-filter=src/.*"
  "--extra-arg=-Wno-unknown-warning-option"
)
