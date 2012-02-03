/*++

Copyright (c) 2006 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MiscSubclassDriverEntryPoint.c

Abstract:

  This driver parses the mMiscSubclassDataTable structure and reports
  any generated data to the DataHub.

**/

#include "MiscSubClassDriver.h"

EFI_HII_HANDLE  mHiiHandle;

/**
  This is the standard EFI driver point that detects whether there is a
  MemoryConfigurationData Variable and, if so, reports memory configuration info
  to the DataHub.

  @param  ImageHandle  Handle for the image of this driver
  @param  SystemTable  Pointer to the EFI System Table

  @return EFI_SUCCESS if the data is successfully reported
  @return EFI_NOT_FOUND if the HOB list could not be located.

**/
EFI_STATUS
LogMemorySmbiosRecord (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINT64                          TotalMemorySize;
  UINT8                           NumSlots;
  SMBIOS_TABLE_TYPE19             *Type19Record;
  EFI_SMBIOS_HANDLE               MemArrayMappedAddrSmbiosHandle;
  EFI_SMBIOS_PROTOCOL             *Smbios;
  CHAR16                          *MemString;

  Status = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  ASSERT_EFI_ERROR (Status);

  NumSlots        = 1;

  //
  // Process Memory String in form size!size ...
  // So 64!64 is 128 MB
  //
  MemString   = (CHAR16 *)PcdGetPtr (PcdEmuMemorySize);
  for (TotalMemorySize = 0; *MemString != '\0';) {
    TotalMemorySize += StrDecimalToUint64 (MemString);
    while (*MemString != '\0') {
      if (*MemString == '!') {
        MemString++;
        break;
      }
      MemString++;
    }
  }

  //
  // Convert Total Memory Size to based on KiloByte
  //
  TotalMemorySize = LShiftU64 (TotalMemorySize, 20);
  //
  // Generate Memory Array Mapped Address info
  //
  Type19Record = AllocateZeroPool(sizeof (SMBIOS_TABLE_TYPE19) + 2);
  Type19Record->Hdr.Type = EFI_SMBIOS_TYPE_MEMORY_ARRAY_MAPPED_ADDRESS;
  Type19Record->Hdr.Length = sizeof(SMBIOS_TABLE_TYPE19);
  Type19Record->Hdr.Handle = 0;
  Type19Record->StartingAddress = 0;
  Type19Record->EndingAddress =  (UINT32)RShiftU64(TotalMemorySize, 10) - 1;
  Type19Record->MemoryArrayHandle = 0;
  Type19Record->PartitionWidth = (UINT8)(NumSlots);

  //
  // Generate Memory Array Mapped Address info (TYPE 19)
  //
  Status = AddSmbiosRecord (Smbios, &MemArrayMappedAddrSmbiosHandle, (EFI_SMBIOS_TABLE_HEADER*) Type19Record);

  FreePool(Type19Record);
  ASSERT_EFI_ERROR (Status);

  return Status;
}


EFI_STATUS
EFIAPI
MiscSubclassDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
/*++
Description:

  Standard EFI driver point.  This driver parses the mMiscSubclassDataTable
  structure and reports any generated data to the DataHub.

Arguments:

  ImageHandle
    Handle for the image of this driver

  SystemTable
    Pointer to the EFI System Table

Returns:

  EFI_SUCCESS
    The data was successfully reported to the Data Hub.

**/
{
  UINTN                Index;
  EFI_STATUS           EfiStatus;
  EFI_SMBIOS_PROTOCOL  *Smbios;

  EfiStatus = gBS->LocateProtocol(&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);

  if (EFI_ERROR(EfiStatus)) {
    DEBUG((EFI_D_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
    return EfiStatus;
  }

  mHiiHandle = HiiAddPackages (
                 &gEfiCallerIdGuid,
                 NULL,
                 MiscSubclassStrings,
                 NULL
                 );
  ASSERT (mHiiHandle != NULL);

  for (Index = 0; Index < mMiscSubclassDataTableEntries; ++Index) {
    //
    // If the entry have a function pointer, just log the data.
    //
    if (mMiscSubclassDataTable[Index].Function != NULL) {
      EfiStatus = (*mMiscSubclassDataTable[Index].Function)(
        mMiscSubclassDataTable[Index].RecordData,
        Smbios
        );

      if (EFI_ERROR(EfiStatus)) {
        DEBUG((EFI_D_ERROR, "Misc smbios store error.  Index=%d, ReturnStatus=%r\n", Index, EfiStatus));
        return EfiStatus;
      }
    }
  }

  //
  // Log Memory SMBIOS Record
  //
  EfiStatus = LogMemorySmbiosRecord();
  return EfiStatus;
}

/**
  Add an SMBIOS record.

  @param  Smbios                The EFI_SMBIOS_PROTOCOL instance.
  @param  SmbiosHandle          A unique handle will be assigned to the SMBIOS record.
  @param  Record                The data for the fixed portion of the SMBIOS record. The format of the record is
                                determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined 
                                by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or 
                                a set of null terminated strings and a null.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.

**/
EFI_STATUS
AddSmbiosRecord (
  IN EFI_SMBIOS_PROTOCOL        *Smbios,
  OUT EFI_SMBIOS_HANDLE         *SmbiosHandle,
  IN EFI_SMBIOS_TABLE_HEADER    *Record
  )
{
  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  return Smbios->Add (
                   Smbios,
                   NULL,
                   SmbiosHandle,
                   Record
                   );
}

