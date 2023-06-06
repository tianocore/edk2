/** @file
  OVMF ACPI support

  Copyright (C) 2021, Red Hat, Inc.
  Copyright (c) 2008 - 2012, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2012, Bei Guan <gbtju85@gmail.com>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/AcpiPlatformLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

EFI_STATUS
EFIAPI
GetAcpiRsdpFromMemory (
  IN UINTN                                            StartAddress,
  IN UINTN                                            EndAddress,
  OUT   EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  **RsdpPtr
  )
{
  EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER  *RsdpStructurePtr;
  UINT8                                         *AcpiPtr;
  UINT8                                         Sum;

  for (AcpiPtr = (UINT8 *)StartAddress;
       AcpiPtr < (UINT8 *)EndAddress;
       AcpiPtr += 0x10)
  {
    RsdpStructurePtr = (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER *)
                       (UINTN)AcpiPtr;

    if (!AsciiStrnCmp ((CHAR8 *)&RsdpStructurePtr->Signature, "RSD PTR ", 8)) {
      //
      // RSDP ACPI 1.0 checksum for 1.0/2.0/3.0 table.
      // This is only the first 20 bytes of the structure
      //
      Sum = CalculateSum8 (
              (CONST UINT8 *)RsdpStructurePtr,
              sizeof (EFI_ACPI_1_0_ROOT_SYSTEM_DESCRIPTION_POINTER)
              );
      if (Sum != 0) {
        return EFI_ABORTED;
      }

      if (RsdpStructurePtr->Revision >= 2) {
        //
        // RSDP ACPI 2.0/3.0 checksum, this is the entire table
        //
        Sum = CalculateSum8 (
                (CONST UINT8 *)RsdpStructurePtr,
                sizeof (EFI_ACPI_2_0_ROOT_SYSTEM_DESCRIPTION_POINTER)
                );
        if (Sum != 0) {
          return EFI_ABORTED;
        }
      }

      *RsdpPtr = RsdpStructurePtr;
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}
