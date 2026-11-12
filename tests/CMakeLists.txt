# Unit tests for DuetScreen using GoogleTest

# Find GoogleTest
# On macOS with GNU toolchain, Homebrew's prebuilt gtest is commonly built
# with clang/libc++, which is ABI-incompatible with libstdc++.
# Build gtest from source in this case so it matches the active compiler.
if(APPLE AND CMAKE_CXX_COMPILER_ID STREQUAL "GNU")
  include(FetchContent)
  FetchContent_Declare(
    googletest
    URL https://github.com/google/googletest/archive/refs/tags/v1.15.2.zip
    DOWNLOAD_EXTRACT_TIMESTAMP ON
  )
  set(INSTALL_GTEST OFF CACHE BOOL "" FORCE)
  FetchContent_MakeAvailable(googletest)
else()
  find_package(GTest REQUIRED)
endif()

enable_testing()

add_executable(DuetScreen.tests)

# Add clang-format custom target
add_custom_target(
  clang-format-tests
  COMMAND find tests/ -iname '*.h' -o -iname '*.cpp'
  -print | xargs ${CLANG_FORMAT_19} -i
  WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
  COMMENT "Running clang-format-19-tests")

if(TARGET clang_format_bootstrap)
  add_dependencies(clang-format-tests clang_format_bootstrap)
endif()
add_dependencies(DuetScreen.tests clang-format-tests)

file(GLOB_RECURSE DUETSCREEN_TEST_SOURCES ${CMAKE_CURRENT_SOURCE_DIR}/src/*.cpp)
target_sources(DuetScreen.tests PRIVATE ${DUETSCREEN_TEST_SOURCES})

if (DUETSCREEN_USE_PCH)
  target_precompile_headers(DuetScreen.tests REUSE_FROM DuetScreen.lib)
endif()

target_include_directories(DuetScreen.tests PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_definitions(lvgl PUBLIC
    LV_USE_TEST=1
    LV_USE_TEST_SCREENSHOT_COMPARE=1
    LV_BUILD_TEST=1
    REF_IMGS_PATH="./tests/ref_imgs/"
)

# Dependencies
target_link_libraries(DuetScreen.tests PRIVATE
    GTest::gtest_main
)

# Link tests with the core library.
target_link_libraries(DuetScreen.tests PRIVATE DuetScreen.lib)

# Use compiler-provided __BASE_FILE__ which expands to the primary source file.
# Also remap the project source prefix to produce relative paths in macros.
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang|AppleClang")
  target_compile_options(DuetScreen.tests PRIVATE -fmacro-prefix-map=${PROJECT_SOURCE_DIR}/=)
endif()
target_compile_definitions(DuetScreen.tests PUBLIC __FILE_RELPATH__=__BASE_FILE__)

include(GoogleTest)
# Ensure tests run from the project root so any relative paths in tests behave as expected
gtest_discover_tests(DuetScreen.tests
    WORKING_DIRECTORY ${CMAKE_SOURCE_DIR}
    PROPERTIES TIMEOUT 30
)
