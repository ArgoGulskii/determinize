#include "../helpers.h"

.intel_syntax noprefix

.section .text

.macro rsqrtss_register dst, src
.global rsqrtss_register_\dst\()_\src
rsqrtss_register_\dst\()_\src\():
  movaps xmm\dst, [rdi]
  movaps xmm\src, [rdi]
  call rsqrtss_register_\dst\()_\src\()_impl
  movaps [rdi], xmm\dst
  ret

.global rsqrtss_register_\dst\()_\src\()_impl
rsqrtss_register_\dst\()_\src\()_impl:
  rsqrtss xmm\dst, xmm\src
  nop
  nop
  nop
  nop
  ret
.endm

for_all_xmm_pairs rsqrtss_register

.global rsqrtss_memory_rdi
rsqrtss_memory_rdi:
  movaps xmm0, [rdi]
  call rsqrtss_memory_rdi_impl
  movaps [rdi], xmm0
  ret

.global rsqrtss_memory_rdi_impl
rsqrtss_memory_rdi_impl:
  rsqrtss xmm0, [rdi]
  nop
  nop
  ret

.global rsqrtss_memory_rsp
rsqrtss_memory_rsp:
  movaps xmm0, [rdi]

  push rbp
  mov rbp, rsp
  mov rsp, rdi

  call rsqrtss_memory_rsp_impl

  mov rsp, rbp
  pop rbp

  movaps [rdi], xmm0
  ret

.global rsqrtss_memory_rsp_impl
rsqrtss_memory_rsp_impl:
  rsqrtss xmm0, [rsp + 8]
  nop
  nop
  ret

.section .note.GNU-stack, "", %progbits
