target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Alert.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Axis.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/BedOrChamber.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Directories.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Fan.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Files.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Heat.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Heightmap.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Job.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/MotionSystem.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/PrinterStatus.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Sensor.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Spindle.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Tool.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Utils.cpp)
