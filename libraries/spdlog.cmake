# Version and upstream URL are consumed by cmake/DuetScreenSbom.cmake so the
# generated SBOM reports whatever is actually fetched here. Note that fmt tags
# releases without a leading "v".
set(DUETSCREEN_FMT_URL https://github.com/fmtlib/fmt)
set(DUETSCREEN_FMT_GIT_TAG 12.1.0)

FetchContent_Declare(
  fmt
  SYSTEM # Mark as system to suppress warnings from this external library
  GIT_REPOSITORY ${DUETSCREEN_FMT_URL}
  GIT_TAG ${DUETSCREEN_FMT_GIT_TAG}
)
FetchContent_MakeAvailable(fmt)

target_compile_definitions(fmt PUBLIC FMT_USE_EXCEPTIONS=0)

# Force spdlog to use the external fmt provided above
# Must be set before FetchContent_MakeAvailable(spdlog)
set(SPDLOG_FMT_EXTERNAL ON CACHE BOOL "Use external fmt library" FORCE)

# Version and upstream URL are consumed by cmake/DuetScreenSbom.cmake so the
# generated SBOM reports whatever is actually fetched here.
set(DUETSCREEN_SPDLOG_URL https://github.com/gabime/spdlog)
set(DUETSCREEN_SPDLOG_GIT_TAG v1.16.0)

FetchContent_Declare(
  spdlog
  SYSTEM # Mark as system to suppress warnings from this external library
  GIT_REPOSITORY ${DUETSCREEN_SPDLOG_URL}
  GIT_TAG ${DUETSCREEN_SPDLOG_GIT_TAG}
)
FetchContent_MakeAvailable(spdlog)

target_compile_definitions(
    spdlog PRIVATE
    SPDLOG_ACTIVE_LEVEL=SPDLOG_LEVEL_TRACE
)
