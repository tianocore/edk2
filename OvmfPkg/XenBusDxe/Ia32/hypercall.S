# INTN
# EFIAPI
# XenHypercall2 (
#   IN     VOID *HypercallAddr,
#   IN OUT INTN Arg1,
#   IN OUT INTN Arg2
#   );
ASM_GLOBAL ASM_PFX(XenHypercall2)
ASM_PFX(XenHypercall2):
  # Save only ebx, ecx is supposed to be a scratch register and needs to be
  # saved by the caller
  push %ebx
  # Copy HypercallAddr to eax
  mov 8(%esp), %eax
  # Copy Arg1 to the register expected by Xen
  mov 12(%esp), %ebx
  # Copy Arg2 to the register expected by Xen
  mov 16(%esp), %ecx
  # Call HypercallAddr
  call *%eax
  pop %ebx
  ret
