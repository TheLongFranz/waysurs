add_library(waysurs_waysurs STATIC)
add_library(waysurs::waysurs ALIAS waysurs_waysurs)

target_sources(
    waysurs_waysurs
    PUBLIC FILE_SET HEADERS
    BASE_DIRS ${PROJECT_SOURCE_DIR}/include
    FILES ${PROJECT_SOURCE_DIR}/include/waysurs/waysurs.hpp
    ${PROJECT_SOURCE_DIR}/include/waysurs/error.hpp
)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(waysurs_waysurs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/waysurs.cpp)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin")
    target_sources(waysurs_waysurs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/posix/waysurs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/posix/enumerate_macos.cpp)
    target_link_libraries(waysurs_waysurs PRIVATE
        "-framework CoreFoundation"
        "-framework IOKit"
    )
elseif(CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(waysurs_waysurs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/posix/waysurs.cpp
        ${CMAKE_CURRENT_SOURCE_DIR}/platform/posix/enumerate_linux.cpp)
else()
    message(FATAL_ERROR "There is no implementation for this Platform.")
endif()

set_property(TARGET waysurs_waysurs PROPERTY EXPORT_NAME waysurs)

target_compile_features(waysurs_waysurs PUBLIC cxx_std_23)

if(PROJECT_IS_TOP_LEVEL)
    install(TARGETS waysurs_waysurs
        EXPORT waysursTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        FILE_SET HEADERS DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})
endif()
