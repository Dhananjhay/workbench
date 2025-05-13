CMAKE_MINIMUM_REQUIRED (VERSION 3.5)
CMAKE_POLICY (VERSION 3.5)

project(cpuinfo)

if(CMAKE_VERSION VERSION_LESS "2.8.12")
    function(add_compile_options)
        add_definitions(${ARGN})
    endfunction(add_compile_options)
endif()

add_compile_options(-std=c99 -Wall -Wextra -Wno-unused-parameter -Wconversion -Wshadow -pedantic)
add_definitions(-DNDEBUG)

add_library(cpuinfo src/cpuinfo.c)

TARGET_LINK_LIBRARIES(cpuinfo ${CARET_QT5_LINK})

