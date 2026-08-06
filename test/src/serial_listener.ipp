#include <memory>

SerialFixture::SerialFixture(Catch::IConfig const *config)
    : Catch::EventListenerBase(config), p_impl(std::make_unique<impl>()) {}
SerialFixture::~SerialFixture() = default;
void SerialFixture::testRunStarting(Catch::TestRunInfo const &) {
  p_impl->start();
}
void SerialFixture::testRunEnded(Catch::TestRunStats const &) {
  p_impl->stop();
}
