
#include <array>
#include <chrono>
#include <csignal>
#include <cstdlib>
#include <filesystem>
#include <stdexcept>
#include <string>
#include <sys/wait.h>
#include <system_error>
#include <thread>
#include <unistd.h>

#include "../../src/serial_listener.hpp"

namespace fs = std::filesystem;

struct serial_listener::impl {
  static constexpr auto poll_interval   = std::chrono::milliseconds{50};
  static constexpr auto startup_timeout = std::chrono::seconds{5};

  fs::path dir;
  pid_t    socat_pid{-1};

  impl() = default;
  ~impl() { stop(); }

  /// @note impl owns a child process and a temp directory; serial_listener moves the owning
  /// unique_ptr instead
  impl(const impl&)            = delete;
  impl& operator=(const impl&) = delete;
  impl(impl&&)                 = delete;
  impl& operator=(impl&&)      = delete;

  void start() {
    // mkdtemp rewrites the trailing XXXXXX in place, which stays within the string's bounds
    std::string tmpl{"/tmp/waysurs_XXXXXX"};
    if (mkdtemp(tmpl.data()) == nullptr) {
      throw std::system_error{errno, std::system_category(), "mkdtemp failed"};
    }

    dir = tmpl;
    const fs::path tx_path{dir / "tx"};
    const fs::path rx_path{dir / "rx"};

    std::error_code ec; // best-effort cleanup, matches old unlink() behavior
    fs::remove(tx_path, ec);
    fs::remove(rx_path, ec);

    // Build argv before fork(): no allocation in the child (fork/malloc
    // safety).
    std::string cmd{"socat"};
    std::string arg_tx = std::string{"PTY,link="} + tx_path.string() + ",raw,echo=0";
    std::string arg_rx = std::string{"PTY,link="} + rx_path.string() + ",raw,echo=0";
    // NOLINTNEXTLINE(modernize-use-designated-initializers) -- std::array takes no designators
    std::array<char*, 4> args{cmd.data(), arg_tx.data(), arg_rx.data(), nullptr};

    socat_pid = fork();
    if (socat_pid == -1) {
      throw std::runtime_error{"SerialFixture: fork() failed"};
    }
    if (socat_pid == 0) {
      execvp(args[0], args.data());
      _exit(1);
    }

    wait_for_links(tx_path, rx_path);

    // Catch2 runs listeners on the main thread before any test begins, so setenv is safe here
    // NOLINTBEGIN(concurrency-mt-unsafe)
    setenv("WAYSURS_SERIAL_TX", tx_path.c_str(), 1);
    setenv("WAYSURS_SERIAL_RX", rx_path.c_str(), 1);
    // NOLINTEND(concurrency-mt-unsafe)
  }

  void stop() {
    std::error_code ec; // non-throwing exists(), matches old access() behavior
    if (socat_pid > 0) {
      kill(socat_pid, SIGTERM);
      waitpid(socat_pid, nullptr, 0);
      socat_pid = -1;
    }

    fs::remove_all(dir, ec);
  }

  private:
  void wait_for_links(const fs::path& tx_path, const fs::path& rx_path) {
    const auto      deadline = std::chrono::steady_clock::now() + startup_timeout;
    std::error_code ec; // non-throwing exists(), matches old access() behavior

    while (!fs::exists(tx_path, ec) || !fs::exists(rx_path, ec)) {
      // Detect early child death (e.g. socat not installed) instead of
      // spinning forever waiting for links that will never appear.
      int status = 0;
      if (waitpid(socat_pid, &status, WNOHANG) == socat_pid) {
        socat_pid = -1;
        throw std::runtime_error{"SerialFixture: socat exited during startup — is it installed?"};
      }
      if (std::chrono::steady_clock::now() >= deadline) {
        stop();
        throw std::runtime_error{"SerialFixture: timed out waiting for PTY links"};
      }
      std::this_thread::sleep_for(poll_interval);
    }
  }
};

#include "../../src/serial_listener.ipp"