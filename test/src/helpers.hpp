#pragma once

#include <cstdlib>
#include <format>
#include <string>

#include <catch2/catch_message.hpp>
#include <catch2/catch_test_macros.hpp>

#define CHECK_RESULT(var_expr)                                                                     \
  do {                                                                                             \
    const auto& _result = (var_expr);                                                              \
    if (!_result.has_value()) {                                                                    \
      UNSCOPED_INFO(std::format("{}", _result.error()));                                           \
    }                                                                                              \
    REQUIRE(_result.has_value());                                                                  \
  } while (0)

[[nodiscard]] inline auto get_env(const char* env) -> const char* {
  const char* result = std::getenv(env);
  if (result == nullptr) {
    INFO(std::format("Environment variable {} does not exist", env));
  }
  REQUIRE(result != nullptr);
  return result;
}

[[nodiscard]] inline auto to_string(const auto& vec) -> std::string {
  return std::string(reinterpret_cast<const char*>(vec.data()), vec.size());
}
