target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Connection.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/FileInfo.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Gcode.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/LineFramer.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ModelSync.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Replies.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Responses.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Thumbnail.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/UsbMode.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Links/DsfLink.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Links/HttpLink.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Links/HttpRequester.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Links/SerialLink.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Ports/UartPort.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Ports/UsbPort.cpp)

# The simulated Duet's port: only with DUETSCREEN_DUET_SIMULATOR, as src/Simulator
if(DUETSCREEN_DUET_SIMULATOR)
  target_sources(DuetScreen.lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Ports/SimulatorPort.cpp)
endif()
