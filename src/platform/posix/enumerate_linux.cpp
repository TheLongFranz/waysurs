#include <expected>
#include <string>
#include <vector>

// system headers

// waysurs headers
#include <waysurs/error.hpp>
#include <waysurs/waysurs.hpp>

namespace waysurs {
  [[nodiscard, maybe_unused]] auto list_ports() -> std::expected<std::vector<std::string>, error> {
    return {};
  }
} // namespace waysurs