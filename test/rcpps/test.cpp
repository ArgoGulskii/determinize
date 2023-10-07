#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

#define RCPPS_DETERMINISTIC_TEST(dst, src)                                                    \
  extern "C" uint64_t rcpps_deterministic_##dst##_##src(float* vec);                          \
  extern "C" uint64_t rcpps_deterministic_##dst##_##src##_impl(float* vec);                   \
  TEST_CASE("rcpps_deterministic_unwrapped(xmm" #dst ", xmm" #src ")", "[rcpps][register]") { \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};                    \
    rcpps_deterministic_##dst##_##src(vec);                                                   \
    CHECK_APPROXIMATE(vec[0], 1.0f / 2.0f);                                                   \
    CHECK_APPROXIMATE(vec[1], 1.0f / static_cast<float>(M_PI));                               \
    CHECK_APPROXIMATE(vec[2], 1.0f / -2.0f);                                                  \
    CHECK_APPROXIMATE(vec[3], 1.0f / static_cast<float>(-M_PI));                              \
  }                                                                                           \
                                                                                              \
  TEST_CASE("rcpps_deterministic_wrapped(xmm" #dst ", xmm" #src ")",                          \
            "[rcpps][register][wrapped]") {                                                   \
    REQUIRE(determinize::Determinize(                                                         \
        {reinterpret_cast<void*>(rcpps_deterministic_##dst##_##src##_impl)}));                \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};                    \
    rcpps_deterministic_##dst##_##src(vec);                                                   \
    CHECK_EXACT(vec[0], 1.0f / 2.0f);                                                         \
    CHECK_EXACT(vec[1], 1.0f / static_cast<float>(M_PI));                                     \
    CHECK_EXACT(vec[2], 1.0f / -2.0f);                                                        \
    CHECK_EXACT(vec[3], 1.0f / static_cast<float>(-M_PI));                                    \
  }

#define XMM_PAIR(dst, src) RCPPS_DETERMINISTIC_TEST(dst, src)
FOR_ALL_XMM_PAIRS()
#undef XMM_PAIR

extern "C" uint64_t rcpps_memory_rdi(float* vec);
extern "C" uint64_t rcpps_memory_rdi_impl(float* vec);
TEST_CASE("rcpps_memory_rdi", "[rcpps][memory]") {
  float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};
  rcpps_memory_rdi(vec);
  CHECK_APPROXIMATE(vec[0], 1.0f / 2.0f);
  CHECK_APPROXIMATE(vec[1], 1.0f / static_cast<float>(M_PI));
  CHECK_APPROXIMATE(vec[2], 1.0f / -2.0f);
  CHECK_APPROXIMATE(vec[3], 1.0f / static_cast<float>(-M_PI));
}

TEST_CASE("rcpps_memory_rdi_wrapped", "[rcpps][memory][wrapped]") {
  REQUIRE(determinize::Determinize({reinterpret_cast<void*>(rcpps_memory_rdi_impl)}));
  float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};
  rcpps_memory_rdi(vec);
  CHECK_EXACT(vec[0], 1.0f / 2.0f);
  CHECK_EXACT(vec[1], 1.0f / static_cast<float>(M_PI));
  CHECK_EXACT(vec[2], 1.0f / -2.0f);
  CHECK_EXACT(vec[3], 1.0f / static_cast<float>(-M_PI));
}
