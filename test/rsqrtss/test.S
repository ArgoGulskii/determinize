#include "../helpers.h"

.intel_syntax noprefix

.section .text

.macro rsqrtss_deterministic dst, src
.global rsqrtss_deterministic_\dst\()_\src
rsqrtss_deterministic_\dst\()_\src\():
  movaps xmm\dst, [rdi]
  movaps xmm\src, [rdi]
  call rsqrtss_deterministic_\dst\()_\src\()_impl
  movaps [rdi], xmm\dst
  ret

.global rsqrtss_deterministic_\dst\()_\src\()_impl
rsqrtss_deterministic_\dst\()_\src\()_impl:
  rsqrtss xmm\dst, xmm\src
  nop
  nop
  nop
  nop
  ret
.endm

for_all_xmm_pairs rsqrtss_deterministic

.section .note.GNU-stack, "", %progbits
