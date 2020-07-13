/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2020, Rebecca Cran <rebecca@bsdio.com>
  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosPlatformDxe.h"

#define TYPE0_STRINGS \
  "EFI Development Kit II / OVMF\0"     /* Vendor */ \
  "0.0.0\0"                             /* BiosVersion */ \
  "02/06/2015\0"                        /* BiosReleaseDate */
//
// Type definition and contents of the default Type 0 SMBIOS table.
//
#pragma pack(1)
typedef struct {
  SMBIOS_TABLE_TYPE0 Base;
  UINT8              Strings[sizeof(TYPE0_STRINGS)];
} OVMF_TYPE0;
#pragma pack()

STATIC CONST OVMF_TYPE0 mOvmfDefaultType0 = {
  {
    // SMBIOS_STRUCTURE Hdr
    {
      EFI_SMBIOS_TYPE_BIOS_INFORMATION, // UINT8 Type
      sizeof (SMBIOS_TABLE_TYPE0),      // UINT8 Length
    },
    1,     // SMBIOS_TABLE_STRING       Vendor
    2,     // SMBIOS_TABLE_STRING       BiosVersion
    0xE800,// UINT16                    BiosSegment
    3,     // SMBIOS_TABLE_STRING       BiosReleaseDate
    0,     // UINT8                     BiosSize
    {      // MISC_BIOS_CHARACTERISTICS BiosCharacteristics
      0,     // Reserved                                      :2
      0,     // Unknown                                       :1
      1,     // BiosCharacteristicsNotSupported               :1
             // Remaining BiosCharacteristics bits left unset :60
    },
    {      // BIOSCharacteristicsExtensionBytes[2]
      0,     // BiosReserved
      0x1C   // SystemReserved = VirtualMachineSupported |
             //                  UefiSpecificationSupported |
             //                  TargetContentDistributionEnabled
    },
    0,     // UINT8                     SystemBiosMajorRelease
    0,     // UINT8                     SystemBiosMinorRelease
    0xFF,  // UINT8                     EmbeddedControllerFirmwareMajorRelease
    0xFF   // UINT8                     EmbeddedControllerFirmwareMinorRelease
  },
  // Text strings (unformatted area)
  TYPE0_STRINGS
};


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
  @param  TableAddress         SMBIOS tables starting address

**/
EFI_STATUS
InstallAllStructures (
  IN EFI_SMBIOS_PROTOCOL       *Smbios,
  IN UINT8                     *TableAddress
  )
{
  EFI_STATUS                Status;
  SMBIOS_STRUCTURE_POINTER  SmbiosTable;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  BOOLEAN                   NeedSmbiosType0;

  SmbiosTable.Raw = TableAddress;
  if (SmbiosTable.Raw == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  NeedSmbiosType0 = TRUE;

  while (SmbiosTable.Hdr->Type != 127) {
    //
    // Log the SMBIOS data for this structure
    //
    SmbiosHandle = SmbiosTable.Hdr->Handle;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER*) SmbiosTable.Raw
                       );
    ASSERT_EFI_ERROR (Status);

    if (SmbiosTable.Hdr->Type == 0) {
      NeedSmbiosType0 = FALSE;
    }

    //
    // Get the next structure address
    //
    SmbiosTable.Raw = (UINT8 *)(SmbiosTable.Raw + SmbiosTableLength (SmbiosTable));
  }

  if (NeedSmbiosType0) {
    //
    // Add OVMF default Type 0 (BIOS Information) table
    //
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = Smbios->Add (
                       Smbios,
                       NULL,
                       &SmbiosHandle,
                       (EFI_SMBIOS_TABLE_HEADER*) &mOvmfDefaultType0
                       );
    ASSERT_EFI_ERROR (Status);
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
  UINT8                     *SmbiosTables = NULL;

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
  // Add bhyve SMBIOS data
  //
  EntryPointStructure = GetBhyveSmbiosTables ();
  if (EntryPointStructure != NULL) {
    SmbiosTables = (UINT8*)(UINTN)EntryPointStructure->TableAddress;
  }

  if (SmbiosTables != NULL) {
    Status = InstallAllStructures (Smbios, SmbiosTables);

    //
    // Free SmbiosTables if allocated by Qemu (i.e., NOT by Xen):
    //
    if (EntryPointStructure == NULL) {
      FreePool (SmbiosTables);
    }
  }

  return Status;
}
