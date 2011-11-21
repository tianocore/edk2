/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "SmbiosPlatformDxe.h"


/**
  Validates the SMBIOS entry point structure

  @param  EntryPointStructure  SMBIOS entry point structure

  @retval TRUE   The entry point structure is valid
  @retval FALSE  The entry point structure is not valid

**/
BOOLEAN
IsEntryPointStructureValid (
  IN SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure
  )
{
  UINTN                     Index;
  UINT8                     Length;
  UINT8                     Checksum;
  UINT8                     *BytePtr;

  BytePtr = (UINT8*) EntryPointStructure;
  Length = EntryPointStructure->EntryPointLength;
  Checksum = 0;

  for (Index = 0; Index < Length; Index++) {
    Checksum = Checksum + (UINT8) BytePtr[Index];
  }

  if (Checksum != 0) {
    return FALSE;
  } else {
    return TRUE;
  }
}


/**
  Get SMBIOS record length.

  @param  SmbiosTable   SMBIOS pointer.

**/
UINTN
SmbiosTableLength (
  IN SMBIOS_STRUCTURE_POINTER SmbiosTable
  )
{
  CHAR8  *AChar;
  UINTN  Length;

  AChar = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Hdr->Length);

  //
  // Each structure shall be terminated by a double-null (SMBIOS spec.7.1)
  //
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar ++;
  }
  Length = ((UINTN)AChar - (UINTN)SmbiosTable.Raw + 2);

  return Length;
}


/**
  Install all structures from the given SMBIOS structures block

  @param  Smbios               SMBIOS protocol
  @param  EntryPointStructure  SMBIOS entry point structures block

**/
EFI_STATUS
InstallAllStructures (
  IN EFI_SMBIOS_PROTOCOL       *Smbios,
  IN SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure
  )
{
  EFI_STATUS                Status;
  SMBIOS_STRUCTURE_POINTER  SmbiosTable;
  EFI_SMBIOS_HANDLE         SmbiosHandle;

  SmbiosTable.Raw = (UINT8*)(UINTN) EntryPointStructure->TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  while (SmbiosTable.Hdr->Type != 127) {
    //
    // Log the SMBIOS data for this structure
    //
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER*) SmbiosTable.Raw
                       );
    ASSERT_EFI_ERROR (Status);

    //
    // Get the next structure address
    //
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosTableLength (SmbiosTable));
  }

  return EFI_SUCCESS;
}


/**
  Installs SMBIOS information for OVMF

  @param ImageHandle     Module's image handle
  @param SystemTable     Pointer of EFI_SYSTEM_TABLE

  @retval EFI_SUCCESS    Smbios data successfully installed
  @retval Other          Smbios data was not installed

**/
EFI_STATUS
EFIAPI
SmbiosTablePublishEntry (
  IN EFI_HANDLE           ImageHandle,
  IN EFI_SYSTEM_TABLE     *SystemTable
  )
{
  EFI_STATUS                Status;
  EFI_SMBIOS_PROTOCOL       *Smbios;
  SMBIOS_TABLE_ENTRY_POINT  *EntryPointStructure;

  //
  // Find the SMBIOS protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID**)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Add Xen SMBIOS data if found
  //
  EntryPointStructure = GetXenSmbiosTables ();
  if (EntryPointStructure != NULL) {
    Status = InstallAllStructures (Smbios, EntryPointStructure);
  }

  return Status;
}
