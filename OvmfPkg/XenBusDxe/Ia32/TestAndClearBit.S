# INT32
# EFIAPI
# TestAndClearBit (
#   IN  INT32 Bit,
#   IN  volatile VOID* Address
#   );
ASM_GLOBAL ASM_PFX(TestAndClearBit)
ASM_PFX(TestAndClearBit):
  mov 4(%esp), %ecx
  mov 8(%esp), %edx
  lock btrl %ecx, (%edx)
  sbbl %eax, %eax
  ret
