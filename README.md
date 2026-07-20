# Why Are You Still Using RS232?

## What is WAYSURS for?

Unfortunately, RS232 is still a common interface on some devices that we have to talk to. 

### usage

```cpp
#include<print>

#include <waysurs/waysurs.hpp>

int main() {
  waysurs::serial_port port;
  if (port.open(waysurs::serial_config{.port_name = "/dev/ttyUSB0",
                                       .baud_rate = 115200,
                                       .parity = none,
                                       .data_bits = 8})) {
    if (const auto bytes_written{port.write("hello world!")}; bytes_written > 0) {
      std::println("{} bytes written successfully");
    }else {
      std::println("{}", bytes_written.error().message);
    }
  }
}
```

### todo
1. add timeout to config
1. profiling
1. fuzzing
1. f/a sanitizer
1. CI/CD
