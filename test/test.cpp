#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include "determinize.h"

using namespace Catch::Matchers;

extern "C" uint64_t rcpps_wrapper(float* vec);
extern "C" uint64_t rcpps_impl(...);
extern "C" uint64_t rcpps_replacement(...);

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
