target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/FanSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/BoardSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/DirectoriesSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/FileSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/HeatSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/JobSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/MoveSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ResponseSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/SensorSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/SpindleSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/StateSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Subscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ThumbnailSubscribers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/ToolSubscribers.cpp
          MachSubscribers.cpp
)
