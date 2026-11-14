target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/FileSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ResponseSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Subscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailSubscribers.cpp)
