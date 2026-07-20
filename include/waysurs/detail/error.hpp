#pragma once

#include <cerrno>
#include <cstdint>
#include <format>
#include <string>
#include <string_view>
#include <system_error>

namespace waysurs {
enum class error_type : std::uint8_t {
  config,
  open,
  close,
  read,
  write,
  out_of_range,
  baud_rate
};

struct error {
  error_type type;
  std::error_code system_code;
  std::string message;
};

[[nodiscard]] inline auto make_error(const error_type type,
                                     const std::string_view message) -> error {
  return {.type = type,
          .system_code = {errno, std::generic_category()},
          .message = std::string{message}};
}
} // namespace waysurs

template <>
struct std::formatter<waysurs::error> : std::formatter<std::string_view> {
  auto format(const waysurs::error &err, auto &ctx) const {
    if (err.system_code) {
      return std::format_to(ctx.out(), "{}: {}", err.message,
                            err.system_code.message());
    }
    return std::format_to(ctx.out(), "{}", err.message);
  }
};