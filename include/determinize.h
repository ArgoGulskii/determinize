#pragma once

#include <stddef.h>

namespace determinize {

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
