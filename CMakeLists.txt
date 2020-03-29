cmake_minimum_required(VERSION 3.6)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Release/Debug")
endif()

include(CheckCXXCompilerFlag)

function(enable_cxx_compiler_flag_if_supported flag)
    string(FIND "${CMAKE_CXX_FLAGS}" "${flag}" flag_already_set)
    if(flag_already_set EQUAL -1)
        check_cxx_compiler_flag("${flag}" flag_supported)
        if(flag_supported)
            set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${flag}" PARENT_SCOPE)
        endif()
        unset(flag_supported CACHE)
    endif()
endfunction()

project(subprocess VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

enable_cxx_compiler_flag_if_supported(-pedantic)

if(NOT MSVC)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -std=c++17")
endif()

file(GLOB src_files src/cpp/subprocess/*.cpp)



add_library(subprocess ${src_files})

target_include_directories(subprocess PUBLIC
    src/cpp
)
if(MSVC)
    target_compile_options(subprocess PUBLIC -Zc:__cplusplus)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(subprocess PUBLIC stdc++fs)
    if(NOT WIN32)
        target_link_libraries(subprocess PUBLIC pthread dl)
    endif()
endif()


# TODO testing
enable_testing()
add_subdirectory(test)