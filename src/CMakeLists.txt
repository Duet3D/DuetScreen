target_sources(DuetScreen PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/main.cpp)

target_sources(
  DuetScreen.lib
  PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/BuildDate.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/DeadlockDetector.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Debug.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/LockWrapper.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/Storage.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/DebugCommands.cpp
          ${CMAKE_CURRENT_SOURCE_DIR}/mouse_cursor_icon.c)

add_subdirectory(Comm)
add_subdirectory(Hardware)
add_subdirectory(i18n)
add_subdirectory(image)
add_subdirectory(ObjectModel)
if(DUETSCREEN_DUET_SIMULATOR)
  add_subdirectory(Simulator)
endif()
add_subdirectory(UI)
add_subdirectory(utils)
