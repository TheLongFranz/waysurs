#include <array>
#include <cstddef>
#include <cstdint>
#include <cstdlib>
#include <format>
#include <print>
#include <span>
#include <string_view>
#include <utility>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>
#include <catch2/generators/catch_generators.hpp>

#include <waysurs/waysurs.hpp>
#include "helpers.hpp"
#include "ports_fixture.hpp"

TEST_CASE_METHOD(ports_fixture, "open(): fails with non-standard baud rates", "[serial]") {
  const auto baud_rates = GENERATE(as<std::uint32_t>{}, 0, 42, 451, 123'456'789);
  REQUIRE(!(tx.open({.port_name = get_env("WAYSURS_SERIAL_TX"), .baud_rate = baud_rates})));
}

TEST_CASE("open(): fails with invalid port name", "[serial]") {
  auto port{waysurs::serial_port()};
  REQUIRE(!(port.open(waysurs::serial_config{.port_name = "bob/hoskins"})).has_value());
}

TEST_CASE("open(): fails with empty port name", "[serial]") {
  auto port{waysurs::serial_port()};
  REQUIRE(!(port.open(waysurs::serial_config{.port_name = ""})));
  REQUIRE(!(port.is_open()));
}

TEST_CASE_METHOD(ports_fixture, "open(): succeeds with valid port name", "[serial]") {
  REQUIRE(tx.is_open());
}

TEST_CASE_METHOD(ports_fixture, "open(): open -> close -> open succeeds with the same config") {
  check(tx.close());
  check(tx.open({.port_name = get_env("WAYSURS_SERIAL_TX")}));

  REQUIRE(tx.is_open());
}

TEST_CASE_METHOD(ports_fixture, "open(): open -> open(new config) succeeds") {
  check(tx.open({.port_name = get_env("WAYSURS_SERIAL_RX")}));

  REQUIRE(tx.is_open());
}

TEST_CASE_METHOD(ports_fixture, "is_open(): succeeds when port is closed") {
  REQUIRE(tx.is_open());

  REQUIRE(tx.close().has_value());

  REQUIRE(!(tx.is_open()));
}

TEST_CASE("close(): succeeds when port hasn't been opened", "[serial]") {
  SKIP("implement later");
}

TEST_CASE("close(): succeeds with a previously closed port", "[serial]") {
  SKIP("implement later");
}

TEST_CASE_METHOD(
  ports_fixture, "write(): succeeds writing a message with std::span<std::byte> parameter",
  "[serial]"
) {
  REQUIRE(tx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto write_result_byte_span(tx.write(std::as_bytes(std::span{msg})));
  check(write_result_byte_span);
  REQUIRE(write_result_byte_span == msg.size());
}

TEST_CASE_METHOD(
  ports_fixture, "write(): succeeds writing a message with std::string_view parameter", "[serial]"
) {
  REQUIRE(tx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto write_result_string_view{tx.write(msg)};
  check(write_result_string_view);
  REQUIRE(write_result_string_view == msg.size());
}

TEST_CASE("write(): before opening port fails with error::config") {
  auto port{waysurs::serial_port()};
  REQUIRE(!(port.write("This should fail\r")));
}

TEST_CASE("read(): before opening port fails with error::config") {
  auto port{waysurs::serial_port()};
  REQUIRE(!(port.read(32)));
}

TEST_CASE("read() -> write() binary roundtrip 0x00 -> 0xFF") {
  SKIP("implement later");
}

TEST_CASE_METHOD(
  ports_fixture, "read(buffer_size): succeeds with roundtrip message to virtual tx/rx pair",
  "[serial]"
) {
  REQUIRE(tx.is_open());
  REQUIRE(rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written == msg.size());

  const auto bytes_read{rx.read(msg.size())};
  check(bytes_read);
  REQUIRE(to_string(bytes_read.value()) == msg);
}

TEST_CASE_METHOD(
  ports_fixture,
  "read(buffer): succeeds with roundtrip message to virtual "
  "tx/rx pair",
  "[serial]"
) {
  REQUIRE(tx.is_open());
  REQUIRE(rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written == msg.size());

  std::array<std::byte, msg.size()> buffer{};
  const auto                        bytes_read{rx.read(buffer)};
  check(bytes_read);
  REQUIRE(bytes_read.value() == msg.size());
  REQUIRE(to_string(buffer) == msg);
}

TEST_CASE_METHOD(
  ports_fixture,
  "read(): all config options succeed with roundtrip message to "
  "virtual tx/rx pair",
  "[serial]"
) {
  const auto baud_rates = GENERATE(as<std::uint32_t>{}, 50, 9600, 57600, 230400);

  const auto parities =
    GENERATE(waysurs::parity::even, waysurs::parity::odd, waysurs::parity::none);
  const auto data_bits = GENERATE(
    waysurs::data_bits::five, waysurs::data_bits::six, waysurs::data_bits::seven,
    waysurs::data_bits::eight
  );
  const auto flow_control = GENERATE(
    waysurs::flow_control::none, waysurs::flow_control::hardware, waysurs::flow_control::software
  );
  const auto stop_bits = GENERATE(waysurs::stop_bits::one, waysurs::stop_bits::two);

  check(tx.open({
    .port_name    = get_env("WAYSURS_SERIAL_TX"),
    .baud_rate    = baud_rates,
    .parity       = parities,
    .stop_bits    = stop_bits,
    .data_bits    = data_bits,
    .flow_control = flow_control,
  }));

  check(rx.open({
    .port_name    = get_env("WAYSURS_SERIAL_RX"),
    .baud_rate    = baud_rates,
    .parity       = parities,
    .stop_bits    = stop_bits,
    .data_bits    = data_bits,
    .flow_control = flow_control,
  }));

  REQUIRE(tx.is_open());
  REQUIRE(rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written.value() == msg.size());

  const auto bytes_read{rx.read(6)};
  check(bytes_read);
  REQUIRE(to_string(bytes_read.value()) == msg);
}

TEST_CASE_METHOD(ports_fixture, "move semantics: moved to serial port succeeds to write") {
  auto port_tx = std::move(tx);
  auto port_rx = std::move(rx);

  REQUIRE(port_tx.is_open());
  REQUIRE(port_rx.is_open());

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{port_tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written == msg.size());

  std::array<std::byte, msg.size()> buffer{};
  const auto                        bytes_read{port_rx.read(buffer)};
  check(bytes_read);
  REQUIRE(bytes_read.value() == msg.size());
  REQUIRE(to_string(buffer) == msg);
}

TEST_CASE("README") {
  waysurs::serial_port port;

  const auto result{
    port
      .open(
        waysurs::serial_config{
          .port_name = "/dev/ttyUSB0",
          .baud_rate = 115200,
          .parity    = waysurs::parity::none,
          .stop_bits = waysurs::stop_bits::one,
          .data_bits = waysurs::data_bits::eight
        }
      )
      .and_then([&port] { return port.write("hello world!"); })
      .transform([](std::size_t bytes_written) {
        std::println("{} bytes written successfully", bytes_written);
      })
      .or_else([](const waysurs::error& err) -> std::expected<void, waysurs::error> {
        std::println("{}", err);
        return std::unexpected(err);
      })
  };

  // this test case exists purely as a compile-time check to ensure that the README example is
  // correct
  REQUIRE(!(result.has_value()));
}