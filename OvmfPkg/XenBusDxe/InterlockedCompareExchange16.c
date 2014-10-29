#include <Library/DebugLib.h>
#include "InterlockedCompareExchange16.h"

/**
  Performs an atomic compare exchange operation on a 16-bit unsigned integer.

  Performs an atomic compare exchange operation on the 16-bit unsigned integer
  specified by Value.  If Value is equal to CompareValue, then Value is set to
  ExchangeValue and CompareValue is returned.  If Value is not equal to CompareValue,
  then Value is returned.  The compare exchange operation must be performed using
  MP safe mechanisms.

  If Value is NULL, then ASSERT().

  @param  Value         A pointer to the 16-bit value for the compare exchange
                        operation.
  @param  CompareValue  16-bit value used in compare operation.
  @param  ExchangeValue 16-bit value used in exchange operation.

  @return The original *Value before exchange.

**/
UINT16
EFIAPI
InterlockedCompareExchange16 (
  IN OUT  UINT16                    *Value,
  IN      UINT16                    CompareValue,
  IN      UINT16                    ExchangeValue
  )
{
  ASSERT (Value != NULL);
  return InternalSyncCompareExchange16 (Value, CompareValue, ExchangeValue);
}
