#include <expected>
#include <filesystem>
#include <string>
#include <vector>

// System headers
#include <sys/stat.h>

// waysurs headers
#include <waysurs/error.hpp>
#include <waysurs/waysurs.hpp>

[[nodiscard]] auto waysurs::list_ports() -> std::expected<std::vector<std::string>, error> {
  std::vector<std::string> out{};

  try {
    for (const auto& it: std::filesystem::directory_iterator("/dev/serial/by-id")) {
      if (it.is_symlink()) {
        const auto target{std::filesystem::canonical(std::filesystem::read_symlink(it))};
        out.emplace_back(target.string());
      }
    }
    return out;
  } catch (const std::filesystem::filesystem_error&) {
    // list_ports() always returns a vector, if there are no serial devices then the vector
    // will be empty. I don't consider a lack of serial devices to be an error but if we end
    // up in here then we should be polite and catch the exception.
    return out;
  }
  return std::unexpected(
    detail::make_error(
      error_type::config, "If you've reached this point then something has gone very wrong"
    )
  );
}