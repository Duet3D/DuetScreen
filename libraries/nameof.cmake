# Version and upstream URL are consumed by cmake/DuetScreenSbom.cmake so the
# generated SBOM reports whatever is actually fetched here.
set(DUETSCREEN_NAMEOF_URL https://github.com/Neargye/nameof)
set(DUETSCREEN_NAMEOF_GIT_TAG v0.10.4)

FetchContent_Declare(
  nameof
  SYSTEM # Mark as system to suppress warnings from this external library
  GIT_REPOSITORY ${DUETSCREEN_NAMEOF_URL}
  GIT_TAG ${DUETSCREEN_NAMEOF_GIT_TAG}
)
FetchContent_MakeAvailable(nameof)