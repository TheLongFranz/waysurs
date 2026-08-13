#pragma once

#include <cerrno>
#include <cstdint>
#include <format>
#include <optional>
#include <string>
#include <string_view>
#include <system_error>

namespace waysurs {
  enum class error_type : std::uint8_t { config, open, close, read, write, flush, baud_rate };

  struct error {
    error_type  type;
    std::string message;
    /// @note defaulted so aggregate initialisation without a system code does not trip
    /// -Wmissing-field-initializers on GCC or -Wmissing-designated-field-initializers on Clang
    std::optional<std::error_code> system_code{std::nullopt};
  };
  namespace detail {

    [[nodiscard]] inline auto make_error(const error_type type, const std::string_view message)
      -> error {
      return {.type = type, .message = std::string{message}};
    }
    [[nodiscard]] inline auto
    make_error(const error_type type, const std::string_view message, const int system_code)
      -> error {
      return {
        .type        = type,
        .message     = std::string{message},
        .system_code = std::error_code{system_code, std::generic_category()}
      };
    }
  } // namespace detail
} // namespace waysurs

template<>
struct std::formatter<waysurs::error> : std::formatter<std::string_view> {
  auto format(const waysurs::error& err, auto& ctx) const {
    if (err.system_code) {
      return std::format_to(ctx.out(), "{}: {}", err.message, err.system_code->message());
    }
    return std::format_to(ctx.out(), "{}", err.message);
  }
};