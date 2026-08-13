[![CI](https://img.shields.io/github/actions/workflow/status/TheLongFranz/waysurs/cmake-multi-platform.yml?style=flat&logo=github&label=CI)](https://github.com/TheLongFranz/waysurs/actions/workflows/cmake-multi-platform.yml)
[![Licence](https://img.shields.io/github/license/TheLongFranz/waysurs?style=flat)](./LICENSE)
![C++23](https://img.shields.io/badge/c++-%2300599C.svg?style=flat&logo=c%2B%2B&logoColor=white)
![Platforms](https://img.shields.io/badge/platforms-Linux%20%7C%20macOS-informational?style=flat)

# WAYSURS - Why Are You Still Using RS232?

## What is WAYSURS for?

If you are unfortunate enough to find yourself needing to communicate with a device using RS232, you might as well do it using this modern little C++ library. WAYSURS is an MIT-Licensed cross-platform-ish modern **C++23** serial port library with std::expected based error handling & PIMPL per-platform backends.

## Usage

```cpp
#include <print>

#include <waysurs/waysurs.hpp>

int main() {
  waysurs::serial_port port;

  const auto result{
    port
      .open(
        waysurs::serial_config{
          .port_name      = "PORT NAME",
          .baud_rate      = 115200,
          .parity_type    = waysurs::parity::none,
          .stop_bits_type = waysurs::stop_bits::one,
          .data_bits_type = waysurs::data_bits::eight
        }
      )
      .and_then([&port] { return port.write("hello world!"); })
      .transform([](std::size_t bytes_written) {
        std::println("{} bytes written successfully", bytes_written);
      })
      .or_else([](const waysurs::error& err) -> std::expected<void, waysurs::error> {
        std::println("{}", err);
        return std::unexpected(err);
      })
  };
}
```

## Prerequisites

Building WAYSURS requires a C++23 capable compiler, below are steps for installing LLVM 20 for reference (because that's what I used).

### MacOS

```bash
brew install llvm@20 ninja cmake
# catch2 & socat are only required for building tests, not consuming the library
brew install catch2 socat
```

### Linux

```bash
wget https://apt.llvm.org/llvm.sh
chmod +x llvm.sh
sudo ./llvm.sh 20
sudo apt install ninja-build cmake
# catch2 & socat are only required for building tests, not consuming the library
sudo apt install catch2 socat
```

### Windows **(Currently Unsupported)**

If Windows **were** supported you would run this command

```powershell
choco install llvm --version=20.1.8
choco install ninja cmake
# Once I have figured out what I will use for testing virtual ports on Windows I will add it here
choco install catch2 {what I will use for testing virtual ports on Windows}
```

## Installation (CMake)

```cmake
cmake_minimum_required(VERSION 3.28.1)

project(something_serial)

add_executable(${PROJECT_NAME} src/main.cpp)

include(FetchContent)

FetchContent_Declare(
    waysurs
    GIT_REPOSITORY https://github.com/TheLongFranz/waysurs.git
    GIT_TAG main
)

FetchContent_MakeAvailable(waysurs)

target_link_libraries(${PROJECT_NAME} PRIVATE waysurs::waysurs)
```

## Building (MacOS / Linux)

Building WAYSURS requires a **C++23** capable compiler. There is a toolchain already defined for **LLVM20** which sets up **clangd/tidy/format** with cmake generated compile_commands.json. If you want to use another C++ compiler you can override the toolchain with your own **CMakeUserPresets.json**.

```bash
git clone https://github.com/TheLongFranz/waysurs.git
cd waysurs
cmake --preset release
cmake --build --preset release
```

Each preset writes to `out/build/<preset>`. The available presets are `debug`, `release`,
`relwithdebinfo`, `minsizerel` and `asan-ubsan`.

To build with a compiler other than LLVM 20, pass both compilers explicitly — the toolchain
skips its own auto-detection when they are already set:

```bash
cmake --preset release -DCMAKE_C_COMPILER=gcc-15 -DCMAKE_CXX_COMPILER=g++-15
```

## Running Tests

Running tests on MacOS/Linux requires socat, which creates a linked pair of virtual serial ports for each test case. If you are building this library standalone i.e. PROJECT_IS_TOP_LEVEL then tests will always build as they are the only executable target in the library.

```bash
ctest --preset release
```

To run the suite under AddressSanitizer and UndefinedBehaviorSanitizer — the same configuration
CI uses, which aborts on the first finding:

```bash
cmake --preset asan-ubsan
cmake --build --preset asan-ubsan
ctest --preset asan-ubsan
```

The preset builds Catch2 from source rather than using an installed copy — linking instrumented
code against an uninstrumented Catch2 makes ASan report spurious container overflows.

> **macOS:** use Apple Clang for this preset (`-DCMAKE_C_COMPILER=clang
> -DCMAKE_CXX_COMPILER=clang++`). Homebrew LLVM 20's ASan runtime deadlocks during its own
> initialisation against current macOS dyld, so any instrumented binary hangs at startup.

## Linting

CI runs both of these as hard gates, and `.clang-tidy` sets `WarningsAsErrors: '*'`. The LLVM 20
toolchain also wires clang-tidy into every local build, so warnings surface as you compile.

```bash
find include src test \( -name '*.hpp' -o -name '*.cpp' -o -name '*.ipp' \) -print0 \
  | xargs -0 clang-format --style=file --dry-run --Werror

cmake --preset release -DCMAKE_CXX_SCAN_FOR_MODULES=OFF
run-clang-tidy -p out/build/release "$(pwd)/(src|test)/"
```

`CMAKE_CXX_SCAN_FOR_MODULES=OFF` keeps Ninja's `@...modmap` arguments out of
`compile_commands.json`; they only exist after a build and clang-tidy treats them as a hard error.
On macOS Homebrew's LLVM is keg-only, so point the tools at it explicitly — for example
`-clang-tidy-binary "$(brew --prefix llvm@20)/bin/clang-tidy"`.

## Todo

1. port enumeration
1. custom baud rates
1. Windows
1. fuzzing
1. f/a sanitizer

# License

WAYSURS is licensed under the MIT License
