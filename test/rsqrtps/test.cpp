#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

#define RSQRTPS_REGISTER_TEST(dst, src)                                                  \
  extern "C" uint64_t rsqrtps_register_##dst##_##src(float* vec);                        \
  extern "C" uint64_t rsqrtps_register_##dst##_##src##_impl(float* vec);                 \
  TEST_CASE("rsqrtps_register_" #dst "_" #src, "[rsqrtps][register][unwrapped]") {       \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};               \
    rsqrtps_register_##dst##_##src(vec);                                                 \
    CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));                                          \
    CHECK_APPROXIMATE(vec[1], 1.0f / sqrtf(M_PI));                                       \
    CHECK_APPROXIMATE(vec[2], 1.0f / sqrtf(420.0));                                      \
    CHECK_APPROXIMATE(vec[3], 1.0f / sqrtf(69.0));                                       \
  }                                                                                      \
                                                                                         \
  TEST_CASE("rsqrtps_register_wrapped_" #dst "_" #src, "[rsqrtps][register][wrapped]") { \
    REQUIRE(determinize::Determinize(                                                    \
        {reinterpret_cast<void*>(rsqrtps_register_##dst##_##src##_impl)}));              \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};               \
    rsqrtps_register_##dst##_##src(vec);                                                 \
    CHECK_EXACT(vec[0], 1.0f / sqrtf(2));                                                \
    CHECK_EXACT(vec[1], 1.0f / sqrtf(M_PI));                                             \
    CHECK_EXACT(vec[2], 1.0f / sqrtf(420.0));                                            \
    CHECK_EXACT(vec[3], 1.0f / sqrtf(69.0));                                             \
  }

#define XMM_PAIR(dst, src) RSQRTPS_REGISTER_TEST(dst, src)
FOR_ALL_XMM_PAIRS()
#undef XMM_PAIR

#define RSQRTPS_MEMORY_TEST(reg)                                                               \
  extern "C" uint64_t rsqrtps_memory_##reg(float* vec);                                        \
  extern "C" uint64_t rsqrtps_memory_##reg##_impl(float* vec);                                 \
  TEST_CASE("rsqrtps_memory_" #reg, "[rsqrtps][memory][unwrapped][" #reg "]") {                \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};                     \
    rsqrtps_memory_##reg(vec);                                                                 \
    CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));                                                \
    CHECK_APPROXIMATE(vec[1], 1.0f / sqrtf(M_PI));                                             \
    CHECK_APPROXIMATE(vec[2], 1.0f / sqrtf(420.0));                                            \
    CHECK_APPROXIMATE(vec[3], 1.0f / sqrtf(69.0));                                             \
  }                                                                                            \
                                                                                               \
  TEST_CASE("rsqrtps_memory_wrapped_ " #reg, "[rsqrtps][memory][wrapped][" #reg "]") {         \
    REQUIRE(determinize::Determinize({reinterpret_cast<void*>(rsqrtps_memory_##reg##_impl)})); \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};                     \
    rsqrtps_memory_##reg(vec);                                                                 \
    CHECK_EXACT(vec[0], 1.0f / sqrtf(2));                                                      \
    CHECK_EXACT(vec[1], 1.0f / sqrtf(M_PI));                                                   \
    CHECK_EXACT(vec[2], 1.0f / sqrtf(420.0));                                                  \
    CHECK_EXACT(vec[3], 1.0f / sqrtf(69.0));                                                   \
  }

RSQRTPS_MEMORY_TEST(rdi)
RSQRTPS_MEMORY_TEST(rsp)
