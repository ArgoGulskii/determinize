#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

#define RSQRTPS_DETERMINISTIC_TEST(dst, src)                                                    \
  extern "C" uint64_t rsqrtps_deterministic_##dst##_##src(float* vec);                          \
  extern "C" uint64_t rsqrtps_deterministic_##dst##_##src##_impl(float* vec);                   \
  TEST_CASE("rsqrtps_deterministic_unwrapped(xmm" #dst ", xmm" #src ")", "[rsqrtps]") {         \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};                      \
    rsqrtps_deterministic_##dst##_##src(vec);                                                   \
    CHECK_APPROXIMATE(vec[0], 1.0f / sqrtf(2));                                                 \
    CHECK_APPROXIMATE(vec[1], 1.0f / sqrtf(M_PI));                                              \
    CHECK_APPROXIMATE(vec[2], 1.0f / sqrtf(420.0));                                             \
    CHECK_APPROXIMATE(vec[3], 1.0f / sqrtf(69.0));                                              \
  }                                                                                             \
                                                                                                \
  TEST_CASE("rsqrtps_deterministic_wrapped(xmm" #dst ", xmm" #src ")", "[rsqrtps]") {           \
    size_t target_length =                                                                      \
        *determinize::GetInstructionLength(rsqrtps_deterministic_##dst##_##src##_impl);         \
    REQUIRE(determinize::Replace(                                                               \
        determinize::GetDeterministicRsqrtps(ZYDIS_REGISTER_XMM##dst, ZYDIS_REGISTER_XMM##src), \
        reinterpret_cast<void*>(rsqrtps_deterministic_##dst##_##src##_impl), target_length));   \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, 420.0, 69.0};                      \
    rsqrtps_deterministic_##dst##_##src(vec);                                                   \
    CHECK_EXACT(vec[0], 1.0f / sqrtf(2));                                                       \
    CHECK_EXACT(vec[1], 1.0f / sqrtf(M_PI));                                                    \
    CHECK_EXACT(vec[2], 1.0f / sqrtf(420.0));                                                   \
    CHECK_EXACT(vec[3], 1.0f / sqrtf(69.0));                                                    \
  }

#define XMM_PAIR(dst, src) RSQRTPS_DETERMINISTIC_TEST(dst, src)
FOR_ALL_XMM_PAIRS()
#undef XMM_PAIR
