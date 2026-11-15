# The simulated Duet: only with DUETSCREEN_DUET_SIMULATOR (see the top-level CMakeLists.txt)
target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Config.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Machine.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/MachineFiles.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/MachineGcode.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/MachineModel.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Simulator.cpp)
