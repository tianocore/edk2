/** @file
  Library to add SMBIOS data records from HOB to SMBIOS table.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

  @par Specification Reference:
  System Management BIOS (SMBIOS) Reference Specification v3.0.0
  dated 2015-Feb-12 (DSP0134)
  http://www.dmtf.org/sites/default/files/standards/documents/DSP0134_3.0.0.pdf

**/
#include <IndustryStandard/SmBios.h>
#include <Library/UefiLib.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Smbios.h>


/**
  Adds SMBIOS records to tables

  @param[in] ImageHandle          Image handle of this driver.
  @param[in] SystemTable          Global system service table.

  @retval EFI_UNSUPPORTED      -  Could not locate SMBIOS protocol
  @retval EFI_OUT_OF_RESOURCES -  Failed to allocate memory for SMBIOS HOB type.
  @retval EFI_SUCCESS          -  Successfully added SMBIOS records based on HOB.
**/
EFI_STATUS
EFIAPI
DxeSmbiosDataHobLibConstructor (
  IN EFI_HANDLE                ImageHandle,
  IN EFI_SYSTEM_TABLE          *SystemTable
  )
{
  EFI_PEI_HOB_POINTERS         Hob;
  EFI_SMBIOS_HANDLE            SmbiosHandle;
  EFI_SMBIOS_PROTOCOL          *Smbios;
  EFI_STATUS                   Status;
  UINT8                        *RecordPtr;
  UINT16                       RecordCount;

  RecordCount = 0;

  DEBUG ((DEBUG_INFO, "Adding SMBIOS records from HOB..\n"));

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (Smbios == NULL) {
    DEBUG ((DEBUG_WARN, "Can't locate SMBIOS protocol\n"));
    return EFI_UNSUPPORTED;
  }

  ///
  /// Get SMBIOS HOB data (each hob contains one SMBIOS record)
  ///
  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if ((GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_GUID_EXTENSION) && (CompareGuid (&Hob.Guid->Name, &gIntelSmbiosDataHobGuid))) {
      RecordPtr = GET_GUID_HOB_DATA (Hob.Raw);

      ///
      /// Add generic SMBIOS HOB to SMBIOS table
      ///
      DEBUG ((DEBUG_VERBOSE, "Add SMBIOS record type: %x\n", ((EFI_SMBIOS_TABLE_HEADER *) RecordPtr)->Type));
      SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
      Status = Smbios->Add (Smbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) RecordPtr);
      if (!EFI_ERROR (Status)) {
        RecordCount++;
      }
    }
  }
  DEBUG ((DEBUG_INFO, "Found %d Records and added to SMBIOS table.\n", RecordCount));

  return EFI_SUCCESS;
}

