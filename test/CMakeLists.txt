add_executable(tests src/test.cpp)

if(WIN32)
    target_sources(tests PRIVATE platform/windows/serial_fixture.cpp)
elseif(APPLE OR UNIX)
    target_sources(tests PRIVATE platform/posix/serial_fixture.cpp)
endif()

find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)

    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.9.1
    )
    FetchContent_MakeAvailable(Catch2)
endif()
include(Catch)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(tests PRIVATE waysurs::waysurs)
catch_discover_tests(tests)
