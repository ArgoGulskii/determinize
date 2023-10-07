#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

#define RSQRTPS_DETERMINISTIC_TEST(dst, src)                                            \
  extern "C" uint64_t rsqrtss_deterministic_##dst##_##src(float* vec);                  \
  extern "C" uint64_t rsqrtss_deterministic_##dst##_##src##_impl(float* vec);           \
  TEST_CASE("rsqrtss_deterministic_unwrapped(xmm" #dst ", xmm" #src ")", "[rsqrtss]") { \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};              \
    rsqrtss_deterministic_##dst##_##src(vec);                                           \
    CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));                                         \
    CHECK_EXACT(vec[1], static_cast<float>(M_PI));                                      \
    CHECK_EXACT(vec[2], 420.0);                                                         \
    CHECK_EXACT(vec[3], 69.0);                                                          \
  }                                                                                     \
                                                                                        \
  TEST_CASE("rsqrtss_deterministic_wrapped(xmm" #dst ", xmm" #src ")", "[rsqrtss]") {   \
    REQUIRE(determinize::Determinize(                                                   \
        {reinterpret_cast<void*>(rsqrtss_deterministic_##dst##_##src##_impl)}));        \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};              \
    rsqrtss_deterministic_##dst##_##src(vec);                                           \
    CHECK_EXACT(vec[0], 1.0f / sqrtf(2));                                               \
    CHECK_EXACT(vec[1], static_cast<float>(M_PI));                                      \
    CHECK_EXACT(vec[2], 420.0);                                                         \
    CHECK_EXACT(vec[3], 69.0);                                                          \
  }

#define XMM_PAIR(dst, src) RSQRTPS_DETERMINISTIC_TEST(dst, src)
FOR_ALL_XMM_PAIRS()
#undef XMM_PAIR

extern "C" uint64_t rsqrtss_memory_rdi(float* vec);
extern "C" uint64_t rsqrtss_memory_rdi_impl(float* vec);
TEST_CASE("rsqrtss_memory_rdi", "[rsqrtss][memory]") {
  float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};
  rsqrtss_memory_rdi(vec);
  CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));
  CHECK_EXACT(vec[1], static_cast<float>(M_PI));
  CHECK_EXACT(vec[2], 420.0);
  CHECK_EXACT(vec[3], 69.0);
}

TEST_CASE("rsqrtss_memory_rdi_wrapped", "[rsqrtss][memory][wrapped]") {
  REQUIRE(determinize::Determinize({reinterpret_cast<void*>(rsqrtss_memory_rdi_impl)}));
  float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};
  rsqrtss_memory_rdi(vec);
  CHECK_EXACT(vec[0], 1.0f / sqrtf(2));
  CHECK_EXACT(vec[1], static_cast<float>(M_PI));
  CHECK_EXACT(vec[2], 420.0);
  CHECK_EXACT(vec[3], 69.0);
}
