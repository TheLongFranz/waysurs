#pragma once

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <memory>

class serial_listener : public Catch::EventListenerBase {
  struct impl;
  std::unique_ptr<impl> p_impl;

  public:
  serial_listener(const serial_listener&)            = delete;
  serial_listener(serial_listener&&)                 = delete;
  serial_listener& operator=(const serial_listener&) = delete;
  serial_listener& operator=(serial_listener&&)      = delete;
  explicit serial_listener(Catch::IConfig const* config);
  ~serial_listener() override;
  void testRunStarting(Catch::TestRunInfo const&) override;
  void testRunEnded(Catch::TestRunStats const&) override;
};
CATCH_REGISTER_LISTENER(serial_listener)
