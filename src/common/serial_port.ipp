#include <expected>
#include <memory>

#include <waysurs/waysurs.hpp>

namespace waysurs {

  serial_port::serial_port() : p_impl(std::make_unique<impl>()) {}
  serial_port::~serial_port()                                 = default;
  serial_port::serial_port(serial_port&&) noexcept            = default;
  serial_port& serial_port::operator=(serial_port&&) noexcept = default;

  auto serial_port::is_open() const noexcept -> bool {
    return p_impl && p_impl->is_open();
  }

  auto serial_port::flush() -> std::expected<void, error> {
    return p_impl->flush();
  }

  auto serial_port::open(const serial_config& config) -> std::expected<void, error> {
    return p_impl->open(config);
  }

  auto serial_port::close() -> std::expected<void, error> {
    return p_impl->close();
  }

  auto serial_port::read(std::size_t buffer_size) -> std::expected<std::vector<std::byte>, error> {
    return p_impl->read(buffer_size);
  }

  auto serial_port::read(std::span<std::byte> buffer) -> std::expected<std::size_t, error> {
    return p_impl->read(buffer);
  }

  auto serial_port::write(std::span<const std::byte> buffer) -> std::expected<std::size_t, error> {
    return p_impl->write(buffer);
  }
  auto serial_port::write(const std::string_view buffer) -> std::expected<std::size_t, error> {
    return p_impl->write(buffer);
  }
} // namespace waysurs