#pragma once

#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <memory>
#include <span>
#include <string>
#include <vector>

#include "detail/error.hpp"

namespace waysurs {
enum class parity : std::uint8_t { none, odd, even };
enum class stop_bits : std::uint8_t { one, two };
enum class data_bits : std::uint8_t { five, six, seven, eight };
enum class flow_control : std::uint8_t { none, software, hardware };

struct serial_config {
  std::string port_name; ///< OS device path i.e. "/dev/ttyUSB0" or "COM1"
  /// @param baud_rate - std::uint32_t validated and mapped to Standard OS Baud
  /// Rates
  /// 50, 300, 600, 1200, 2400, 4800, 9600, 19200, 38400, 57600, 115200, 230400
  /// Custom baud rates not yet implemented.
  std::uint32_t baud_rate{9600};
  parity parity{parity::none};
  stop_bits stop_bits{stop_bits::one};
  data_bits data_bits{data_bits::eight};
  flow_control flow_control{flow_control::none};
  std::uint8_t min_bytes{8}; ///< Number of bytes allowed in the buffer before
                             ///< read() blocks and returns
  std::chrono::milliseconds inter_byte_timeout{
      100}; ///< Timeout between reading bytes

  bool operator==(const serial_config &) const = default;
};

class serial_port {
  struct impl;
  std::unique_ptr<impl> p_impl;

public:
  [[nodiscard]] serial_port();
  /// @note destructor calls close()
  /// @see close()
  ~serial_port();
  serial_port(serial_port &&) noexcept = default;
  serial_port &operator=(serial_port &&other) noexcept = default;

  /// @param other - const reference to another serial_port class
  /// @note copy-constructor deleted as only one handle to a hardware
  /// port can exist at once. When C++26 is feature complete I'll
  /// replace this with delete("reason")
  serial_port(const serial_port &other) = delete;

  /// @param other - const reference to another serial_port class
  /// @note copy-assignment operator deleted as only one handle to a
  /// hardware port can exist at once. When C++26 is feature
  /// complete I'll replace this with delete("reason")
  serial_port &operator=(const serial_port &other) = delete;

  /// @param config - config struct defining port configuration
  /// @note returns a no-op success if a port was never opened or if the config
  /// hasn't changed, void on success, error on failure
  [[nodiscard]] auto open(const serial_config &config)
      -> std::expected<void, error>;

  [[nodiscard]] auto is_open() -> bool;

  /// @note returns a no-op success if a port was never opened, void on
  /// success, error on failure
  [[nodiscard]] auto close() -> std::expected<void, error>;

  /// @param buffer_size - The number of bytes requested from the read buffer
  /// @note this functions allocates a vector of size = buffer_size on every
  /// call, this vector is resized to the number of bytes read. If you want
  /// non-allocating reads use the read(std::span<std::byte>) overload
  /// @returns - a vector of bytes successfully written on success, error on
  /// failure
  [[nodiscard]] auto read(std::size_t buffer_size)
      -> std::expected<std::vector<std::byte>, error>;

  /// @param buffer - The buffer provided by the caller
  /// @returns - number of bytes successfully written on success, error on
  /// failure
  [[nodiscard]] auto read(std::span<std::byte> buffer)
      -> std::expected<std::size_t, error>;

  /// @param buffer - Span of immutable bytes, this is a 1-1 representation of
  /// the data passed into the serial buffer
  /// @returns - number of bytes successfully written on success, error on
  /// failure
  [[nodiscard]] auto write(std::span<const std::byte> buffer)
      -> std::expected<std::size_t, error>;

  /// @param buffer - non-owning string
  /// @returns - number of bytes successfully written on success, error on
  /// failure
  /// @note This function converts the string_view using std::as_bytes and
  /// passes it to the referenced write(/*bytes*/)
  /// @see write(std::span<const std::byte> buffer)
  [[nodiscard]] auto write(std::string_view buffer)
      -> std::expected<std::size_t, error>;
};
} // namespace waysurs