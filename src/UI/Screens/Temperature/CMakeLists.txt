target_sources(
  DuetScreen.lib
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/ExtruderPresenter.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/HeaterActions.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/TemperaturePresenter.cpp
)
