file(GLOB src ./subprocess/*.cpp)

find_package(Threads REQUIRED)

add_library(subprocess ${src})
target_include_directories(subprocess PUBLIC
    ./
)

if(MSVC)
    target_compile_options(subprocess PUBLIC -Zc:__cplusplus)
endif()

if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
    target_link_libraries(subprocess PUBLIC stdc++fs)
    if(NOT WIN32)
        target_link_libraries(subprocess PUBLIC dl)
    endif()
endif()

target_link_libraries(subprocess PUBLIC Threads::Threads)