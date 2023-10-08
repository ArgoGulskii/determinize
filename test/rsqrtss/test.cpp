#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

#define RSQRTSS_REGISTER_TEST(dst, src)                                                  \
  extern "C" uint64_t rsqrtss_register_##dst##_##src(float* vec);                        \
  extern "C" uint64_t rsqrtss_register_##dst##_##src##_impl(float* vec);                 \
  TEST_CASE("rsqrtss_register_" #dst "_" #src, "[rsqrtss][register][unwrapped]") {       \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};               \
    rsqrtss_register_##dst##_##src(vec);                                                 \
    CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));                                          \
    CHECK_EXACT(vec[1], static_cast<float>(M_PI));                                       \
    CHECK_EXACT(vec[2], 420.0);                                                          \
    CHECK_EXACT(vec[3], 69.0);                                                           \
  }                                                                                      \
                                                                                         \
  TEST_CASE("rsqrtss_register_wrapped_" #dst "_" #src, "[rsqrtss][register][wrapped]") { \
    REQUIRE(determinize::Determinize(                                                    \
        {reinterpret_cast<void*>(rsqrtss_register_##dst##_##src##_impl)}));              \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};               \
    rsqrtss_register_##dst##_##src(vec);                                                 \
    CHECK_EXACT(vec[0], 1.0f / sqrtf(2));                                                \
    CHECK_EXACT(vec[1], static_cast<float>(M_PI));                                       \
    CHECK_EXACT(vec[2], 420.0);                                                          \
    CHECK_EXACT(vec[3], 69.0);                                                           \
  }

#define XMM_PAIR(dst, src) RSQRTSS_REGISTER_TEST(dst, src)
FOR_ALL_XMM_PAIRS()
#undef XMM_PAIR

#define RSQRTSS_MEMORY_TEST(reg)                                                               \
  extern "C" uint64_t rsqrtss_memory_##reg(float* vec);                                        \
  extern "C" uint64_t rsqrtss_memory_##reg##_impl(float* vec);                                 \
  TEST_CASE("rsqrtss_memory_" #reg, "[rsqrtss][memory][unwrapped][" #reg "]") {                \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};                     \
    rsqrtss_memory_##reg(vec);                                                                 \
    CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));                                                \
    CHECK_EXACT(vec[1], static_cast<float>(M_PI));                                             \
    CHECK_EXACT(vec[2], 420.0);                                                                \
    CHECK_EXACT(vec[3], 69.0);                                                                 \
  }                                                                                            \
                                                                                               \
  TEST_CASE("rsqrtss_memory_" #reg "_wrapped", "[rsqrtss][memory][wrapped][" #reg "]") {       \
    REQUIRE(determinize::Determinize({reinterpret_cast<void*>(rsqrtss_memory_##reg##_impl)})); \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};                     \
    rsqrtss_memory_##reg(vec);                                                                 \
    CHECK_EXACT(vec[0], 1.0f / sqrtf(2));                                                      \
    CHECK_EXACT(vec[1], static_cast<float>(M_PI));                                             \
    CHECK_EXACT(vec[2], 420.0);                                                                \
    CHECK_EXACT(vec[3], 69.0);                                                                 \
  }

RSQRTSS_MEMORY_TEST(rdi)
RSQRTSS_MEMORY_TEST(rsp)
