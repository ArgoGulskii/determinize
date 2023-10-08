.intel_syntax noprefix

.section .text

.global jmp_rel8
.global jmp_rel8_impl
jmp_rel8:
jmp_rel8_impl:
  rcpps xmm0, [rdi]
  jmp 1f
  int 3
1:
  movaps [rdi], xmm0
  mov rax, rsi
  ret

.global je_rel8
je_rel8:
  cmp rsi, 1

.global je_rel8_impl
je_rel8_impl:
  rcpps xmm0, [rdi]
  je 1f
  jmp 2f
  int 3
1:
  mov rax, 1
  movaps [rdi], xmm0
  ret
2:
  mov rax, 0
  movaps [rdi], xmm0
  ret

.section .note.GNU-stack, "", %progbits
