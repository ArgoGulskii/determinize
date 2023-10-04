#pragma once

#include <stddef.h>
#include <string.h>

#include <optional>

namespace determinize {

std::optional<size_t> GetInstructionLength(void* p);

template <typename T, typename... Args>
static std::optional<size_t> GetInstructionLength(T (*p)(Args...)) {
  return GetInstructionLength(reinterpret_cast<void*>(p));
}

bool Replace(void* replacement, void* target, size_t target_length);

template <typename T, typename... Args>
static bool Replace(T (*replacement)(Args...), T (*target)(Args...), size_t target_length) {
  return Replace(reinterpret_cast<void*>(replacement), reinterpret_cast<void*>(target),
                 target_length);
}

template <typename T, typename... Args>
static bool Replace(T (*replacement)(Args..., ...), T (*target)(Args..., ...),
                    size_t target_length) {
  return Replace(reinterpret_cast<void*>(replacement), reinterpret_cast<void*>(target),
                 target_length);
}

}  // namespace determinize
