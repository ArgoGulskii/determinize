#include <math.h>
#include <xmmintrin.h>

#include <catch.hpp>

#include <determinize/determinize.h>
#include <determinize/replacements.h>

#include "../helpers.h"

using namespace Catch::Matchers;

struct __attribute__((packed, aligned(16))) IntegrationArgs {
  // rcpps xmm0, [rdi]
  float xmm0[4];

  // rsqrtps xmm1, [rsp]
  float xmm1[4];

  // rsqrtss xmm2, xmm2
  float xmm2[4];
};

static constexpr IntegrationArgs args = {
    .xmm0 = {2.0, M_PI, -2.0, -M_PI},
    .xmm1 = {M_PI, 2.0, M_E, M_LN2},
    .xmm2 = {M_PI, -M_PI, 2.0, M_PI},
};

extern "C" void integration(IntegrationArgs* args);
extern "C" void integration_impl(...);

TEST_CASE("integration", "[integration][unwrapped]") {
  IntegrationArgs local = args;
  integration(&local);

  for (int i = 0; i < 4; ++i) {
    CHECK_APPROXIMATE(local.xmm0[i], 1.0f / args.xmm0[i]);
  }

  for (int i = 0; i < 4; ++i) {
    CHECK_APPROXIMATE(local.xmm1[i], 1.0f / sqrtf(args.xmm1[i]));
  }

  CHECK_APPROXIMATE(local.xmm2[0], 1.0f / sqrtf(args.xmm2[0]));
  CHECK_EXACT(local.xmm2[1], args.xmm2[1]);
  CHECK_EXACT(local.xmm2[2], args.xmm2[2]);
  CHECK_EXACT(local.xmm2[3], args.xmm2[3]);
}

TEST_CASE("integration_wrapped", "[integration][wrapped]") {
  IntegrationArgs local = args;

  char* addr = reinterpret_cast<char*>(integration_impl);
  determinize::Determinize({addr, addr + 3, addr + 8});
  integration(&local);

  for (int i = 0; i < 4; ++i) {
    CHECK_EXACT(local.xmm0[i], 1.0f / args.xmm0[i]);
  }

  for (int i = 0; i < 4; ++i) {
    CHECK_EXACT(local.xmm1[i], 1.0f / sqrtf(args.xmm1[i]));
  }

  CHECK_EXACT(local.xmm2[0], 1.0f / sqrtf(args.xmm2[0]));
  CHECK_EXACT(local.xmm2[1], args.xmm2[1]);
  CHECK_EXACT(local.xmm2[2], args.xmm2[2]);
  CHECK_EXACT(local.xmm2[3], args.xmm2[3]);
}
