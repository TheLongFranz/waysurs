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
  std::string port_name;
  std::uint32_t baud_rate{9600};
  parity parity{parity::none};
  stop_bits stop_bits{stop_bits::one};
  data_bits data_bits{data_bits::eight};
  flow_control flow_control{flow_control::none};
  std::uint8_t min_bytes{8};
  std::chrono::milliseconds inter_byte_timeout{100};

  bool operator==(const serial_config &) const = default;
};

class serial_port {
  struct impl;
  std::unique_ptr<impl> p_impl;

public:
  [[nodiscard]] serial_port();
  ~serial_port();
  serial_port(serial_port &&) noexcept = default;
  serial_port &operator=(serial_port &&other) noexcept = default;
  serial_port(const serial_port &other) = delete;
  serial_port &operator=(const serial_port &other) = delete;

  [[nodiscard]] auto open(const serial_config &config)
      -> std::expected<void, error>;
  [[nodiscard]] auto close() -> std::expected<void, error>;
  [[nodiscard]] auto read(std::size_t buffer_size)
      -> std::expected<std::vector<std::byte>, error>;
  [[nodiscard]] auto write(std::span<const std::byte> buffer)
      -> std::expected<std::size_t, error>;
  [[nodiscard]] auto write(std::string_view buffer)
      -> std::expected<std::size_t, error>;
};
} // namespace waysurs