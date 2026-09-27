add_executable(tests src/test.cpp)

if(CMAKE_SYSTEM_NAME STREQUAL "Windows")
    target_sources(tests PRIVATE platform/windows/serial_listener.cpp)
    # MSVC's ucrt headers mark std::getenv deprecated in favour of _dupenv_s;
    # -Werror would otherwise turn that warning fatal for helpers.hpp's use of it.
    target_compile_definitions(tests PRIVATE _CRT_SECURE_NO_WARNINGS)
elseif(CMAKE_SYSTEM_NAME STREQUAL "Darwin" OR CMAKE_SYSTEM_NAME STREQUAL "Linux")
    target_sources(tests PRIVATE platform/posix/serial_listener.cpp)
else()
    message(FATAL_ERROR "There is no implementation for this Platform.")
endif()

find_package(Catch2 3 QUIET)
if(NOT Catch2_FOUND)
    include(FetchContent)

    # SYSTEM marks Catch2's headers as third-party, matching how find_package's
    # imported target behaves. Without it clang-tidy reports its macro internals
    # (static test registration, function size) against our test sources.
    FetchContent_Declare(Catch2
        GIT_REPOSITORY https://github.com/catchorg/Catch2
        GIT_TAG v3.9.1
        SYSTEM
    )
    FetchContent_MakeAvailable(Catch2)
endif()
include(Catch)

target_link_libraries(tests PRIVATE Catch2::Catch2WithMain)
target_link_libraries(tests PRIVATE waysurs::waysurs)
# PRE_TEST defers test enumeration from build time to ctest time. Without it the build runs
# `tests --list-tests`, which fails or hangs whenever the host cannot execute the target -- e.g.
# instrumented builds, or cross-compilation.
catch_discover_tests(tests DISCOVERY_MODE PRE_TEST)
