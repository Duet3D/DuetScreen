target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Communication.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/FileInfo.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/LineFramer.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ModelSync.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Receive.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Replies.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Responses.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Thumbnail.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Usb.cpp)
