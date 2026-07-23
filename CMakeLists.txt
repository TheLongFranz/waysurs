cmake_minimum_required(VERSION 3.28.1)

project(waysurs)

add_subdirectory(src)

include(CTest)
if(BUILD_TESTING)
    add_subdirectory(test)
endif()
