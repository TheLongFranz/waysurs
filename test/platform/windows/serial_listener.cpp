
#include <array>
#include <chrono>
#include <cstdlib>
#include <filesystem>
#include <regex>
#include <stdexcept>
#include <string>
#include <system_error>
#include <thread>
#include <vector>

#define WIN32_LEAN_AND_MEAN
#define NOMINMAX
#include <windows.h>

#include "../../src/serial_listener.hpp"

namespace fs = std::filesystem;

struct serial_listener::impl {
  static constexpr auto poll_interval   = std::chrono::milliseconds{50};
  static constexpr auto startup_timeout = std::chrono::seconds{5};
  static constexpr auto setupc_timeout  = std::chrono::seconds{30};

  fs::path setupc_path;
  int      pair_number{-1};

  impl() = default;
  ~impl() { stop(); }

  /// @note impl owns a com0com virtual port pair registered with the kernel driver;
  /// serial_listener moves the owning unique_ptr instead
  impl(const impl&)            = delete;
  impl& operator=(const impl&) = delete;
  impl(impl&&)                 = delete;
  impl& operator=(impl&&)      = delete;

  void start() {
    setupc_path = locate_setupc();

    // "-" "-" means driver defaults, i.e. port names equal to their IDs. That, like an explicit
    // PortName=, is usable almost at once -- unlike a PortName=COM# pair, which needs Windows PnP
    // to assign a number and takes longer.
    const std::string output{
      run_setupc({"--silent", "install", "-", "-"}, /*capture_output=*/true)
    };

    static const std::regex pair_id_pattern{R"(CNCA(\d+))"};
    std::smatch             match;
    if (!std::regex_search(output, match, pair_id_pattern)) {
      throw std::runtime_error{
        "SerialFixture: could not parse pair ID from setupc.exe output: " + output
      };
    }
    pair_number = std::stoi(match[1].str());

    const std::string tx_device{R"(\\.\CNCA)" + std::to_string(pair_number)};
    const std::string rx_device{R"(\\.\CNCB)" + std::to_string(pair_number)};

    wait_for_ports(tx_device, rx_device);

    // Catch2 runs listeners on the main thread before any test begins, so _putenv_s is safe here
    _putenv_s("WAYSURS_SERIAL_TX", tx_device.c_str());
    _putenv_s("WAYSURS_SERIAL_RX", rx_device.c_str());
  }

  void stop() {
    if (pair_number < 0) {
      return;
    }

    try {
      [[maybe_unused]] const auto _ =
        run_setupc({"--silent", "remove", std::to_string(pair_number)}, /*capture_output=*/false);
      // best-effort cleanup, matches posix's fire-and-forget kill()/waitpid()
      // NOLINTNEXTLINE(bugprone-empty-catch)
    } catch (...) {
    }

    cleanup_registry(pair_number);
    pair_number = -1;
  }

  private:
  [[nodiscard]] static auto locate_setupc() -> fs::path {
    // Catch2 runs listeners on the main thread before any test begins, so getenv is safe here
    // NOLINTNEXTLINE(concurrency-mt-unsafe)
    if (const char* override_path = std::getenv("WAYSURS_COM0COM_SETUPC");
        override_path != nullptr) {
      return fs::path{override_path};
    }

    std::array<char, MAX_PATH> buffer{};
    const DWORD                length{SearchPathA(
      nullptr, "setupc.exe", nullptr, static_cast<DWORD>(buffer.size()), buffer.data(), nullptr
    )};
    if (length == 0 || length >= buffer.size()) {
      throw std::runtime_error{
        "SerialFixture: could not locate setupc.exe -- set WAYSURS_COM0COM_SETUPC or add "
        "com0com's package folder to PATH"
      };
    }
    return fs::path{buffer.data()};
  }

  /// @note setupc.exe must run with its cwd set to its own folder, where the .inf files live
  [[nodiscard]] auto run_setupc(const std::vector<std::string>& args, bool capture_output) const
    -> std::string {
    std::string command_line{'"' + setupc_path.string() + '"'};
    for (const auto& arg: args) {
      command_line += ' ';
      command_line += arg;
    }

    SECURITY_ATTRIBUTES pipe_attrs{sizeof(SECURITY_ATTRIBUTES), nullptr, TRUE};

    HANDLE stdout_read{nullptr};
    HANDLE stdout_write{nullptr};
    if (capture_output) {
      if (CreatePipe(&stdout_read, &stdout_write, &pipe_attrs, 0) == 0) {
        throw std::system_error{
          static_cast<int>(GetLastError()), std::system_category(),
          "SerialFixture: CreatePipe failed"
        };
      }
      SetHandleInformation(stdout_read, HANDLE_FLAG_INHERIT, 0);
    }

    STARTUPINFOA startup_info{};
    startup_info.cb = sizeof(startup_info);
    if (capture_output) {
      startup_info.dwFlags    = STARTF_USESTDHANDLES;
      startup_info.hStdOutput = stdout_write;
      startup_info.hStdError  = stdout_write;
    }

    PROCESS_INFORMATION process_info{};
    const std::string   working_dir{setupc_path.parent_path().string()};
    const BOOL          started{CreateProcessA(
      nullptr, command_line.data(), nullptr, nullptr, capture_output ? TRUE : FALSE,
      CREATE_NO_WINDOW, nullptr, working_dir.c_str(), &startup_info, &process_info
    )};

    if (capture_output) {
      CloseHandle(stdout_write);
    }

    if (started == 0) {
      if (capture_output) {
        CloseHandle(stdout_read);
      }
      throw std::system_error{
        static_cast<int>(GetLastError()), std::system_category(),
        "SerialFixture: failed to start setupc.exe"
      };
    }

    std::string output;
    if (capture_output) {
      constexpr auto               chunk_size = 256;
      std::array<char, chunk_size> chunk{};
      DWORD                        bytes_read{};
      while (ReadFile(
               stdout_read, chunk.data(), static_cast<DWORD>(chunk.size()), &bytes_read, nullptr
             ) != 0 &&
             bytes_read > 0) {
        output.append(chunk.data(), bytes_read);
      }
      CloseHandle(stdout_read);
    }

    constexpr auto ms_per_second = 1000;
    const DWORD    wait_result{WaitForSingleObject(
      process_info.hProcess, static_cast<DWORD>(setupc_timeout.count() * ms_per_second)
    )};

    DWORD exit_code{};
    if (wait_result != WAIT_OBJECT_0) {
      TerminateProcess(process_info.hProcess, 1);
      exit_code = 1;
    } else {
      GetExitCodeProcess(process_info.hProcess, &exit_code);
    }
    CloseHandle(process_info.hProcess);
    CloseHandle(process_info.hThread);

    if (exit_code != 0) {
      throw std::runtime_error{
        "SerialFixture: setupc.exe exited with code " + std::to_string(exit_code)
      };
    }

    return output;
  }

  [[nodiscard]] static auto port_ready(const std::string& device_path) -> bool {
    HANDLE handle{CreateFileA(
      device_path.c_str(), GENERIC_READ | GENERIC_WRITE, 0, nullptr, OPEN_EXISTING, 0, nullptr
    )};
    if (handle == INVALID_HANDLE_VALUE) {
      return false;
    }
    CloseHandle(handle);
    return true;
  }

  void wait_for_ports(const std::string& tx_device, const std::string& rx_device) {
    const auto deadline = std::chrono::steady_clock::now() + startup_timeout;

    while (!port_ready(tx_device) || !port_ready(rx_device)) {
      if (std::chrono::steady_clock::now() >= deadline) {
        stop();
        throw std::runtime_error{"SerialFixture: timed out waiting for com0com ports"};
      }
      std::this_thread::sleep_for(poll_interval);
    }
  }

  /// @note setupc remove leaves the pair's settings in the registry, and the next pair with the
  /// same number would inherit them; deleting these keys keeps the fixture leaving no trace
  static void cleanup_registry(int n) {
    HKEY parameters{};
    if (RegOpenKeyExA(
          HKEY_LOCAL_MACHINE, R"(SYSTEM\CurrentControlSet\Services\com0com\Parameters)", 0,
          KEY_ALL_ACCESS, &parameters
        ) != ERROR_SUCCESS) {
      return;
    }
    RegDeleteKeyA(parameters, ("CNCA" + std::to_string(n)).c_str());
    RegDeleteKeyA(parameters, ("CNCB" + std::to_string(n)).c_str());
    RegCloseKey(parameters);
  }
};

#include "../../src/serial_listener.ipp"
