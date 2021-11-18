/** @file
  NULL RpmcLib instance for build purpose.

Copyright (c) 2020, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/DebugLib.h>
#include <Library/RpmcLib.h>

/**
  Requests the monotonic counter from the designated RPMC counter.

  @param[out]   CounterValue            A pointer to a buffer to store the RPMC value.

  @retval       EFI_SUCCESS             The operation completed successfully.
  @retval       EFI_DEVICE_ERROR        A device error occurred while attempting to update the counter.
  @retval       EFI_UNSUPPORTED         The operation is un-supported.
**/
EFI_STATUS
EFIAPI
RequestMonotonicCounter (
  OUT UINT32  *CounterValue
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Increments the monotonic counter in the SPI flash device by 1.

  @retval       EFI_SUCCESS             The operation completed successfully.
  @retval       EFI_DEVICE_ERROR        A device error occurred while attempting to update the counter.
  @retval       EFI_UNSUPPORTED         The operation is un-supported.
**/
EFI_STATUS
EFIAPI
IncrementMonotonicCounter (
  VOID
  )
{
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}
