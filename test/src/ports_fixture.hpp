#pragma once
#include <waysurs/waysurs.hpp>
#include "helpers.hpp"

struct ports_fixture {
  waysurs::serial_port tx;
  waysurs::serial_port rx;

  ports_fixture() {
    check(tx.open({.port_name = get_env("WAYSURS_SERIAL_TX")}));
    check(rx.open({.port_name = get_env("WAYSURS_SERIAL_RX")}));
  }
};