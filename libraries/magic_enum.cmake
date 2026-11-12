# Version and upstream URL are consumed by cmake/DuetScreenSbom.cmake so the
# generated SBOM reports whatever is actually fetched here.
set(DUETSCREEN_MAGIC_ENUM_URL https://github.com/Neargye/magic_enum)
set(DUETSCREEN_MAGIC_ENUM_GIT_TAG v0.9.7)

FetchContent_Declare(
  magic_enum
  SYSTEM # Mark as system to suppress warnings from this external library
  GIT_REPOSITORY ${DUETSCREEN_MAGIC_ENUM_URL}
  GIT_TAG ${DUETSCREEN_MAGIC_ENUM_GIT_TAG}
)
FetchContent_MakeAvailable(magic_enum)