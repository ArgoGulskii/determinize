.intel_syntax noprefix

.section .text
.global rcpps_smoke
rcpps_smoke:
  movaps xmm2, [rdi]
  call rcpps_smoke_impl
  movaps [rdi], xmm1
  ret

.global rcpps_smoke_impl
rcpps_smoke_impl:
  rcpps xmm1, xmm2
  mov rax, [rip + deadbeef]
  nop
  nop
  nop
  nop
  nop
  nop
  ret
deadbeef:
  .quad 0xdeadbeefdeadbeef

.global rcpps_smoke_replacement
rcpps_smoke_replacement:
  movaps xmm1, [rip + funny_numbers]
  ret
.p2align 4
funny_numbers:
  .float 420.0
  .float 69.0
  .float -420.0
  .float -69.0

.section .note.GNU-stack, "", %progbits
