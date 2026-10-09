/** @file
  Shared MMIO mapping for UFDT resource consumer constructors.

  Resource-specific DEPEX libraries gate consumers on resource selection and PCD setup,
  then call this helper from their constructors before the driver entry point.
  Constructor errors are returned to the generated constructor list, which
  asserts on failure but does not prevent driver entry in release builds.

  Copyright (c) 2026, Arm Ltd. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Uefi.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/UfdtResource.h>

#include <Library/UfdtDepexLib.h>

/**
  Map the selected resource's MMIO regions, if a mapping callback is present.

  The calling library's DEPEX must require ResourceGuid before its constructor
  calls this function. The resource is a singleton platform selection.
  Call only while boot services are available.

  @param[in] ResourceGuid  Protocol identifying the selected resource.

  @retval EFI_SUCCESS  The regions were mapped or no mapping was required.
  @return              An error returned by LocateProtocol() or MapMmio().
**/
EFI_STATUS
EFIAPI
UfdtDepexMapMmio (
  IN EFI_GUID  *ResourceGuid
  )
{
  UFDT_RESOURCE_PROTOCOL  *Resource;
  EFI_STATUS              Status;

  Status = gBS->LocateProtocol (ResourceGuid, NULL, (VOID **)&Resource);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  if (Resource->MapMmio == NULL) {
    return EFI_SUCCESS;
  }

  return Resource->MapMmio (Resource);
}
