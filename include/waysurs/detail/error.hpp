#pragma once

#include <cerrno>
#include <cstdint>
#include <format>
#include <optional>
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
  baud_rate,
  validation
};

struct error {
  error_type type;
  std::optional<std::error_code> system_code;
  std::string message;
};

[[nodiscard]] inline auto make_error(const error_type type,
                                     const std::string_view message) -> error {
  switch (type) {
    using enum waysurs::error_type;
  case open:
    [[fallthrough]];
  case close:
    [[fallthrough]];
  case read:
    [[fallthrough]];
  case write:
    return {.type = type,
            .system_code = std::error_code{errno, std::generic_category()},
            .message = std::string{message}};
  default:
    break;
  }
  return {.type = type, .message = std::string{message}};
}
} // namespace waysurs

template <>
struct std::formatter<waysurs::error> : std::formatter<std::string_view> {
  auto format(const waysurs::error &err, auto &ctx) const {
    if (err.system_code) {
      return std::format_to(ctx.out(), "{}: {}", err.message,
                            err.system_code->message());
    }
    return std::format_to(ctx.out(), "{}", err.message);
  }
};