cmake_minimum_required(VERSION 3.20)

project(mrowisko LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

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

# Ostrzezenia kompilatora dla obu programow

if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
target_compile_options(mrowisko PRIVATE -Wall -Wextra -Wpedantic)
target_compile_options(ant PRIVATE -Wall -Wextra -Wpedantic)
endif()