/** @file
  Library to add SMBIOS data records from HOB to SMBIOS table.

  Copyright (c) 2016, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License which accompanies this distribution.
  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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

  Get the full size of SMBIOS structure including optional strings that follow the formatted structure.
  @note: This function is copy from SmbiosDxe in MdeModulePkg.

  @param[in] This                 The EFI_SMBIOS_PROTOCOL instance.
  @param[in] Head                 Pointer to the beginning of SMBIOS structure.
  @param[out] Size                The returned size.
  @param[out] NumberOfStrings     The returned number of optional strings that follow the formatted structure.

  @retval EFI_SUCCESS           Size returned in Size.
  @retval EFI_INVALID_PARAMETER Input SMBIOS structure mal-formed or Size is NULL.

**/
EFI_STATUS
EFIAPI
GetSmbiosStructureSize (
  IN   CONST EFI_SMBIOS_PROTOCOL        *This,
  IN   EFI_SMBIOS_TABLE_HEADER          *Head,
  OUT  UINTN                            *Size,
  OUT  UINTN                            *NumberOfStrings
  )
{
  UINTN  FullSize;
  UINTN  StrLen;
  UINTN  MaxLen;
  INT8*  CharInStr;

  if (Size == NULL || NumberOfStrings == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  FullSize = Head->Length;
  CharInStr = (INT8*)Head + Head->Length;
  *Size = FullSize;
  *NumberOfStrings = 0;
  StrLen = 0;

  //
  // look for the two consecutive zeros, check the string limit by the way.
  //
  while (*CharInStr != 0 || *(CharInStr+1) != 0) {
    if (*CharInStr == 0) {
      *Size += 1;
      CharInStr++;
    }

    if (This->MajorVersion < 2 || (This->MajorVersion == 2 && This->MinorVersion < 7)) {
      MaxLen = SMBIOS_STRING_MAX_LENGTH;
    } else if (This->MajorVersion < 3) {
      //
      // Reference SMBIOS 2.7, chapter 6.1.3, it will have no limit on the length of each individual text string.
      // However, the length of the entire structure table (including all strings) must be reported
      // in the Structure Table Length field of the SMBIOS Structure Table Entry Point,
      // which is a WORD field limited to 65,535 bytes.
      //
      MaxLen = SMBIOS_TABLE_MAX_LENGTH;
    } else {
      //
      // SMBIOS 3.0 defines the Structure table maximum size as DWORD field limited to 0xFFFFFFFF bytes.
      // Locate the end of string as long as possible.
      //
      MaxLen = SMBIOS_3_0_TABLE_MAX_LENGTH;
    }

    for (StrLen = 0 ; StrLen < MaxLen; StrLen++) {
      if (*(CharInStr+StrLen) == 0) {
        break;
      }
    }

    if (StrLen == MaxLen) {
      return EFI_INVALID_PARAMETER;
    }

    //
    // forward the pointer
    //
    CharInStr += StrLen;
    *Size += StrLen;
    *NumberOfStrings += 1;
  }

  //
  // count ending two zeros.
  //
  *Size += 2;
  return EFI_SUCCESS;
}

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
  UINTN                        InstalledPayloadSize;
  UINTN                        MaxPayloadSize;
  UINT8                        *RecordPtr;
  UINT16                       RecordCount;
  UINTN                        StructureSize;
  UINTN                        NumberOfStrings;

  RecordCount = 0;

  DEBUG ((DEBUG_INFO, "Adding SMBIOS records from HOB..\n"));

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&Smbios);
  if (Smbios == NULL) {
    DEBUG ((DEBUG_WARN, "  Can't locate SMBIOS protocol\n"));
    return EFI_UNSUPPORTED;
  }

  ///
  /// Get SMBIOS HOB data
  ///
  for (Hob.Raw = GetHobList (); !END_OF_HOB_LIST(Hob); Hob.Raw = GET_NEXT_HOB (Hob)) {
    if ((GET_HOB_TYPE (Hob) == EFI_HOB_TYPE_GUID_EXTENSION) && (CompareGuid (&Hob.Guid->Name, &gIntelSmbiosDataHobGuid))) {
      RecordPtr = (UINT8 *)Hob.Raw + sizeof (EFI_HOB_GUID_TYPE);
      MaxPayloadSize = Hob.Guid->Header.HobLength - sizeof (EFI_HOB_GUID_TYPE);

      InstalledPayloadSize = 0;
      do {
        StructureSize = 0;
        Status = GetSmbiosStructureSize (Smbios, (EFI_SMBIOS_TABLE_HEADER *)RecordPtr, &StructureSize, &NumberOfStrings);
        if ((Status == EFI_SUCCESS) && (InstalledPayloadSize + StructureSize <= MaxPayloadSize)) {
          InstalledPayloadSize += StructureSize;

          ///
          /// Add generic SMBIOS HOB to SMBIOS table
          ///
          DEBUG ((DEBUG_VERBOSE, "  Add SMBIOS record type: %x\n", ((EFI_SMBIOS_TABLE_HEADER *) RecordPtr)->Type));
          SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
          Status = Smbios->Add (Smbios, NULL, &SmbiosHandle, (EFI_SMBIOS_TABLE_HEADER *) RecordPtr);
          if (!EFI_ERROR (Status)) {
            RecordPtr += StructureSize;
            RecordCount++;
          }
        } else {
          break;
        }
      } while (TRUE);
    }
  }
  DEBUG ((DEBUG_INFO, "  Found %d Records and added to SMBIOS table.\n", RecordCount));

  return EFI_SUCCESS;
}

