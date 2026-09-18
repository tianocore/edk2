/** @file
  Traditional MM entry point for the IPMI Protocol driver.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <PiSmm.h>

#include "IpmiProtocolMmCommon.h"

/**
  The entry point of the IPMI Traditional MM driver.

  @param[in] ImageHandle  Handle of this driver image.
  @param[in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS  The IPMI Protocol was installed successfully.
  @retval Others       An error occurred while initializing the driver.
**/
EFI_STATUS
EFIAPI
SmmIpmiEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return IpmiProtocolInitialize ();
}
