.intel_syntax noprefix

.section .text

.global integration
integration:
  // We enter with a stack aligned to 8 % 16, so forcibly align it.
  push rbp

  movaps xmm0, [rdi + 0]
  movaps xmm1, [rdi + 16]
  movaps xmm2, [rdi + 32]

  sub rsp, 16
  movups [rsp], xmm1
  call integration_impl
  add rsp, 16

  movaps [rdi + 0], xmm0
  movaps [rdi + 16], xmm1
  movaps [rdi + 32], xmm2

  pop rbp
  ret

.global integration_impl
integration_impl:
  rcpps xmm0, [rdi]
  rsqrtps xmm1, [rsp + 8]
  rsqrtss xmm2, xmm2
  ret

.section .note.GNU-stack, "", %progbits
