#include <memory>

serial_listener::serial_listener(Catch::IConfig const* config) :
    Catch::EventListenerBase(config), p_impl(std::make_unique<impl>()) {}
serial_listener::~serial_listener() = default;
void serial_listener::testRunStarting(Catch::TestRunInfo const&) {
  p_impl->start();
}
void serial_listener::testRunEnded(Catch::TestRunStats const&) {
  p_impl->stop();
}
