/** @file
  System Firmware descriptor producer.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiPei.h>
#include <Library/PcdLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/DebugLib.h>
#include <Protocol/FirmwareManagement.h>
#include <Guid/EdkiiSystemFmpCapsule.h>

/**
  Entrypoint for SystemFirmwareDescriptor PEIM.

  @param[in]  FileHandle  Handle of the file being invoked.
  @param[in]  PeiServices Describes the list of possible PEI Services.

  @retval EFI_SUCCESS            PPI successfully installed.
**/
EFI_STATUS
EFIAPI
SystemFirmwareDescriptorPeimEntry (
  IN EFI_PEI_FILE_HANDLE     FileHandle,
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS                              Status;
  EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR  *Descriptor;
  UINTN                                   Size;
  UINTN                                   Index;
  UINT32                                  AuthenticationStatus;

  //
  // Search RAW section.
  //
  Index = 0;
  while (TRUE) {
    Status = PeiServicesFfsFindSectionData3(EFI_SECTION_RAW, Index, FileHandle, (VOID **)&Descriptor, &AuthenticationStatus);
    if (EFI_ERROR(Status)) {
      // Should not happen, must something wrong in FDF.
      ASSERT(FALSE);
      return EFI_NOT_FOUND;
    }
    if (Descriptor->Signature == EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR_SIGNATURE) {
      break;
    }
    Index++;
  }

  DEBUG((DEBUG_INFO, "EDKII_SYSTEM_FIRMWARE_IMAGE_DESCRIPTOR size - 0x%x\n", Descriptor->Length));

  Size = Descriptor->Length;
  PcdSetPtrS (PcdEdkiiSystemFirmwareImageDescriptor, &Size, Descriptor);

  return EFI_SUCCESS;
}
