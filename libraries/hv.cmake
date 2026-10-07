set(BUILD_SHARED OFF CACHE BOOL "Build shared library")
set(BUILD_STATIC ON CACHE BOOL "Build static library")

# UpgradeHelper performs HTTPS requests via libhv, so OpenSSL headers and
# libraries are a hard requirement for this dependency.
find_package(OpenSSL REQUIRED)
set(WITH_OPENSSL ON CACHE BOOL "For HTTPS support" FORCE)

# Version and upstream URL are consumed by cmake/DuetScreenSbom.cmake so the
# generated SBOM reports whatever is actually fetched here.
set(DUETSCREEN_HV_URL https://github.com/ithewei/libhv)
set(DUETSCREEN_HV_GIT_TAG v1.3.4)

FetchContent_Declare(
  hv
  SYSTEM # Mark as system to suppress warnings from this external library
  GIT_REPOSITORY ${DUETSCREEN_HV_URL}
  GIT_TAG ${DUETSCREEN_HV_GIT_TAG}
)
FetchContent_MakeAvailable(hv)
# target_include_directories(DuetScreen.lib PUBLIC ${hv_SOURCE_DIR})