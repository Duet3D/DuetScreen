# Every ds_* widget XML can use, and the C++ widget behind it, by what it is for. Widgets.cpp registers them.
target_sources(DuetScreen.lib PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/Widgets.cpp)

add_subdirectory(Buttons)
add_subdirectory(Inputs)
add_subdirectory(Display)
add_subdirectory(Layout)
