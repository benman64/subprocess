configure_file(test_config.h.in include/test_config.h)

set(CXXTEST_PATH "${CMAKE_CURRENT_LIST_DIR}/dependencies/cxxtest")
include(${CXXTEST_PATH}/build_tools/cmake/FindCxxTest.cmake)

if (NOT "${CXXTEST_INCLUDES}")
    set(CXXTEST_INCLUDES ${CXXTEST_PATH})
endif()
function (basic_setup name)
    target_include_directories(${name} PUBLIC
        ${CMAKE_CURRENT_BINARY_DIR}/include
        ${CXXTEST_INCLUDES}
    )
    target_link_libraries(${name} PUBLIC subprocess)

    if ("${CMAKE_CXX_COMPILER_ID}" STREQUAL "GNU")
        if (NOT WIN32)
            target_link_libraries(${name} PUBLIC pthread dl)
        endif()
        target_link_libraries(${name} PUBLIC pthread stdc++fs)
    endif()

endfunction()


function(cxx_test source)
    get_filename_component(CPP_FILE_NAME ${source} NAME)
    string(REGEX REPLACE "h$|hpp$" "cpp" CPP_FILE_NAME ${CPP_FILE_NAME})
    string(REGEX REPLACE "\\.cpp$" "" target "${CPP_FILE_NAME}")

    message("${target} => ${CPP_FILE_NAME}")
    set(CPP_FULL_NAME "${CMAKE_CURRENT_BINARY_DIR}/${CPP_FILE_NAME}")
    add_custom_command(
        OUTPUT "${CPP_FULL_NAME}"
        COMMAND ${CXXTESTGEN} --runner=ErrorPrinter
            --output "${CPP_FULL_NAME}" "${source}"
            --main=cxxtest_main
        DEPENDS "${source}"
    )
    add_executable(${target} ${CPP_FULL_NAME})
    basic_setup(${target})

    if(NOT RUNTIME_OUTPUT_DIRECTORY)
        set(RUNTIME_OUTPUT_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR})
    endif()
    message(STATUS "runtime = ${RUNTIME_OUTPUT_DIRECTORY}")
    # this is only a gcc option
    #set_target_properties(${target} PROPERTIES COMPILE_FLAGS "-Wno-effc++")
    if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux" AND MINGW)
        add_test(NAME ${target} COMMAND wine ${RUNTIME_OUTPUT_DIRECTORY}/${target}.exe)
    else()
        add_test(NAME ${target} COMMAND ${RUNTIME_OUTPUT_DIRECTORY}/${target})
    endif()
    #add_custom_command(TARGET test_all
    #    COMMAND $<TARGET_FILE:${target}>
    #)
endfunction(cxx_test)

cxx_test(${CMAKE_CURRENT_LIST_DIR}/basic_test.cpp)

link_libraries(subprocess)

add_executable(cat ./cat_child.cpp)
add_executable(echo ./echo_main.cpp)
add_executable(sleep ./sleep_main.cpp)
add_executable(printenv ./printenv_main.cpp)

add_executable(examples ./examples.cpp)


if(MINGW)
    set(MINGW_DLLS
        libgcc_s_seh-1.dll
        libstdc++-6.dll
    )

    file(COPY "/usr/x86_64-w64-mingw32/lib/libwinpthread-1.dll"
        DESTINATION ./
    )

    foreach(dll ${MINGW_DLLS})
        file(COPY "/usr/lib/gcc/x86_64-w64-mingw32/13-win32/${dll}"
            DESTINATION ./
        )
    endforeach()
endif()