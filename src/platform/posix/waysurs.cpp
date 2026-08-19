#include <algorithm>
#include <cerrno>
#include <chrono>
#include <cstddef>
#include <cstdint>
#include <expected>
#include <format>
#include <limits>
#include <optional>
#include <span>
#include <string_view>
#include <vector>

// Posix headers
#include <fcntl.h>
#include <sys/types.h> // ssize_t
#include <termios.h>
#include <unistd.h>

// Library headers
#include <waysurs/error.hpp>
#include <waysurs/waysurs.hpp>

namespace waysurs {

  struct serial_port::impl {
private:
    // The baud rates below are the values users pass in, not constants worth naming.
    // NOLINTBEGIN(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)
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
      default:     break;
      }
      return std::unexpected(
        detail::make_error(
          error_type::baud_rate,
          std::format("Baud rate '{}' is not standard and could not be applied", rate)
        )
      );
    }
    // NOLINTEND(cppcoreguidelines-avoid-magic-numbers,readability-magic-numbers)

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

    /// @note VTIME is a single byte counting deciseconds, so timeouts saturate at 25.5s
    [[nodiscard]] static auto ms_to_vtime(std::chrono::milliseconds ms) noexcept -> std::uint8_t {
      using deciseconds = std::chrono::duration<int, std::deci>;

      constexpr auto ms_per_decisecond = 100;
      constexpr auto max_vtime_ms =
        std::chrono::milliseconds{std::numeric_limits<std::uint8_t>::max() * ms_per_decisecond};

      // clamp before narrowing, so an out-of-range timeout saturates rather than wrapping
      const auto clamped_ms{std::clamp(ms, std::chrono::milliseconds::zero(), max_vtime_ms)};

      return static_cast<std::uint8_t>(std::chrono::ceil<deciseconds>(clamped_ms).count());
    }

    [[nodiscard]] static auto build_termios(const serial_config& config)
      -> std::expected<termios, error> {
      struct termios tty{};
      cfmakeraw(&tty); // boilerplate ICANON, ECHO, ECHONL, ISIG, IEXTEN, IGNBRK,

      tty.c_cflag |= (CREAD | CLOCAL); // necessary

      tty.c_cflag &= ~CSIZE;
      switch (config.data_bits_type) {
        using enum waysurs::data_bits;
      case five:  tty.c_cflag |= CS5; break;
      case six:   tty.c_cflag |= CS6; break;
      case seven: tty.c_cflag |= CS7; break;
      case eight: tty.c_cflag |= CS8; break;
      }

      switch (config.parity_type) {
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

      switch (config.stop_bits_type) {
        using enum waysurs::stop_bits;
      case one: tty.c_cflag &= ~CSTOPB; break;
      case two: tty.c_cflag |= CSTOPB; break;
      }

      switch (config.flow_control_type) {
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

      tty.c_cc[VTIME] = ms_to_vtime(config.inter_byte_timeout);

      tty.c_cc[VMIN] = config.min_bytes;

      if (const auto result{apply_baud_rate(tty, config.baud_rate)}; !(result.has_value())) {
        return std::unexpected(result.error());
      }

      return tty;
    }

    auto discard_close() -> void { [[maybe_unused]] const auto _{close()}; }

    /// @note Re-issues a syscall for as long as a signal interrupts it. errno is only meaningful
    /// once a call has failed, so it is read solely on the negative branch.
    template<typename Fn>
    [[nodiscard]] static auto retry_on_eintr(Fn syscall) -> ssize_t {
      ssize_t result{};
      do {
        result = syscall();
      } while (result < 0 && errno == EINTR);
      return result;
    }

public:
    impl() = default;
    ~impl() { discard_close(); }

    /// @note impl owns a file descriptor and is never copied or moved; serial_port moves the
    /// owning unique_ptr instead
    impl(const impl&)            = delete;
    impl& operator=(const impl&) = delete;
    impl(impl&&)                 = delete;
    impl& operator=(impl&&)      = delete;

    [[nodiscard]] auto is_open() const noexcept -> bool { return m_config.has_value(); }

    [[nodiscard]] auto flush() const -> std::expected<void, error> {
      if (!is_open()) {
        return std::unexpected(
          detail::make_error(error_type::config, "Cannot flush an unopened port")
        );
      }

      if (tcflush(m_port_id, TCIOFLUSH) != 0) {
        return std::unexpected(
          detail::make_error(error_type::flush, "OS Error flushing port buffers", errno)
        );
      }
      return {};
    }

    [[nodiscard]] auto open(const serial_config& config) -> std::expected<void, error> {
      if (m_config.has_value()) {
        if (*m_config == config) {
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

      if (m_port_id = ::open(config.port_name.c_str(), O_RDWR | O_NOCTTY | O_NONBLOCK);
          m_port_id < 0) {
        return std::unexpected(detail::make_error(error_type::open, "Error opening port", errno));
      }

      auto tty{build_termios(config)};
      if (!(tty.has_value())) {
        discard_close();
        return std::unexpected(tty.error());
      }

      // TCSAFLUSH applies the settings once output has drained and discards any input received
      // while the port was still running at the previous settings, i.e. misframed bytes
      if (tcsetattr(m_port_id, TCSAFLUSH, &tty.value()) != 0) {
        const auto result =
          detail::make_error(error_type::open, "OS Error setting port attributes", errno);
        discard_close();
        return std::unexpected(result);
      }

      if (const int flags = fcntl(m_port_id, F_GETFL);
          flags < 0 || fcntl(m_port_id, F_SETFL, flags & ~O_NONBLOCK) < 0) {
        const auto result =
          detail::make_error(error_type::open, "OS Error restoring blocking mode", errno);
        discard_close();
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

    /// @note A read is deliberately single-shot: the port cannot tell us how much the sender
    /// still intends to send, so returning whatever has arrived is the only honest answer. A
    /// caller that needs a fixed number of bytes loops until it has them.
    [[nodiscard]] auto read(std::size_t buffer_size) const
      -> std::expected<std::vector<std::byte>, error> {
      if (!is_open()) {
        return std::unexpected(
          detail::make_error(error_type::config, "Port has not been configured")
        );
      }

      std::vector<std::byte> read_buf(buffer_size);
      const auto             bytes_read{retry_on_eintr([&] {
        return ::read(m_port_id, read_buf.data(), read_buf.size());
      })};
      if (bytes_read < 0) {
        return std::unexpected(detail::make_error(error_type::read, "Error reading buffer", errno));
      }

      read_buf.resize(static_cast<std::size_t>(bytes_read));
      return read_buf;
    }

    /// @see read(std::size_t) for why a short read is not an error
    [[nodiscard]] auto read(std::span<std::byte> buffer) const
      -> std::expected<std::size_t, error> {
      if (!is_open()) {
        return std::unexpected(
          detail::make_error(error_type::config, "Port has not been configured")
        );
      }

      const auto bytes_read{retry_on_eintr([&] {
        return ::read(m_port_id, buffer.data(), buffer.size());
      })};
      if (bytes_read < 0) {
        return std::unexpected(detail::make_error(error_type::read, "Error reading buffer", errno));
      }
      return static_cast<std::size_t>(bytes_read);
    }

    /// @note Unlike read(), a write loops until the whole buffer is handed to the driver. A
    /// single ::write() stops at the driver's buffer limit, so a short count is routine rather
    /// than a failure, and a caller asking to send N bytes means all N.
    /// @warning On a blocking port this waits for the peer to drain. Sending more than the
    /// driver will buffer with nothing reading the other end blocks until something does.
    [[nodiscard]] auto write(std::span<const std::byte> buffer) const
      -> std::expected<std::size_t, error> {
      if (!is_open()) {
        return std::unexpected(
          detail::make_error(error_type::config, "Port has not been configured")
        );
      }

      std::size_t total_written{0};
      while (total_written < buffer.size()) {
        const auto remaining{buffer.subspan(total_written)};
        const auto bytes_written{retry_on_eintr([&] {
          return ::write(m_port_id, remaining.data(), remaining.size());
        })};
        if (bytes_written < 0) {
          return std::unexpected(
            detail::make_error(error_type::write, "Error writing to buffer", errno)
          );
        }
        total_written += static_cast<std::size_t>(bytes_written);
      }
      return total_written;
    }

    [[nodiscard]] auto write(const std::string_view buffer) const
      -> std::expected<std::size_t, error> {
      return write(std::as_bytes(std::span{buffer}));
    }

private:
    int                          m_port_id{-1};
    std::optional<serial_config> m_config{std::nullopt};
  };
} // namespace waysurs

#include "../../common/serial_port.ipp"