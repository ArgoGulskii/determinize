#include <math.h>
#include <xmmintrin.h>

#define CATCH_CONFIG_MAIN
#include <catch.hpp>

#include "determinize.h"

using namespace Catch::Matchers;

extern "C" uint64_t rcpps_wrapper(float* vec);
extern "C" uint64_t rcpps_impl(...);
extern "C" uint64_t rcpps_replacement(...);

__asm__("                             \n\
rcpps_wrapper:                        \n\
  movaps (%rdi), %xmm2                \n\
  call rcpps_impl                     \n\
  movaps %xmm1, (%rdi)                \n\
  ret                                 \n\
                                      \n\
rcpps_impl:                           \n\
  rcpps %xmm2, %xmm1                  \n\
  mov deadbeef(%rip), %rax            \n\
  nop                                 \n\
  nop                                 \n\
  nop                                 \n\
  nop                                 \n\
  nop                                 \n\
  nop                                 \n\
  ret                                 \n\
deadbeef:                             \n\
  .quad 0xdeadbeefdeadbeef            \n\
                                      \n\
rcpps_replacement:                    \n\
  movaps funny_numbers(%rip), %xmm1   \n\
  ret                                 \n\
.p2align 4                            \n\
funny_numbers:                        \n\
  .float 420.0                        \n\
  .float 69.0                         \n\
  .float -420.0                       \n\
  .float -69.0                        \n\
");

TEST_CASE("rcpps unwrapped", "[rcpps]") {
  float vec[4] __attribute__((aligned(16))) = {1.0f, M_PI, -1.0, -M_PI};
  uint64_t rc = rcpps_wrapper(vec);
  CHECK_THAT(vec[0], WithinRel(1.0, 0.001));
  CHECK_THAT(vec[1], WithinRel(1.0 / M_PI, 0.001));
  CHECK_THAT(vec[2], WithinRel(-1.0, 0.001));
  CHECK_THAT(vec[3], WithinRel(-1.0 / M_PI, 0.001));
  CHECK(rc == 0xdeadbeefdeadbeef);
}

TEST_CASE("rcpps wrapped", "[rcpps]") {
  REQUIRE(determinize::Replace(rcpps_replacement, rcpps_impl, 3));

  float vec[4] __attribute__((aligned(16))) = {1.0f, M_PI, -1.0, -M_PI};
  uint64_t rc = rcpps_wrapper(vec);
  CHECK(vec[0] == 420.0f);
  CHECK(vec[1] == 69.0f);
  CHECK(vec[2] == -420.0f);
  CHECK(vec[3] == -69.0f);
  CHECK(rc == 0xdeadbeefdeadbeef);
}
