#include <array>
#include <expected>
#include <format>
#include <string>
#include <vector>

// IOKit / Core headers
#include <CoreFoundation/CFBase.h>
#include <CoreFoundation/CFDictionary.h>
#include <CoreFoundation/CFString.h>
#include <CoreFoundation/CoreFoundation.h>
#include <IOKit/IOBSD.h>
#include <IOKit/IOKitLib.h>
#include <IOKit/IOTypes.h>
#include <IOKit/serial/IOSerialKeys.h>
#include <IOKit/serial/ioss.h>

// waysurs headers
#include <waysurs/error.hpp>
#include <waysurs/waysurs.hpp>

static constexpr auto string_length{256};

namespace waysurs {
  [[nodiscard, maybe_unused]] auto list_ports() -> std::expected<std::vector<std::string>, error> {
    std::vector<std::string> out;
    auto                     matching_services{io_iterator_t{}};
    auto*                    classes_to_match{IOServiceMatching(kIOSerialBSDServiceValue)};
    auto                     service{io_object_t{}};

    if (!classes_to_match) {
      return std::unexpected(
        detail::make_error(error_type::config, "IOServiceMatching returned nullptr")
      );
    }

    CFDictionarySetValue(classes_to_match, CFSTR(kIOSerialBSDTypeKey), CFSTR(kIOSerialBSDAllTypes));

    if (const auto kern_result{
          IOServiceGetMatchingServices(kIOMainPortDefault, classes_to_match, &matching_services)
        };
        kern_result != 0) {
      return std::unexpected(
        detail::make_error(
          error_type::config, std::format("IOServiceGetMatchingServices returned {}", kern_result)
        )
      );
    }

    while ((service = IOIteratorNext(matching_services)) != 0) {
      const auto* const path_as_string{
        IORegistryEntryCreateCFProperty(service, CFSTR(kIOCalloutDeviceKey), kCFAllocatorDefault, 0)
      };

      if (path_as_string) {
        std::array<char, string_length> buf{};
        if (CFGetTypeID(path_as_string) == CFStringGetTypeID()) {
          const auto result{CFStringGetCString(
            static_cast<CFStringRef>(path_as_string), buf.data(), buf.size(), kCFStringEncodingUTF8
          )};
          if (result != 0U) {
            out.emplace_back(buf.data());
          }
        }
        CFRelease(path_as_string);
      }
      IOObjectRelease(service);
    }
    IOObjectRelease(matching_services);
    return out;
  }
} // namespace waysurs