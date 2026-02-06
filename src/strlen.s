.intel_syntax noprefix
.global nv_strlen

nv_strlen:
  xor rax,rax
.loop:
  cmp byte ptr [rdi + rax], 0
  je .done
  inc rax
  jmp .loop
.done:
  ret
