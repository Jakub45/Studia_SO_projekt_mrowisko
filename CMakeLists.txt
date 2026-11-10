cmake_minimum_required(VERSION 3.20)

project(mrowisko LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

# Wspolny kod
set(COMMON_SOURCES
    src/common/environment.cpp
    src/common/error_handler/error_handler.cpp
)

# Glowny program mrowiska
add_executable(mrowisko
    src/anthill/main_anthill.cpp
    ${COMMON_SOURCES}
)

# Proces krolowej
add_executable(ant_queen
    src/ant_queen/ant_queen.cpp
    ${COMMON_SOURCES}
)

# Program pojedynczej mrowki
add_executable(ant
    src/ant/ant.cpp
    ${COMMON_SOURCES}
)

# Interfejs TUI (ncurses)
add_executable(tui
    src/tui/tui.cpp
    ${COMMON_SOURCES}
)

target_include_directories(tui PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(tui PRIVATE ${CURSES_LIBRARIES})

# Zbudowanie mrowisko buduje tez ant i tui
add_dependencies(mrowisko ant tui)

# Ostrzezenia kompilatora
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    foreach(target mrowisko ant ant_queen tui)
        target_compile_options(${target} PRIVATE -Wall -Wextra -Wpedantic)
    endforeach()
endif()