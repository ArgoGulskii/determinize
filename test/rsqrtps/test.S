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

.global rsqrtps_memory_rdi
.global rsqrtps_memory_rdi_impl
rsqrtps_memory_rdi:
rsqrtps_memory_rdi_impl:
  rsqrtps xmm0, [rdi]
  movaps [rdi], xmm0
  ret

.section .note.GNU-stack, "", %progbits
