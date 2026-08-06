#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <optional>
#include <span>
#include <string_view>

// Posix headers
#include <fcntl.h>
#include <termios.h>
#include <unistd.h>

/*
 * termios man page
 * https://man7.org/linux/man-pages/man3/termios.3.html
 */

#include <vector>
#include <waysurs/waysurs.hpp>
#include "waysurs/detail/error.hpp"

namespace waysurs {

  struct serial_port::impl {
private:
    [[nodiscard]] static auto standard_baud_rates(const std::uint32_t rate)
      -> std::expected<speed_t, error> {
      switch (rate) {
      case 50:     return B50;
      case 75:     return B75;
      case 110:    return B110;
      case 134:    return B134;
      case 150:    return B150;
      case 200:    return B200;
      case 300:    return B300;
      case 600:    return B600;
      case 1200:   return B1200;
      case 1800:   return B1800;
      case 2400:   return B2400;
      case 4800:   return B4800;
      case 9600:   return B9600;
      case 19200:  return B19200;
      case 38400:  return B38400;
      case 57600:  return B57600;
      case 115200: return B115200;
      case 230400: return B230400;
      }
      return std::unexpected(
        detail::make_error(
          error_type::baud_rate,
          std::format("Baud rate '{}' is not standard and could not be applied", rate)
        )
      );
    }

    [[nodiscard]] static auto apply_baud_rate(termios& tty, std::uint32_t rate)
      -> std::expected<void, error> {
      const auto tmp_rate{standard_baud_rates(rate)};
      if (tmp_rate.has_value()) {
        cfsetispeed(&tty, tmp_rate.value());
        cfsetospeed(&tty, tmp_rate.value());
        return {};
      }
      return std::unexpected(tmp_rate.error());
    }

    [[nodiscard]] static auto ms_to_vtime(std::chrono::milliseconds ms) noexcept -> std::uint8_t {
      using deciseconds = std::chrono::duration<int, std::deci>;
      return std::clamp(std::chrono::ceil<deciseconds>(ms).count(), 0, 255);
    }

    [[nodiscard]] static auto build_termios(const serial_config& config) noexcept
      -> std::expected<termios, error> {
      struct termios tty{};
      cfmakeraw(&tty); // boilerplate ICANON, ECHO, ECHONL, ISIG, IEXTEN, IGNBRK,

      tty.c_cflag |= (CREAD | CLOCAL); // necessary

      tty.c_cflag &= ~CSIZE;
      switch (config.data_bits) {
        using enum waysurs::data_bits;
      case five:  tty.c_cflag |= CS5; break;
      case six:   tty.c_cflag |= CS6; break;
      case seven: tty.c_cflag |= CS7; break;
      case eight: tty.c_cflag |= CS8; break;
      }

      switch (config.parity) {
        using enum waysurs::parity;
      case none: tty.c_cflag &= ~PARENB; break;
      case even:
        tty.c_cflag |= PARENB;
        tty.c_cflag &= ~PARODD;
        break;
      case odd:
        tty.c_cflag |= PARENB;
        tty.c_cflag |= PARODD;
        break;
      }

      switch (config.stop_bits) {
        using enum waysurs::stop_bits;
      case one: tty.c_cflag &= ~CSTOPB; break;
      case two: tty.c_cflag |= CSTOPB; break;
      }

      switch (config.flow_control) {
        using enum waysurs::flow_control;
      case none:
        tty.c_cflag &= ~CRTSCTS;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
      case hardware:
        tty.c_cflag |= CRTSCTS;
        tty.c_iflag &= ~(IXON | IXOFF | IXANY);
        break;
      case software:
        tty.c_cflag &= ~CRTSCTS;
        tty.c_iflag |= (IXON | IXOFF | IXANY);
        break;
      }

      // implement apply_timeout()
      tty.c_cc[VTIME] = ms_to_vtime(config.inter_byte_timeout);

      tty.c_cc[VMIN] = config.min_bytes;

      if (const auto result{apply_baud_rate(tty, config.baud_rate)}; !(result.has_value())) {
        return std::unexpected(result.error());
      }

      return tty;
    }

public:
    ~impl() { const auto _{close()}; }

    [[nodiscard]] auto is_open() const noexcept -> bool { return m_config.has_value(); }

    [[nodiscard]] auto open(const serial_config& config) -> std::expected<void, error> {
      if (is_open()) {
        if (m_config.value() == config) {
          return {};
        }
        if (const auto result = close(); !result.has_value()) {
          return std::unexpected(result.error());
        }
      }

      if (config.port_name.empty()) {
        return std::unexpected(
          detail::make_error(
            error_type::config, "Failed to open port: name empty or port does not exist"
          )
        );
      }

      if (m_port_id = ::open(config.port_name.c_str(), O_RDWR | O_NOCTTY); m_port_id < 0) {
        return std::unexpected(detail::make_error(error_type::open, "Error opening port", errno));
      }

      auto tty{build_termios(config)};
      if (!(tty.has_value())) {
        const auto _{close()};
        return std::unexpected(tty.error());
      }

      if (tcsetattr(m_port_id, TCSANOW, &tty.value()) != 0) {
        const auto result =
          detail::make_error(error_type::open, "OS Error setting port attributes", errno);
        const auto _{close()};
        return std::unexpected(result);
      }
      m_config = config;
      return {};
    }

    auto close() -> std::expected<void, error> {
      if (m_port_id < 0) {
        return {};
      }

      const auto result = ::close(m_port_id);
      m_port_id         = -1;
      m_config          = std::nullopt;
      if (result < 0) {
        return std::unexpected(detail::make_error(error_type::close, "Error closing port", errno));
      }
      return {};
    }

    [[nodiscard]] auto read(std::size_t buffer_size)
      -> std::expected<std::vector<std::byte>, error> {
      if (is_open()) {
        std::vector<std::byte> read_buf(buffer_size);
        const auto             bytes_read = ::read(m_port_id, read_buf.data(), read_buf.size());
        if (bytes_read < 0) {
          return std::unexpected(
            detail::make_error(error_type::read, "Error reading buffer", errno)
          );
        }

        read_buf.resize(bytes_read);
        return read_buf;
      }
      return std::unexpected(
        detail::make_error(error_type::config, "Port has not been configured")
      );
    }

    [[nodiscard]] auto read(std::span<std::byte> buffer) -> std::expected<std::size_t, error> {
      if (m_config.has_value()) {
        const auto bytes_read = ::read(m_port_id, buffer.data(), buffer.size());
        if (bytes_read < 0) {
          return std::unexpected(
            detail::make_error(error_type::read, "Error reading buffer", errno)
          );
        }
        return bytes_read;
      }
      return std::unexpected(
        detail::make_error(error_type::config, "Port has not been configured")
      );
    }

    [[nodiscard]] auto write(std::span<const std::byte> buffer)
      -> std::expected<std::size_t, error> {
      if (is_open()) {
        if (const auto result = ::write(m_port_id, buffer.data(), buffer.size()); result >= 0) {
          return static_cast<std::size_t>(result);
        }
        return std::unexpected(
          detail::make_error(error_type::write, "Error writing to buffer", errno)
        );
      }
      return std::unexpected(
        detail::make_error(error_type::config, "Port has not been configured")
      );
    }

    [[nodiscard]] auto write(const std::string_view buffer) -> std::expected<std::size_t, error> {
      return write(std::as_bytes(std::span{buffer}));
    }

private:
    int                          m_port_id{-1};
    std::optional<serial_config> m_config{std::nullopt};
  };
} // namespace waysurs

#include "../../common/serial_port.ipp"