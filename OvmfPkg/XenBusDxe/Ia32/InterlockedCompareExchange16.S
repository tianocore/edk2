# UINT16
# EFIAPI
# InternalSyncCompareExchange16 (
#   IN      volatile UINT16           *Value,
#   IN      UINT16                    CompareValue,
#   IN      UINT16                    ExchangeValue
#   );
ASM_GLOBAL ASM_PFX(InternalSyncCompareExchange16)
ASM_PFX(InternalSyncCompareExchange16):
  mov 4(%esp), %ecx
  mov 8(%esp), %eax
  mov 12(%esp), %edx
  lock
  cmpxchgw %dx, (%ecx)
  ret
