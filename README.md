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
          .port_name = "/dev/ttyUSB0",
          .baud_rate = 115200,
          .parity    = waysurs::parity::none,
          .stop_bits = waysurs::stop_bits::one,
          .data_bits = waysurs::data_bits::eight
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

**MacOS**

```bash
brew install llvm@20 ninja cmake
# catch2 & socat are only required for building tests, not consuming the library
brew install catch2 socat
```

**Linux**

```bash
apt install llvm-20 ninja-build cmake
# catch2 & socat are only required for building tests, not consuming the library
apt install catch2 socat
```

Windows **(Currently Unsupported)**

If Windows **were** supported you would run this command

```powershell
choco install llvm --version=20.1.8
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
cmake --preset release -B out/build/release
cmake --build out/build/release
```

## Running Tests

Running tests on MacOS/Linux requires socat, which creates a linked pair of virtual serial ports for each test case. If you are building this library standalone i.e. PROJECT_IS_TOP_LEVEL then tests will always build as they are the only executable target in the library.

```bash
cmake --preset debug -B ... -DBUILD_TESTING=ON && ctest
```

## Todo

1. port enumeration
1. custom baud rates
1. Windows
1. fuzzing
1. f/a sanitizer
1. CI/CD

# License

WAYSURS is licensed under the MIT License
