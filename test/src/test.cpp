#include <cstdlib>
#include <span>
#include <string_view>

#include <catch2/catch_test_macros.hpp>

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

TEST_CASE("open(): fails with invalid port name", "[serial]") {
  auto port{waysurs::serial_port()};
  const auto result{
      port.open(waysurs::serial_config{.port_name = "bob/hoskins"})};
  REQUIRE(!(result.has_value()));
}

TEST_CASE("open(): succeeds with valid port name", "[serial]") {
  const char *port_tx_name(get_env("WAYSURS_SERIAL_TX"));
  auto port{waysurs::serial_port()};
  check(port.open({.port_name = port_tx_name}));
}

TEST_CASE("write to port", "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};

  auto port{waysurs::serial_port()};

  const auto open_result{port.open({.port_name = port_tx_name})};
  check(open_result);

  constexpr std::string_view msg{"hello\r"};

  const auto write_result_byte_span(port.write(std::as_bytes(std::span{msg})));
  check(write_result_byte_span);
  REQUIRE(write_result_byte_span == msg.size());

  const auto write_result_string_view{port.write(msg)};
  check(write_result_string_view);
  REQUIRE(write_result_string_view == msg.size());
}

TEST_CASE("read(): succeeds with roundtrip message to virtual tx/rx pair",
          "[serial]") {
  const char *port_tx_name{get_env("WAYSURS_SERIAL_TX")};
  const char *port_rx_name{get_env("WAYSURS_SERIAL_RX")};

  auto port_tx{waysurs::serial_port()};
  auto port_rx{waysurs::serial_port()};

  auto open_tx_result{port_tx.open({.port_name = port_tx_name})};
  check(open_tx_result);

  auto open_rx_result{port_rx.open({.port_name = port_rx_name})};
  check(open_rx_result);

  constexpr std::string_view msg{"hello\r"};

  const auto bytes_written{port_tx.write(msg)};
  check(bytes_written);
  REQUIRE(bytes_written == msg.size());

  const auto bytes_read{port_rx.read(6)};
  check(bytes_read);
  REQUIRE(to_string(bytes_read.value()) == msg);
}