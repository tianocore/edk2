/** @file
  This driver will install SMBIOS tables provided by bootloader.

  Copyright (c) 2014 - 2021, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "BlSupportDxe.h"
#include <Library/MemoryAllocationLib.h>


/**

  Acquire the string associated with the Index from smbios structure and return it.
  The caller is responsible for free the string buffer.

  @param    OptionalStrStart  The start position to search the string
  @param    Index             The index of the string to extract
  @param    String            The string that is extracted

  @retval   EFI_SUCCESS       The function returns EFI_SUCCESS always.

**/
EFI_STATUS
GetOptionalStringByIndex (
  IN      CHAR8                   *OptionalStrStart,
  IN      UINT8                   Index,
  OUT     CHAR16                  **String
  )
{
  UINTN          StrSize;

  if (Index == 0) {
    *String = AllocateZeroPool (sizeof (CHAR16));
    return EFI_SUCCESS;
  }

  StrSize = 0;
  do {
    Index--;
    OptionalStrStart += StrSize;
    StrSize           = AsciiStrSize (OptionalStrStart);
  } while (OptionalStrStart[StrSize] != 0 && Index != 0);

  if ((Index != 0) || (StrSize == 1)) {
    //
    // Meet the end of strings set but Index is non-zero, or
    // Find an empty string
    //
    *String = L"";
  } else {
    *String = AllocatePool (StrSize * sizeof (CHAR16));
    AsciiStrToUnicodeStrS (OptionalStrStart, *String, StrSize);
  }

  return EFI_SUCCESS;
}


/**
  Returns the mainboard name.

  @param[out] Name              The mainboard name.
  @param[out] Manufacturer      The mainboard manufacturer name.

  @retval EFI_SUCCESS           The tables could successfully be installed.
  @retval other                 Some error occurs when installing SMBIOS tables.

**/
STATIC
EFI_STATUS
EFIAPI
BlDxeGetMainboardName(
  OUT CHAR16 **Name,
  OUT CHAR16 **Manufacturer
)
{
  EFI_STATUS                    Status;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  EFI_SMBIOS_PROTOCOL           *SmbiosProto;
  EFI_SMBIOS_TABLE_HEADER       *Record;
  SMBIOS_TABLE_TYPE1            *Type1Record;
  UINT8                         StrIndex;

  //
  // Locate Smbios protocol.
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&SmbiosProto);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate gEfiSmbiosProtocolGuid\n",
      __FUNCTION__));
    return Status;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = SmbiosProto->GetNext (SmbiosProto, &SmbiosHandle, NULL, &Record, NULL);
  while (!EFI_ERROR(Status)) {

    if (Record->Type == SMBIOS_TYPE_SYSTEM_INFORMATION) {
      Type1Record = (SMBIOS_TABLE_TYPE1 *) Record;
      StrIndex = Type1Record->ProductName;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type1Record + Type1Record->Hdr.Length), StrIndex, Name);
      StrIndex = Type1Record->Manufacturer;
      GetOptionalStringByIndex ((CHAR8*)((UINT8*)Type1Record + Type1Record->Hdr.Length), StrIndex, Manufacturer);
      return EFI_SUCCESS;
    }
    Status = SmbiosProto->GetNext (SmbiosProto, &SmbiosHandle, NULL, &Record, NULL);
  }

  return EFI_NOT_FOUND;
}

/**
  Install bootloader provided SMBIOS tables.

  @param[in] SmbiosTableBase    The address of the SMBIOS tables in memory.
  @param[in] SmbiosTableSize    The size of the SMBIOS tables in memory.

  @retval EFI_SUCCESS           The tables could successfully be installed.
  @retval other                 Some error occurs when installing SMBIOS tables.

**/
STATIC
EFI_STATUS
EFIAPI
BlDxeInstallSmbiosTables(
  IN UINT64    SmbiosTableBase,
  IN UINT32    SmbiosTableSize
)
{
  EFI_STATUS                    Status;
  SMBIOS_TABLE_ENTRY_POINT      *SmbiosTable;
  SMBIOS_TABLE_3_0_ENTRY_POINT  *Smbios30Table;
  SMBIOS_STRUCTURE_POINTER      Smbios;
  SMBIOS_STRUCTURE_POINTER      SmbiosEnd;
  CHAR8                         *String;
  EFI_SMBIOS_HANDLE             SmbiosHandle;
  EFI_SMBIOS_PROTOCOL           *SmbiosProto;

  //
  // Locate Smbios protocol.
  //
  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID **)&SmbiosProto);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "%a: Failed to locate gEfiSmbiosProtocolGuid\n",
      __FUNCTION__));
    return Status;
  }

  Smbios30Table = (SMBIOS_TABLE_3_0_ENTRY_POINT *)(UINTN)(SmbiosTableBase);
  SmbiosTable = (SMBIOS_TABLE_ENTRY_POINT *)(UINTN)(SmbiosTableBase);

  if (CompareMem (Smbios30Table->AnchorString, "_SM3_", 5) == 0) {
    Smbios.Hdr = (SMBIOS_STRUCTURE *) (UINTN) Smbios30Table->TableAddress;
    SmbiosEnd.Raw = (UINT8 *) (UINTN) (Smbios30Table->TableAddress + Smbios30Table->TableMaximumSize);
    if (Smbios30Table->TableMaximumSize > SmbiosTableSize) {
      DEBUG((DEBUG_INFO, "%a: SMBIOS table size greater than reported by bootloader\n",
        __FUNCTION__));
    }
  } else if (CompareMem (SmbiosTable->AnchorString, "_SM_", 4) == 0) {
    Smbios.Hdr    = (SMBIOS_STRUCTURE *) (UINTN) SmbiosTable->TableAddress;
    SmbiosEnd.Raw = (UINT8 *) ((UINTN) SmbiosTable->TableAddress + SmbiosTable->TableLength);

    if (SmbiosTable->TableLength > SmbiosTableSize) {
      DEBUG((DEBUG_INFO, "%a: SMBIOS table size greater than reported by bootloader\n",
        __FUNCTION__));
    }
  } else {
    DEBUG ((DEBUG_ERROR, "%a: No valid SMBIOS table found\n", __FUNCTION__ ));
    return EFI_NOT_FOUND;
  }

  do {
    // Check for end marker
    if (Smbios.Hdr->Type == 127) {
      break;
    }

    // Install the table
    SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
    Status = SmbiosProto->Add (
                          SmbiosProto,
                          gImageHandle,
                          &SmbiosHandle,
                          Smbios.Hdr
                          );
    ASSERT_EFI_ERROR (Status);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    //
    // Go to the next SMBIOS structure. Each SMBIOS structure may include 2 parts:
    // 1. Formatted section; 2. Unformatted string section. So, 2 steps are needed
    // to skip one SMBIOS structure.
    //

    //
    // Step 1: Skip over formatted section.
    //
    String = (CHAR8 *) (Smbios.Raw + Smbios.Hdr->Length);

    //
    // Step 2: Skip over unformatted string section.
    //
    do {
      //
      // Each string is terminated with a NULL(00h) BYTE and the sets of strings
      // is terminated with an additional NULL(00h) BYTE.
      //
      for ( ; *String != 0; String++) {
      }

      if (*(UINT8*)++String == 0) {
        //
        // Pointer to the next SMBIOS structure.
        //
        Smbios.Raw = (UINT8 *)++String;
        break;
      }
    } while (TRUE);
  } while (Smbios.Raw < SmbiosEnd.Raw);

  return EFI_SUCCESS;
}

/**
  Main entry for the bootloader SMBIOS support DXE module.

  @param[in] ImageHandle    The firmware allocated handle for the EFI image.
  @param[in] SystemTable    A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs when executing this entry point.

**/
EFI_STATUS
EFIAPI
BlDxeSmbiosEntryPoint (
  IN EFI_HANDLE              ImageHandle,
  IN EFI_SYSTEM_TABLE        *SystemTable
  )
{
  EFI_STATUS Status;
  EFI_HOB_GUID_TYPE          *GuidHob;
  SYSTEM_TABLE_INFO          *SystemTableInfo;
  CHAR16                     *MainboardName;
  CHAR16                     *ManufacturerName;

  Status = EFI_SUCCESS;

  //
  // Find the system table information guid hob
  //
  GuidHob = GetFirstGuidHob (&gUefiSystemTableInfoGuid);
  ASSERT (GuidHob != NULL);
  if (GuidHob == NULL) {
    return Status;
  }

  SystemTableInfo = (SYSTEM_TABLE_INFO *)GET_GUID_HOB_DATA (GuidHob);

  //
  // Install Smbios Table
  //
  if (SystemTableInfo->SmbiosTableBase != 0 && SystemTableInfo->SmbiosTableSize != 0) {
    DEBUG ((DEBUG_INFO, "Install SMBIOS Table at 0x%lx, length 0x%x\n",
      SystemTableInfo->SmbiosTableBase, SystemTableInfo->SmbiosTableSize));

    Status = BlDxeInstallSmbiosTables(SystemTableInfo->SmbiosTableBase, SystemTableInfo->SmbiosTableSize);
    if (EFI_ERROR(Status)) {
      //
      // Just install the configuration table. gEfiSmbiosProtocolGuid will return an empty table.
      //
      Status = gBS->InstallConfigurationTable (&gEfiSmbiosTableGuid, (VOID *)(UINTN)SystemTableInfo->SmbiosTableBase);
      ASSERT_EFI_ERROR (Status);
    }
  }

  //
  // Install board specific NULL protocols
  //
  Status = BlDxeGetMainboardName(&MainboardName, &ManufacturerName);
  if (!EFI_ERROR(Status)) {
    DEBUG ((DEBUG_INFO, "Running on mainboard %s '%s'\n", ManufacturerName, MainboardName));
    if (StrCmp(ManufacturerName, L"Prodrive") == 0 && 
        StrCmp(MainboardName, L"Hermes CFL") == 0) {
      Status = gBS->InstallMultipleProtocolInterfaces (&ImageHandle,
        &gEfiProdriveHermesBoardNullGuid, NULL, NULL);
      ASSERT_EFI_ERROR (Status);
    }

    FreePool (MainboardName);
    FreePool (ManufacturerName);
  }

  return EFI_SUCCESS;
}

