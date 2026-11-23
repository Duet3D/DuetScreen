target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Alert.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Directories.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Followers.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Files.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Heightmap.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/PrinterStatus.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/ChangeSet.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/Json.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/Node.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/Patch.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/Path.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/Schema.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tree/Tree.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Utils.cpp)
