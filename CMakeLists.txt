cmake_minimum_required(VERSION 3.20)

project(mrowisko LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

find_package(Curses REQUIRED)

# Glowny program mrowiska
add_executable(mrowisko
    src/main_anthill.cpp
    src/ant_queen.cpp
    src/environment.cpp
)

# Program pojedynczej mrowki
add_executable(ant
    src/ant.cpp
    src/environment.cpp
)

# Interfejs TUI (ncurses)
add_executable(tui
    src/tui.cpp
)
target_include_directories(tui PRIVATE ${CURSES_INCLUDE_DIRS})
target_link_libraries(tui PRIVATE ${CURSES_LIBRARIES})

# Zbudowanie mrowisko buduje tez ant i tui
add_dependencies(mrowisko ant tui)

# Ostrzezenia kompilatora
if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(mrowisko PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(ant PRIVATE -Wall -Wextra -Wpedantic)
    target_compile_options(tui PRIVATE -Wall -Wextra -Wpedantic)
endif()