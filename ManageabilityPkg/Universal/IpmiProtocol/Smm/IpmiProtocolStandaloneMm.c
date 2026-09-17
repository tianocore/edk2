/** @file
  Standalone MM entry point for the IPMI Protocol driver.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiMm.h>

#include "IpmiProtocolMmCommon.h"

/**
  The entry point of the IPMI Standalone MM driver.

  @param[in] ImageHandle    Handle of this driver image.
  @param[in] MmSystemTable  Pointer to the MM system table.

  @retval EFI_SUCCESS  The IPMI Protocol was installed successfully.
  @retval Others       An error occurred while initializing the driver.
**/
EFI_STATUS
EFIAPI
StandaloneMmIpmiEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_MM_SYSTEM_TABLE  *MmSystemTable
  )
{
  return IpmiProtocolInitialize ();
}
