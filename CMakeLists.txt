cmake_minimum_required(VERSION 3.16)

project(cove LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

add_executable(cove
    src/entry.cpp
)

target_include_directories(cove PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src/include
)

target_compile_features(cove PRIVATE cxx_std_17)
