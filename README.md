# WAYSURS - Why Are You Still Using RS232?

## What is WAYSURS for?



### Prerequisites

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

**Windows (Currently Unsupported)**
```powershell
choco install llvm --version=20.1.8
```

### Installation (CMake)

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

### Building (MacOS / Linux)

Building WAYSURS requires a **C++23** capable compiler. There is a toolchain already defined for **LLVM20** which sets up **clangd/tidy/format** with cmake generated compile_commands.json. If you want to use another C++ compiler you can override the toolchain with your own **CMakeUserPresets.json**.

```bash
git clone https://github.com/TheLongFranz/waysurs.git
cd waysurs
cmake --preset release -B out/build/release
cmake --build out/build/release
```

### Usage

```cpp
#include <print>

#include <waysurs/waysurs.hpp>

int main() {
  waysurs::serial_port port;

  if (const auto port_opened = port.open(
          waysurs::serial_config{.port_name = "/dev/ttyUSB0",
                                 .baud_rate = 115200,
                                 .parity = waysurs::parity::none,
                                 .stop_bits = waysurs::stop_bits::one,
                                 .data_bits = waysurs::data_bits::eight});
      port_opened) {
    if (const auto bytes_written{port.write("hello world!")};
        bytes_written.has_value()) {
      std::println("{} bytes written successfully", bytes_written.value());
    } else {
      std::println("{}", bytes_written.error());
    }
  } else {
    std::println("{}", port_opened.error()); // prints system error if present
  }
}
```

### Todo
1. custom baud rates
1. port enumeration
1. Windows
1. fuzzing
1. f/a sanitizer
1. CI/CD
