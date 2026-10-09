/** @file
  Map MMIO for the selected UFDT GICv2 resource before consumer entry.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Protocol/UfdtResourceInfo.h>

#include <Library/UfdtDepexLib.h>

/**
  Map the selected resource's MMIO regions, if required.

  @param[in] ImageHandle  Handle of the consuming image.
  @param[in] SystemTable  Pointer to the EFI system table.

  @retval EFI_SUCCESS  The regions were mapped or no mapping was required.
  @return              An error returned by LocateProtocol() or MapMmio().
**/
EFI_STATUS
EFIAPI
UfdtGicV2DepexLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  return UfdtDepexMapMmio (&gEdkiiUfdtGicV2InfoProtocolGuid);
}
