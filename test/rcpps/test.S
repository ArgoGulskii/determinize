#include "../helpers.h"

.intel_syntax noprefix

.section .text

.macro rcpps_deterministic dst, src
.global rcpps_deterministic_\dst\()_\src
rcpps_deterministic_\dst\()_\src\():
  movaps xmm\src, [rdi]
  call rcpps_deterministic_\dst\()_\src\()_impl
  movaps [rdi], xmm\dst
  ret

.global rcpps_deterministic_\dst\()_\src\()_impl
rcpps_deterministic_\dst\()_\src\()_impl:
  rcpps xmm\dst, xmm\src
  nop
  nop
  nop
  nop
  ret
.endm

for_all_xmm_pairs rcpps_deterministic

.section .note.GNU-stack, "", %progbits
