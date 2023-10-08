#include "../helpers.h"

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>
#include <determinize/thunk.h>

using namespace Catch::Matchers;

#define JMP_TEST(jmp_type)                                                       \
  extern "C" uint64_t jmp_type(float* vec, uint64_t val);                        \
  extern "C" uint64_t jmp_type##_impl(float* vec, uint64_t val);                 \
  TEST_CASE(#jmp_type, "[branch][unwrapped][" #jmp_type "]") {                   \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};       \
    CHECK(jmp_type(vec, 0) == 0);                                                \
    CHECK_APPROXIMATE(vec[0], 1.0f / 2.0f);                                      \
    CHECK_APPROXIMATE(vec[1], 1.0f / static_cast<float>(M_PI));                  \
    CHECK_APPROXIMATE(vec[2], 1.0f / -2.0f);                                     \
    CHECK_APPROXIMATE(vec[3], 1.0f / static_cast<float>(-M_PI));                 \
    CHECK(jmp_type(vec, 1) == 1);                                                \
  }                                                                              \
                                                                                 \
  TEST_CASE(#jmp_type "_wrapped", "[branch][wrapped][" #jmp_type "]") {          \
    float vec[4] __attribute__((aligned(16))) = {2.0f, M_PI, -2.0, -M_PI};       \
    CHECK(determinize::Determinize({reinterpret_cast<void*>(jmp_type##_impl)})); \
    CHECK(jmp_type(vec, 0) == 0);                                                \
    CHECK_EXACT(vec[0], 1.0f / 2.0f);                                            \
    CHECK_EXACT(vec[1], 1.0f / static_cast<float>(M_PI));                        \
    CHECK_EXACT(vec[2], 1.0f / -2.0f);                                           \
    CHECK_EXACT(vec[3], 1.0f / static_cast<float>(-M_PI));                       \
    CHECK(jmp_type(vec, 1) == 1);                                                \
  }

JMP_TEST(jmp_rel8)
JMP_TEST(je_rel8)
