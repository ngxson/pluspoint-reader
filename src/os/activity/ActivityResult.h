#pragma once

#include <cstdint>
#include <functional>
#include <string>
#include <type_traits>
#include <utility>
#include <memory>

struct ActivityResult {
  bool isCancelled = false;
  explicit ActivityResult() = default;
};

using ActivityResultHandler = std::function<void(std::unique_ptr<ActivityResult>&&)>;
