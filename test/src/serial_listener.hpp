#pragma once

#include <catch2/reporters/catch_reporter_event_listener.hpp>
#include <catch2/reporters/catch_reporter_registrars.hpp>
#include <memory>

class SerialFixture : public Catch::EventListenerBase {
  struct impl;
  std::unique_ptr<impl> p_impl;

public:
  SerialFixture(Catch::IConfig const *config);
  ~SerialFixture();
  void testRunStarting(Catch::TestRunInfo const &) override;
  void testRunEnded(Catch::TestRunStats const &) override;
};
CATCH_REGISTER_LISTENER(SerialFixture)
