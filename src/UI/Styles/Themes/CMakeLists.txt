# Core theme types remain in DuetScreen.lib for normal symbol usage.
target_sources(
  DuetScreen.lib
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/CustomTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/DefaultTheme.cpp
)

# Concrete themes are placed in a dedicated archive so we can whole-archive/
# force-load only theme registration objects (not the full DuetScreen.lib).
add_library(DuetScreen.themes STATIC)

target_sources(
  DuetScreen.themes
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/DuetScreenTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/FlatTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/GreyScaleTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/IndustrialTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/SoftTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/NeonTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/RetroTheme.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/ThreeDTheme.cpp
)

target_link_libraries(DuetScreen.themes PRIVATE DuetScreen.lib)
