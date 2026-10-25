cmake_minimum_required(VERSION 3.20)

project(mrowisko LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

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
```