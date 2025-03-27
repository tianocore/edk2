/** @file
  System Firmware descriptor producer.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Ventana Micro Systems Inc. All Rights Reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Pi/PiFirmwareFile.h>
#include <Guid/EdkiiSystemFmpCapsule.h>
#include <Library/DebugLib.h>
#include <Library/PcdLib.h>
#include <Library/DxeServicesLib.h>
#include <Protocol/FirmwareManagement.h>

/**
  Entrypoint for SystemFirmwareDescriptor DXE.

  @param[in] ImageHandle      The image handle of the driver.
  @param[in] SystemTable      The system table.

  @retval EFI_SUCCESS         Firmware description set succesfully.
**/
EFI_STATUS
EFIAPI
SystemFirmwareDescriptorDxeEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS                              Status;
  EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR  *Descriptor;
  UINTN                                   Size;
  UINTN                                   Index;

  //
  // Search RAW section.
  //
  Index = 0;
  while (TRUE) {
    Status = GetSectionFromFv (
               &gEfiCallerIdGuid,
               EFI_SECTION_RAW,
               Index,
               (VOID **)&Descriptor,
               &Size
               );
    if (EFI_ERROR (Status)) {
      // Should not happen, must something wrong in FDF.
      ASSERT (FALSE);
      return EFI_NOT_FOUND;
    }

    if (Descriptor->Signature == EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR_SIGNATURE) {
      break;
    }

    Index++;
  }

  DEBUG ((
    DEBUG_INFO,
    "EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR size - 0x%x\n",
    Descriptor->Length
    ));

  Size = Descriptor->Length;
  PcdSetPtrS (PcdEdkiiSystemFirmwareImageDescriptor, &Size, Descriptor);

  return EFI_SUCCESS;
}
