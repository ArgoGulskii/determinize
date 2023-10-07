#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

#define RCPPS_REGISTER_TEST(dst, src)                                                              \
  extern "C" uint64_t rcpps_register_##dst##_##src(float* vec);                                    \
  extern "C" uint64_t rcpps_register_##dst##_##src##_impl(float* vec);                             \
  TEST_CASE("rcpps_register_" #dst "_" #src, "[rcpps][register][unwrapped]") {                     \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};                         \
    rcpps_register_##dst##_##src(vec);                                                             \
    CHECK_APPROXIMATE(vec[0], 1.0f / 2.0f);                                                        \
    CHECK_APPROXIMATE(vec[1], 1.0f / static_cast<float>(M_PI));                                    \
    CHECK_APPROXIMATE(vec[2], 1.0f / -2.0f);                                                       \
    CHECK_APPROXIMATE(vec[3], 1.0f / static_cast<float>(-M_PI));                                   \
  }                                                                                                \
                                                                                                   \
  TEST_CASE("rcpps_register_wrapped_" #dst "_" #src, "[rcpps][register][wrapped]") {               \
    REQUIRE(                                                                                       \
        determinize::Determinize({reinterpret_cast<void*>(rcpps_register_##dst##_##src##_impl)})); \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};                         \
    rcpps_register_##dst##_##src(vec);                                                             \
    CHECK_EXACT(vec[0], 1.0f / 2.0f);                                                              \
    CHECK_EXACT(vec[1], 1.0f / static_cast<float>(M_PI));                                          \
    CHECK_EXACT(vec[2], 1.0f / -2.0f);                                                             \
    CHECK_EXACT(vec[3], 1.0f / static_cast<float>(-M_PI));                                         \
  }

#define XMM_PAIR(dst, src) RCPPS_REGISTER_TEST(dst, src)
FOR_ALL_XMM_PAIRS()
#undef XMM_PAIR

#define RCPPS_MEMORY_TEST(reg)                                                               \
  extern "C" uint64_t rcpps_memory_##reg(float* vec);                                        \
  extern "C" uint64_t rcpps_memory_##reg##_impl(float* vec);                                 \
  TEST_CASE("rcpps_memory_" #reg, "[rcpps][memory][unwrapped][" #reg "]") {                  \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};                   \
    rcpps_memory_##reg(vec);                                                                 \
    CHECK_APPROXIMATE(vec[0], 1.0f / 2.0f);                                                  \
    CHECK_APPROXIMATE(vec[1], 1.0f / static_cast<float>(M_PI));                              \
    CHECK_APPROXIMATE(vec[2], 1.0f / -2.0f);                                                 \
    CHECK_APPROXIMATE(vec[3], 1.0f / static_cast<float>(-M_PI));                             \
  }                                                                                          \
                                                                                             \
  TEST_CASE("rcpps_memory_wrapped_" #reg, "[rcpps][memory][wrapped][" #reg "]") {            \
    REQUIRE(determinize::Determinize({reinterpret_cast<void*>(rcpps_memory_##reg##_impl)})); \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};                   \
    rcpps_memory_##reg(vec);                                                                 \
    CHECK_EXACT(vec[0], 1.0f / 2.0f);                                                        \
    CHECK_EXACT(vec[1], 1.0f / static_cast<float>(M_PI));                                    \
    CHECK_EXACT(vec[2], 1.0f / -2.0f);                                                       \
    CHECK_EXACT(vec[3], 1.0f / static_cast<float>(-M_PI));                                   \
  }

RCPPS_MEMORY_TEST(rdi)
