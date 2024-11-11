cmake_minimum_required(VERSION 3.29)
project(BigramsAndThreegrams)

set(CMAKE_CXX_STANDARD 20)

# Configura OpenMP per macOS
if(APPLE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Xpreprocessor -fopenmp")
    set(CMAKE_EXE_LINKER_FLAGS "${CMAKE_EXE_LINKER_FLAGS} -L/opt/homebrew/opt/libomp/lib -lomp")
    include_directories("/opt/homebrew/opt/libomp/include")
endif()

add_executable(BigramsAndThreegrams main.cpp)
