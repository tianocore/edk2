# INT32
# EFIAPI
# TestAndClearBit (
#   IN  INT32 Bit,                // rcx
#   IN  volatile VOID* Address    // rdx
#   );
ASM_GLOBAL ASM_PFX(TestAndClearBit)
ASM_PFX(TestAndClearBit):
  lock
  btrl %ecx, (%rdx)
  sbbl %eax, %eax
  ret
