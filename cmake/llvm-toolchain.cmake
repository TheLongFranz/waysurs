set(LLVM_MAJOR_VERSION 20)

set(EXE "")
if(CMAKE_HOST_SYSTEM_NAME STREQUAL "Windows")
    set(EXE ".exe")
endif()
# try_compile() re-runs this toolchain in a fresh CMake instance and does not
# forward -D compiler settings, because a toolchain is normally what defines
# them. Without this list the auto-detection below runs inside the compiler
# check, which then tests a different compiler than the build uses and FORCEs
# clang-tidy into it -- failing on any machine that has clang but not clang-tidy.
list(APPEND CMAKE_TRY_COMPILE_PLATFORM_VARIABLES
    CMAKE_C_COMPILER
    CMAKE_CXX_COMPILER
)

# ─── Skip if a compiler is already defined (e.g. via CMakeUserPresets.json) ───
# CXX alone is the right condition: this project is CXX-only, so requiring C too
# would leave the guard ineffective wherever only CMAKE_CXX_COMPILER is set.
if(DEFINED CMAKE_CXX_COMPILER)
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
set(CMAKE_C_COMPILER "${LLVM_BIN_DIR}/clang${EXE}" CACHE FILEPATH "C compiler")
set(CMAKE_CXX_COMPILER "${LLVM_BIN_DIR}/clang++${EXE}" CACHE FILEPATH "C++ compiler")

# ─── Binutils ─────────────────────────────────────────────────────────────────
set(CMAKE_AR "${LLVM_BIN_DIR}/llvm-ar${EXE}" CACHE FILEPATH "Archiver")
set(CMAKE_NM "${LLVM_BIN_DIR}/llvm-nm${EXE}" CACHE FILEPATH "Symbol lister")
set(CMAKE_RANLIB "${LLVM_BIN_DIR}/llvm-ranlib${EXE}" CACHE FILEPATH "Archive indexer")

# ld.lld doesn't exist on macOS — use lld via -fuse-ld instead
if(NOT CMAKE_HOST_SYSTEM_NAME STREQUAL "Darwin")
    set(CMAKE_LINKER "${LLVM_BIN_DIR}/ld.lld" CACHE FILEPATH "Linker")
endif()

# ─── Analysis tools ───────────────────────────────────────────────────────────
set(CMAKE_C_CLANG_TIDY "${LLVM_BIN_DIR}/clang-tidy${EXE}" CACHE STRING "clang-tidy (C)" FORCE)
set(CMAKE_CXX_CLANG_TIDY "${LLVM_BIN_DIR}/clang-tidy${EXE}" CACHE STRING "clang-tidy (C++)" FORCE)
set(CMAKE_CLANGD_EXECUTABLE "${LLVM_BIN_DIR}/clangd${EXE}" CACHE STRING "clangd" FORCE)
set(CLANG_FORMAT_EXECUTABLE "${LLVM_BIN_DIR}/clang-format${EXE}" CACHE STRING "clang-format" FORCE)

message(STATUS "  clang:        ${CMAKE_CXX_COMPILER}")
message(STATUS "  clang-tidy:   ${CMAKE_CXX_CLANG_TIDY}")
message(STATUS "  clangd:       ${CMAKE_CLANGD_EXECUTABLE}")
message(STATUS "  clang-format: ${CLANG_FORMAT_EXECUTABLE}")
