add_library(waysurs_waysurs STATIC)
add_library(waysurs::waysurs ALIAS waysurs_waysurs)

target_include_directories(
    waysurs_waysurs ${warning_guard}
    PUBLIC
    "\$<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/include>"
)

if(WIN32)
    target_sources(waysurs_waysurs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/windows/waysurs.cpp)
elseif(APPLE)
    target_sources(waysurs_waysurs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/macos/waysurs.cpp)
elseif(UNIX)
    target_sources(waysurs_waysurs PRIVATE ${CMAKE_CURRENT_SOURCE_DIR}/platform/linux/waysurs.cpp)
endif()

set_property(TARGET waysurs_waysurs PROPERTY EXPORT_NAME waysurs)
