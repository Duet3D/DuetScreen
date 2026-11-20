target_sources(
  DuetScreen.lib
  PRIVATE
  ${CMAKE_CURRENT_SOURCE_DIR}/Reset.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/UartController.cpp
  ${CMAKE_CURRENT_SOURCE_DIR}/Usb.cpp
)
