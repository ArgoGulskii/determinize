#include "../helpers.h"

.intel_syntax noprefix

.section .text

.macro rcpps_register dst, src
.global rcpps_register_\dst\()_\src
rcpps_register_\dst\()_\src\():
  movaps xmm\src, [rdi]
  call rcpps_register_\dst\()_\src\()_impl
  movaps [rdi], xmm\dst
  ret

.global rcpps_register_\dst\()_\src\()_impl
rcpps_register_\dst\()_\src\()_impl:
  rcpps xmm\dst, xmm\src
  nop
  nop
  nop
  nop
  ret
.endm

for_all_xmm_pairs rcpps_register

.global rcpps_memory_rdi
.global rcpps_memory_rdi_impl
rcpps_memory_rdi:
rcpps_memory_rdi_impl:
  rcpps xmm0, [rdi]
  movaps [rdi], xmm0
  ret

.global rcpps_memory_rsp
rcpps_memory_rsp:
  push rbp
  mov rbp, rsp
  mov rsp, rdi

  call rcpps_memory_rsp_impl

  mov rsp, rbp
  pop rbp
  ret

.global rcpps_memory_rsp_impl
rcpps_memory_rsp_impl:
  rcpps xmm0, [rsp + 8]
  movaps [rdi], xmm0
  ret

.section .note.GNU-stack, "", %progbits
