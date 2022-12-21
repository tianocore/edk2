/** @file
  This driver installs SMBIOS information for OVMF

  Copyright (c) 2011, Bei Guan <gbtju85@gmail.com>
  Copyright (c) 2011 - 2015, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <IndustryStandard/SmBios.h>          // SMBIOS_TABLE_TYPE0
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>                 // ASSERT_EFI_ERROR()
#include <Library/MemoryAllocationLib.h>
#include <Library/PcdLib.h>
#include <Library/UefiBootServicesTableLib.h> // gBS
#include <Protocol/Smbios.h>                  // EFI_SMBIOS_PROTOCOL

#include "SmbiosPlatformDxe.h"

STATIC CONST SMBIOS_TABLE_TYPE0  mOvmfDefaultType0 = {
  // SMBIOS_STRUCTURE Hdr
  {
    EFI_SMBIOS_TYPE_BIOS_INFORMATION, // UINT8 Type
    sizeof (SMBIOS_TABLE_TYPE0),      // UINT8 Length
  },
  1,      // SMBIOS_TABLE_STRING       Vendor
  2,      // SMBIOS_TABLE_STRING       BiosVersion
  0xE800, // UINT16                    BiosSegment
  3,      // SMBIOS_TABLE_STRING       BiosReleaseDate
  0,      // UINT8                     BiosSize
  {      // MISC_BIOS_CHARACTERISTICS BiosCharacteristics
    0,   // Reserved                                      :2
    0,   // Unknown                                       :1
    1,   // BiosCharacteristicsNotSupported               :1
    // Remaining BiosCharacteristics bits left unset :60
  },
  {      // BIOSCharacteristicsExtensionBytes[2]
    0,   // BiosReserved
    0x1C // SystemReserved = VirtualMachineSupported |
    //                  UefiSpecificationSupported |
    //                  TargetContentDistributionEnabled
  },
  0,     // UINT8                     SystemBiosMajorRelease
  0,     // UINT8                     SystemBiosMinorRelease
  0xFF,  // UINT8                     EmbeddedControllerFirmwareMajorRelease
  0xFF   // UINT8                     EmbeddedControllerFirmwareMinorRelease
};

/**
  Get SMBIOS record length.

  @param  SmbiosTable   SMBIOS pointer.

**/
UINTN
SmbiosTableLength (
  IN SMBIOS_STRUCTURE_POINTER  SmbiosTable
  )
{
  CHAR8  *AChar;
  UINTN  Length;

  AChar = (CHAR8 *)(SmbiosTable.Raw + SmbiosTable.Hdr->Length);

  //
  // Each structure shall be terminated by a double-null (SMBIOS spec.7.1)
  //
  while ((*AChar != 0) || (*(AChar + 1) != 0)) {
    AChar++;
  }

  Length = ((UINTN)AChar - (UINTN)SmbiosTable.Raw + 2);

  return Length;
}

/**
  Install all structures from the given SMBIOS structures block

  @param  TableAddress         SMBIOS tables starting address

**/
EFI_STATUS
InstallAllStructures (
  IN UINT8  *TableAddress
  )
{
  EFI_SMBIOS_PROTOCOL       *Smbios;
  EFI_STATUS                Status;
  SMBIOS_STRUCTURE_POINTER  SmbiosTable;
  EFI_SMBIOS_HANDLE         SmbiosHandle;
  BOOLEAN                   NeedSmbiosType0;

  //
  // Find the SMBIOS protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiSmbiosProtocolGuid,
                  NULL,
                  (VOID **)&Smbios
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

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
    Status       = Smbios->Add (
                             Smbios,
                             NULL,
                             &SmbiosHandle,
                             (EFI_SMBIOS_TABLE_HEADER *)SmbiosTable.Raw
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
    CHAR16  *VendStr, *VersStr, *DateStr;
    UINTN   VendLen, VersLen, DateLen;
    CHAR8   *Type0;

    VendStr = (CHAR16 *)FixedPcdGetPtr (PcdFirmwareVendor);
    VendLen = StrLen (VendStr);
    if (VendLen < 3) {
      VendStr = L"unknown";
      VendLen = StrLen (VendStr);
    }

    VersStr = (CHAR16 *)FixedPcdGetPtr (PcdFirmwareVersionString);
    VersLen = StrLen (VersStr);
    if (VersLen < 3) {
      VersStr = L"unknown";
      VersLen = StrLen (VersStr);
    }

    DateStr = (CHAR16 *)FixedPcdGetPtr (PcdFirmwareReleaseDateString);
    DateLen = StrLen (DateStr);
    if (DateLen < 3) {
      DateStr = L"unknown";
      DateLen = StrLen (DateStr);
    }

    DEBUG ((DEBUG_INFO, "FirmwareVendor:            \"%s\" (%d chars)\n", VendStr, VendLen));
    DEBUG ((DEBUG_INFO, "FirmwareVersionString:     \"%s\" (%d chars)\n", VersStr, VersLen));
    DEBUG ((DEBUG_INFO, "FirmwareReleaseDateString: \"%s\" (%d chars)\n", DateStr, DateLen));

    Type0 = AllocateZeroPool (sizeof (mOvmfDefaultType0) + VendLen + VersLen + DateLen + 4);
    if (Type0 == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    CopyMem (Type0, &mOvmfDefaultType0, sizeof (mOvmfDefaultType0));
    UnicodeStrToAsciiStrS (VendStr, Type0 + sizeof (mOvmfDefaultType0), VendLen + 1);
    UnicodeStrToAsciiStrS (VersStr, Type0 + sizeof (mOvmfDefaultType0) + VendLen + 1, VersLen + 1);
    UnicodeStrToAsciiStrS (DateStr, Type0 + sizeof (mOvmfDefaultType0) + VendLen + VersLen + 2, DateLen + 1);

    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status       = Smbios->Add (
                             Smbios,
                             NULL,
                             &SmbiosHandle,
                             (EFI_SMBIOS_TABLE_HEADER *)Type0
                             );
    ASSERT_EFI_ERROR (Status);

    FreePool (Type0);
  }

  return EFI_SUCCESS;
}
