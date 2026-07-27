#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <print>
#include <span>
#include <string>
#include <string_view>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <waysurs/waysurs.hpp>

namespace {
auto check(const auto &var) -> void {
  if (!(var.has_value())) {
    INFO(std::format("{}", var.error()));
  }
  REQUIRE(var.has_value());
}

auto get_env(const char *env) -> const char * {
  const char *result = std::getenv(env);
  if (result == nullptr) {
    INFO(std::format("Environment variable {} does not exist", env));
  }
  REQUIRE(result != nullptr);
  return result;
}

auto to_string(const auto &vec) -> std::string {
  return std::string(reinterpret_cast<const char *>(vec.data()), vec.size());
}
} // namespace

TEST_CASE("open(): fails with non-standard baud rates", "[serial]") {
  const char *port_tx_name(get_env("WAYSURS_SERIAL_TX"));
  auto port{waysurs::serial_port()};

  const auto baud_rates =
      GENERATE(as<std::uint32_t>{}, 0, 42, 451, 123'456'789);
  REQUIRE(!(port.open({.port_name = port_tx_name, .baud_rate = baud_rates})));
}

TEST_CASE("open(): fails with invalid port name", "[serial]") {
  auto port{waysurs::serial_port()};
  REQUIRE(!(port.open(waysurs::serial_config{.port_name = "bob/hoskins"}))
               .has_value());
}

TEST_CASE("open(): fails with empty port name", "[serial]") {
  auto port{waysurs::serial_port()};
  REQUIRE(!(port.open(waysurs::serial_config{.port_name = ""})));
  REQUIRE(!(port.is_open()));
}

TEST_CASE("open(): succeeds with valid port name", "[serial]") {
  const char *port_tx_name(get_env("WAYSURS_SERIAL_TX"));
  auto port{waysurs::serial_port()};
  check(port.open({.port_name = port_tx_name}));
  REQUIRE(port.is_open());
}

TEST_CASE(
    "write(): succeeds writing a message with std::span<std::byte> parameter",
    "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};

  auto port{waysurs::serial_port()};

  check(port.open({.port_name = port_tx_name}));

  REQUIRE(port.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto write_result_byte_span(port.write(std::as_bytes(std::span{msg})));
  check(write_result_byte_span);
  REQUIRE(write_result_byte_span == msg.size());
}

TEST_CASE("write(): succeeds writing a message with std::string_view parameter",
          "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};

  auto port{waysurs::serial_port()};

  check(port.open({.port_name = port_tx_name}));

  REQUIRE(port.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto write_result_string_view{port.write(msg)};
  check(write_result_string_view);
  REQUIRE(write_result_string_view == msg.size());
}

TEST_CASE("read(): before opening port fails with error::config") {}

TEST_CASE("write(): before opening port fails with error::config") {}

TEST_CASE("read -> write binary roundtrip") {}

TEST_CASE("open(): open -> close -> open succeeds with the same config") {}

TEST_CASE("open(): open -> open(new config) succeeds") {}

TEST_CASE("move semantics: moved from ") {}

TEST_CASE(
    "read(buffer_size): succeeds with roundtrip message to virtual tx/rx pair",
    "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};
  const char *port_rx_name{get_env("WAYSURS_SERIAL_RX")};

  auto port_tx{waysurs::serial_port()};
  auto port_rx{waysurs::serial_port()};

  check(port_tx.open({.port_name = port_tx_name}));
  check(port_rx.open({.port_name = port_rx_name}));

  REQUIRE(port_tx.is_open());
  REQUIRE(port_rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{port_tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written == msg.size());

  const auto bytes_read{port_rx.read(msg.size())};
  check(bytes_read);
  REQUIRE(to_string(bytes_read.value()) == msg);
}

TEST_CASE("read(buffer): succeeds with roundtrip message to virtual "
          "tx/rx pair",
          "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};
  const char *port_rx_name{get_env("WAYSURS_SERIAL_RX")};

  auto port_tx{waysurs::serial_port()};
  auto port_rx{waysurs::serial_port()};

  check(port_tx.open({.port_name = port_tx_name}));
  check(port_rx.open({.port_name = port_rx_name}));

  REQUIRE(port_tx.is_open());
  REQUIRE(port_rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{port_tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written == msg.size());

  std::array<std::byte, msg.size()> buffer{};
  const auto bytes_read{port_rx.read(buffer)};
  check(bytes_read);
  REQUIRE(bytes_read.value() == msg.size());
  REQUIRE(to_string(buffer) == msg);
}

TEST_CASE("read(): all config options succeed with roundtrip message to "
          "virtual tx/rx pair",
          "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};
  const char *port_rx_name{get_env("WAYSURS_SERIAL_RX")};

  auto port_tx{waysurs::serial_port()};
  auto port_rx{waysurs::serial_port()};

  const auto baud_rates =
      GENERATE(as<std::uint32_t>{}, 50, 300, 600, 1200, 2400, 4800, 9600, 19200,
               38400, 57600, 115200, 230400);
  const auto parities = GENERATE(waysurs::parity::even, waysurs::parity::odd,
                                 waysurs::parity::none);
  const auto data_bits =
      GENERATE(waysurs::data_bits::five, waysurs::data_bits::six,
               waysurs::data_bits::seven, waysurs::data_bits::eight);
  const auto flow_control =
      GENERATE(waysurs::flow_control::none, waysurs::flow_control::hardware,
               waysurs::flow_control::software);
  const auto stop_bits =
      GENERATE(waysurs::stop_bits::one, waysurs::stop_bits::two);

  check(port_tx.open({.port_name = port_tx_name,
                      .baud_rate = baud_rates,
                      .parity = parities,
                      .stop_bits = stop_bits,
                      .data_bits = data_bits,
                      .flow_control = flow_control}));

  check(port_rx.open({.port_name = port_rx_name,
                      .baud_rate = baud_rates,
                      .parity = parities,
                      .stop_bits = stop_bits,
                      .data_bits = data_bits,
                      .flow_control = flow_control}));

  REQUIRE(port_tx.is_open());
  REQUIRE(port_rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{port_tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written.value() == msg.size());

  const auto bytes_read{port_rx.read(6)};
  check(bytes_read);
  REQUIRE(to_string(bytes_read.value()) == msg);
}

TEST_CASE("README") {
  waysurs::serial_port port;

  const auto result{
      port.open(waysurs::serial_config{.port_name = "/dev/ttyUSB0",
                                       .baud_rate = 115200,
                                       .parity = waysurs::parity::none,
                                       .stop_bits = waysurs::stop_bits::one,
                                       .data_bits = waysurs::data_bits::eight})
          .and_then([&port] { return port.write("hello world!"); })
          .transform([](std::size_t bytes_written) {
            std::println("{} bytes written successfully", bytes_written);
          })
          .or_else([](const waysurs::error &err)
                       -> std::expected<void, waysurs::error> {
            std::println("{}", err);
            return std::unexpected(err);
          })};
}