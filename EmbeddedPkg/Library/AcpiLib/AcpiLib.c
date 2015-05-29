/** @file
*
*  Copyright (c) 2014-2015, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#include <Uefi.h>

#include <Library/AcpiLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/AcpiTable.h>
#include <Protocol/FirmwareVolume2.h>

#include <IndustryStandard/Acpi.h>

/**
  Locate and Install the ACPI tables from the Firmware Volume if it verifies
  the function condition.

  @param  AcpiFile                Guid of the ACPI file into the Firmware Volume
  @param  CheckAcpiTableFunction  Function that checks if the ACPI table should be installed

  @return EFI_SUCCESS             The function completed successfully.
  @return EFI_NOT_FOUND           The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES    There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFvConditional (
  IN CONST EFI_GUID*        AcpiFile,
  IN EFI_LOCATE_ACPI_CHECK  CheckAcpiTableFunction
  )
{
  EFI_STATUS                    Status;
  EFI_ACPI_TABLE_PROTOCOL       *AcpiProtocol;
  EFI_HANDLE                    *HandleBuffer;
  UINTN                         NumberOfHandles;
  UINT32                        FvStatus;
  UINTN                         Index;
  EFI_FIRMWARE_VOLUME2_PROTOCOL *FvInstance;
  INTN                          SectionInstance;
  UINTN                         SectionSize;
  EFI_ACPI_COMMON_HEADER       *AcpiTable;
  UINTN                         AcpiTableSize;
  UINTN                         AcpiTableKey;
  BOOLEAN                       Valid;

  // Ensure the ACPI Table is present
  Status = gBS->LocateProtocol (
                  &gEfiAcpiTableProtocolGuid,
                  NULL,
                  (VOID**)&AcpiProtocol
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FvStatus        = 0;
  SectionInstance = 0;

  // Locate all the Firmware Volume protocols.
  Status = gBS->LocateHandleBuffer (
                   ByProtocol,
                   &gEfiFirmwareVolume2ProtocolGuid,
                   NULL,
                   &NumberOfHandles,
                   &HandleBuffer
                   );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  // Looking for FV with ACPI storage file
  for (Index = 0; Index < NumberOfHandles; Index++) {
    //
    // Get the protocol on this handle
    // This should not fail because of LocateHandleBuffer
    //
    Status = gBS->HandleProtocol (
                     HandleBuffer[Index],
                     &gEfiFirmwareVolume2ProtocolGuid,
                     (VOID**) &FvInstance
                     );
    if (EFI_ERROR (Status)) {
      goto FREE_HANDLE_BUFFER;
    }

    while (Status == EFI_SUCCESS) {
      // AcpiTable must be allocated by ReadSection (ie: AcpiTable == NULL)
      AcpiTable = NULL;

      // See if it has the ACPI storage file
      Status = FvInstance->ReadSection (
                        FvInstance,
                        AcpiFile,
                        EFI_SECTION_RAW,
                        SectionInstance,
                        (VOID**) &AcpiTable,
                        &SectionSize,
                        &FvStatus
                        );
      if (!EFI_ERROR (Status)) {
        AcpiTableKey = 0;
        AcpiTableSize = ((EFI_ACPI_DESCRIPTION_HEADER *) AcpiTable)->Length;
        ASSERT (SectionSize >= AcpiTableSize);

        DEBUG ((EFI_D_ERROR, "- Found '%c%c%c%c' ACPI Table\n",
            (((EFI_ACPI_DESCRIPTION_HEADER *) AcpiTable)->Signature & 0xFF),
            ((((EFI_ACPI_DESCRIPTION_HEADER *) AcpiTable)->Signature >> 8) & 0xFF),
            ((((EFI_ACPI_DESCRIPTION_HEADER *) AcpiTable)->Signature >> 16) & 0xFF),
            ((((EFI_ACPI_DESCRIPTION_HEADER *) AcpiTable)->Signature >> 24) & 0xFF)));

        // Is the ACPI table valid?
        if (CheckAcpiTableFunction) {
          Valid = CheckAcpiTableFunction ((EFI_ACPI_DESCRIPTION_HEADER *)AcpiTable);
        } else {
          Valid = TRUE;
        }

        // Install the ACPI Table
        if (Valid) {
          Status = AcpiProtocol->InstallAcpiTable (
                                 AcpiProtocol,
                                 AcpiTable,
                                 AcpiTableSize,
                                 &AcpiTableKey
                                 );
        }

        // Free memory allocated by ReadSection
        gBS->FreePool (AcpiTable);

        if (EFI_ERROR (Status)) {
          break;
        }

        // Increment the section instance
        SectionInstance++;
      }
    }
  }

FREE_HANDLE_BUFFER:
  //
  // Free any allocated buffers
  //
  gBS->FreePool (HandleBuffer);

  return EFI_SUCCESS;
}

/**
  Locate and Install the ACPI tables from the Firmware Volume

  @param  AcpiFile              Guid of the ACPI file into the Firmware Volume

  @return EFI_SUCCESS           The function completed successfully.
  @return EFI_NOT_FOUND         The protocol could not be located.
  @return EFI_OUT_OF_RESOURCES  There are not enough resources to find the protocol.

**/
EFI_STATUS
LocateAndInstallAcpiFromFv (
  IN CONST EFI_GUID* AcpiFile
  )
{
  return LocateAndInstallAcpiFromFvConditional (AcpiFile, NULL);
}
