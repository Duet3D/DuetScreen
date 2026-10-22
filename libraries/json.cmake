# Version and upstream URL are consumed by cmake/DuetScreenSbom.cmake so the
# generated SBOM reports whatever is actually fetched here.
set(DUETSCREEN_NLOHMANN_JSON_URL https://github.com/nlohmann/json)
set(DUETSCREEN_NLOHMANN_JSON_GIT_TAG v3.12.0)

FetchContent_Declare(
  nlohmann_json
  SYSTEM # Mark as system to suppress warnings from this external library
  GIT_REPOSITORY ${DUETSCREEN_NLOHMANN_JSON_URL}
  GIT_TAG ${DUETSCREEN_NLOHMANN_JSON_GIT_TAG}
  CONFIGURE_COMMAND "" BUILD_COMMAND "")

FetchContent_MakeAvailable(nlohmann_json)

set_target_properties(
  nlohmann_json
  PROPERTIES JSON_BuildTests OFF
             JSON_MultipleHeaders OFF # Use single header mode
             JSON_Install OFF # Don't install
             JSON_BuildExamples OFF # Don't build examples
)
