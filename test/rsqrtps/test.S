#include "../helpers.h"

.intel_syntax noprefix

.section .text

.macro rsqrtps_deterministic dst, src
.global rsqrtps_deterministic_\dst\()_\src
rsqrtps_deterministic_\dst\()_\src\():
  movaps xmm\src, [rdi]
  call rsqrtps_deterministic_\dst\()_\src\()_impl
  movaps [rdi], xmm\dst
  ret

.global rsqrtps_deterministic_\dst\()_\src\()_impl
rsqrtps_deterministic_\dst\()_\src\()_impl:
  rsqrtps xmm\dst, xmm\src
  nop
  nop
  nop
  nop
  ret
.endm

for_all_xmm_pairs rsqrtps_deterministic

.section .note.GNU-stack, "", %progbits
