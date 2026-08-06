cmake_minimum_required(VERSION 3.28.1)

project(waysurs LANGUAGES CXX)

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    include(CTest)
    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()
endif()
