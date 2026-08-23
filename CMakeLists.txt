cmake_minimum_required(VERSION 3.28.1)

project(waysurs VERSION 0.1.0 LANGUAGES CXX)

include(GNUInstallDirs)

if(PROJECT_IS_TOP_LEVEL)
    # setting export compile commands here globally so that clang-tidy checks are ignored in imported builds
    set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
endif()

add_subdirectory(src)

if(PROJECT_IS_TOP_LEVEL)
    include(CMakePackageConfigHelpers)
    include(CTest)

    if(BUILD_TESTING)
        add_subdirectory(test)
    endif()

    configure_package_config_file(
        ${CMAKE_CURRENT_SOURCE_DIR}/cmake/waysursConfig.cmake.in
        ${CMAKE_CURRENT_BINARY_DIR}/waysursConfig.cmake
        INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/waysurs)

    write_basic_package_version_file(
        ${CMAKE_CURRENT_BINARY_DIR}/waysursConfigVersion.cmake
        VERSION ${PROJECT_VERSION}
        COMPATIBILITY SameMajorVersion)
    install(
        EXPORT waysursTargets
        NAMESPACE waysurs::
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/waysurs)

    install(
        FILES ${CMAKE_CURRENT_BINARY_DIR}/waysursConfig.cmake ${CMAKE_CURRENT_BINARY_DIR}/waysursConfigVersion.cmake
        DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/waysurs)
endif()
