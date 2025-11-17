FetchContent_Declare(
  colorm
  GIT_REPOSITORY https://github.com/AndyEveritt/colorm.git
  GIT_TAG 409b22571ab00d9f6147b5f3970e2415a1ecda25
)
FetchContent_MakeAvailable(colorm)
target_include_directories(DuetScreen.lib PUBLIC ${colorm_SOURCE_DIR})