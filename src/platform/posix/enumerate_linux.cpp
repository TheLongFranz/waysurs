#include <expected>
#include <filesystem>
#include <string>
#include <vector>

// System headers
#include <sys/stat.h>

// waysurs headers
#include <waysurs/error.hpp>
#include <waysurs/waysurs.hpp>

namespace waysurs {
  [[nodiscard]] auto list_ports() -> std::expected<std::vector<std::string>, error> {
    std::vector<std::string> out{};
    struct stat              buffer{};

    const auto p{std::filesystem::path{"/dev/serial/by-id"}};
    // const auto p{std::filesystem::path{"/sys/class/tty"}};

    for (const auto& it: std::filesystem::directory_iterator(p)) {
      if (const auto result = lstat(it.path().c_str(), &buffer); result >= 0) {
        if (std::filesystem::is_symlink(it.symlink_status())) {
          const auto symlink_points_at = std::filesystem::read_symlink(it);
          const auto canonical_path    = std::filesystem::canonical(symlink_points_at);
          out.emplace_back(canonical_path.string());
        }
      }
    }

    return out;
  }
} // namespace waysurs