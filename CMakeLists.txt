cmake_minimum_required(VERSION 3.16)
project(cove LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

file(GLOB_RECURSE COVE_SOURCES
    CONFIGURE_DEPENDS
    "src/*.cpp"
)

if(EMSCRIPTEN)
    list(REMOVE_ITEM COVE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/src/entry.cpp")
    list(APPEND COVE_SOURCES "${CMAKE_CURRENT_SOURCE_DIR}/web/cove.cpp")
endif()

add_executable(cove ${COVE_SOURCES})

target_include_directories(cove PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
)

target_compile_features(cove PRIVATE cxx_std_17)

if(EMSCRIPTEN)
    target_link_options(cove PRIVATE
        --bind
        "-sMODULARIZE=1"
        "-sEXPORT_NAME=createCoveModule"
        "-sALLOW_MEMORY_GROWTH=1"
    )
    set_target_properties(cove PROPERTIES
        SUFFIX ".js"
        RUNTIME_OUTPUT_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/web"
        RUNTIME_OUTPUT_DIRECTORY_RELEASE "${CMAKE_CURRENT_SOURCE_DIR}/web"
    )
endif()