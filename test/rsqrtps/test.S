#include "../helpers.h"

.intel_syntax noprefix

.section .text

.macro rsqrtps_register dst, src
.global rsqrtps_register_\dst\()_\src
rsqrtps_register_\dst\()_\src\():
  movaps xmm\src, [rdi]
  call rsqrtps_register_\dst\()_\src\()_impl
  movaps [rdi], xmm\dst
  ret

.global rsqrtps_register_\dst\()_\src\()_impl
rsqrtps_register_\dst\()_\src\()_impl:
  rsqrtps xmm\dst, xmm\src
  nop
  nop
  nop
  nop
  ret
.endm

for_all_xmm_pairs rsqrtps_register

.global rsqrtps_memory_rdi
.global rsqrtps_memory_rdi_impl
rsqrtps_memory_rdi:
rsqrtps_memory_rdi_impl:
  rsqrtps xmm0, [rdi]
  movaps [rdi], xmm0
  ret

.global rsqrtps_memory_rsp
rsqrtps_memory_rsp:
  push rbp
  mov rbp, rsp
  mov rsp, rdi

  call rsqrtps_memory_rsp_impl

  mov rsp, rbp
  pop rbp
  ret

.global rsqrtps_memory_rsp_impl
rsqrtps_memory_rsp_impl:
  rsqrtps xmm0, [rsp + 8]
  movaps [rdi], xmm0
  ret

.section .note.GNU-stack, "", %progbits
