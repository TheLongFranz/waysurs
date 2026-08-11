set(LLVM_MAJOR_VERSION 20)

# ─── Skip if compilers already defined (e.g. via CMakeUserPresets.json) ───────
if(DEFINED CMAKE_C_COMPILER AND DEFINED CMAKE_CXX_COMPILER)
    message(STATUS "Compilers already defined, skipping LLVM toolchain auto-detection")
    return()
endif()

# ─── Resolve LLVM_BIN_DIR per platform ────────────────────────────────────────
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    message(STATUS "Detected platform: macOS (${CMAKE_HOST_SYSTEM_PROCESSOR})")

    if(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "arm64")
        set(_LLVM_HOMEBREW_PREFIX "/opt/homebrew")
    elseif(CMAKE_HOST_SYSTEM_PROCESSOR MATCHES "x86_64")
        set(_LLVM_HOMEBREW_PREFIX "/usr/local")
    else()
        message(WARNING "Unknown macOS architecture: ${CMAKE_HOST_SYSTEM_PROCESSOR}")
    endif()

    if(_LLVM_HOMEBREW_PREFIX)
        # Glob the versioned Cellar dir to handle patch versions (e.g. 20.1.2)
        file(GLOB _LLVM_CELLAR_DIRS
            "${_LLVM_HOMEBREW_PREFIX}/Cellar/llvm@${LLVM_MAJOR_VERSION}/*/bin"
        )
        list(SORT _LLVM_CELLAR_DIRS ORDER DESCENDING)
        list(GET _LLVM_CELLAR_DIRS 0 LLVM_BIN_DIR)
    endif()

elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Linux")
    message(STATUS "Detected platform: Linux")
    set(LLVM_BIN_DIR "/usr/lib/llvm-${LLVM_MAJOR_VERSION}/bin")

elseif(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    message(STATUS "Detected platform: Windows")
    set(LLVM_BIN_DIR "C:/Program Files/LLVM/bin")

else()
    message(WARNING "Unsupported platform: ${CMAKE_HOST_SYSTEM_NAME}. Set CMAKE_C_COMPILER and CMAKE_CXX_COMPILER manually.")
endif()

# ─── Validate ─────────────────────────────────────────────────────────────────
if(NOT LLVM_BIN_DIR)
    message(FATAL_ERROR "Could not determine LLVM bin directory. Set LLVM_ROOT to override.")
endif()

# Allow full override via LLVM_ROOT cache variable
if(DEFINED LLVM_ROOT)
    set(LLVM_BIN_DIR "${LLVM_ROOT}/bin")
    message(STATUS "LLVM_ROOT override: ${LLVM_ROOT}")
endif()

if(NOT EXISTS "${LLVM_BIN_DIR}")
    message(FATAL_ERROR
        "LLVM ${LLVM_MAJOR_VERSION} bin directory not found: ${LLVM_BIN_DIR}\n"
        "Install LLVM ${LLVM_MAJOR_VERSION} or set LLVM_ROOT to its install prefix."
    )
endif()

message(STATUS "Using LLVM ${LLVM_MAJOR_VERSION} tools from: ${LLVM_BIN_DIR}")

# ─── Compilers ────────────────────────────────────────────────────────────────
set(CMAKE_C_COMPILER "${LLVM_BIN_DIR}/clang" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${LLVM_BIN_DIR}/clang++" CACHE FILEPATH "C++ compiler")

# ─── Binutils ─────────────────────────────────────────────────────────────────
set(CMAKE_AR "${LLVM_BIN_DIR}/llvm-ar" CACHE FILEPATH "Archiver")
set(CMAKE_NM "${LLVM_BIN_DIR}/llvm-nm" CACHE FILEPATH "Symbol lister")
set(CMAKE_RANLIB "${LLVM_BIN_DIR}/llvm-ranlib" CACHE FILEPATH "Archive indexer")

# ─── Standard library ──────────────────────────────────────────────────────────
# Clang is defaulting to an older GCC stdlib on Linux, this forces libc++.
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_CXX_FLAGS_INIT "${CMAKE_CXX_FLAGS_INIT} -stdlib=libc++")
    set(CMAKE_EXE_LINKER_FLAGS_INIT "${CMAKE_EXE_LINKER_FLAGS_INIT} -stdlib=libc++")
    set(CMAKE_SHARED_LINKER_FLAGS_INIT "${CMAKE_SHARED_LINKER_FLAGS_INIT} -stdlib=libc++")
endif()

# ld.lld doesn't exist on macOS — use lld via -fuse-ld instead
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_LINKER "${LLVM_BIN_DIR}/ld.lld" CACHE FILEPATH "Linker")
endif()

# ─── Analysis tools ───────────────────────────────────────────────────────────
set(CMAKE_C_CLANG_TIDY "${LLVM_BIN_DIR}/clang-tidy" CACHE STRING "clang-tidy (C)" FORCE)
set(CMAKE_CXX_CLANG_TIDY "${LLVM_BIN_DIR}/clang-tidy" CACHE STRING "clang-tidy (C++)" FORCE)
set(CMAKE_CLANGD_EXECUTABLE "${LLVM_BIN_DIR}/clangd" CACHE STRING "clangd" FORCE)
set(CLANG_FORMAT_EXECUTABLE "${LLVM_BIN_DIR}/clang-format" CACHE STRING "clang-format" FORCE)

message(STATUS "  clang:        ${CMAKE_CXX_COMPILER}")
message(STATUS "  clang-tidy:   ${CMAKE_CXX_CLANG_TIDY}")
message(STATUS "  clangd:       ${CMAKE_CLANGD_EXECUTABLE}")
message(STATUS "  clang-format: ${CLANG_FORMAT_EXECUTABLE}")
