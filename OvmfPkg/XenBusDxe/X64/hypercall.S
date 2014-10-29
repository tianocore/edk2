# INTN
# EFIAPI
# XenHypercall2 (
#   IN     VOID *HypercallAddr,
#   IN OUT INTN Arg1,
#   IN OUT INTN Arg2
#   );
ASM_GLOBAL ASM_PFX(XenHypercall2)
ASM_PFX(XenHypercall2):
  push %rdi
  push %rsi
  # Copy HypercallAddr to rax
  movq %rcx, %rax
  # Copy Arg1 to the register expected by Xen
  movq %rdx, %rdi
  # Copy Arg2 to the register expected by Xen
  movq %r8, %rsi
  # Call HypercallAddr
  call *%rax
  pop %rsi
  pop %rdi
  ret
