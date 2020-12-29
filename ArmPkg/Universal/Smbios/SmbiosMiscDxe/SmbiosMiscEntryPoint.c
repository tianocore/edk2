/** @file
  This driver parses the mSmbiosMiscDataTable structure and reports
  any generated data using SMBIOS protocol.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"


EFI_HANDLE              mImageHandle;
EFI_HII_HANDLE          mHiiHandle;
EFI_SMBIOS_PROTOCOL     *mSmbios = NULL;

/**
  Standard EFI driver point.  This driver parses the mSmbiosMiscDataTable
  structure and reports any generated data using SMBIOS protocol.

  @param  ImageHandle     Handle for the image of this driver
  @param  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

**/
EFI_STATUS
EFIAPI
SmbiosMiscEntryPoint(
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  UINTN                Index;
  EFI_STATUS           EfiStatus;
  EFI_SMBIOS_PROTOCOL  *Smbios;

  mImageHandle = ImageHandle;

  EfiStatus = gBS->LocateProtocol (&gEfiSmbiosProtocolGuid, NULL, (VOID**)&Smbios);
  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
    return EfiStatus;
  }

  mSmbios = Smbios;

  mHiiHandle = HiiAddPackages (
                 &gEfiCallerIdGuid,
                  mImageHandle,
                  SmbiosMiscDxeStrings,
                  NULL
                  );
  if (mHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mSmbiosMiscDataTableEntries; ++Index) {
    //
    // If the entry have a function pointer, just log the data.
    //
    if (mSmbiosMiscDataTable[Index].Function != NULL) {
      EfiStatus = (*mSmbiosMiscDataTable[Index].Function)(
          mSmbiosMiscDataTable[Index].RecordData,
          Smbios
          );

      if (EFI_ERROR(EfiStatus)) {
        DEBUG ((DEBUG_ERROR, "Misc smbios store error.  Index=%d, ReturnStatus=%r\n", Index, EfiStatus));
        return EfiStatus;
      }
    }
  }

  return EfiStatus;
}


/**
  Logs SMBIOS record.

  @param  Buffer                The data for the fixed portion of the SMBIOS record. The format of the record is
                                determined by EFI_SMBIOS_TABLE_HEADER.Type. The size of the formatted area is defined
                                by EFI_SMBIOS_TABLE_HEADER.Length and either followed by a double-null (0x0000) or
                                a set of null terminated strings and a null.
  @param  SmbiosHandle          A unique handle will be assigned to the SMBIOS record.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.

**/
EFI_STATUS
LogSmbiosData (
  IN       UINT8                      *Buffer,
  IN  OUT  EFI_SMBIOS_HANDLE          *SmbiosHandle
  )
{
  EFI_STATUS         Status;

  *SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

  Status = mSmbios->Add (
                   mSmbios,
                   NULL,
                   SmbiosHandle,
                   (EFI_SMBIOS_TABLE_HEADER *)Buffer
                   );

  return Status;
}

/**
  Fetches a list of the specified SMBIOS table types.

  @param[in]  SmbiosType    The type of table to fetch
  @param[out] **HandleArray The array of handles
  @param[out] *HandleCount  Number of handles in the array
**/
VOID
GetLinkTypeHandle(
  IN  UINT8                 SmbiosType,
  OUT SMBIOS_HANDLE         **HandleArray,
  OUT UINTN                 *HandleCount
  )
{
  UINTN                    Index;
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;

  if (mSmbios == NULL) {
    return;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  *HandleCount = 0;

  // Iterate through entries to get the number
  while (TRUE) {
    Status = mSmbios->GetNext (
                        mSmbios,
                        &SmbiosHandle,
                        &SmbiosType,
                        &Record,
                        NULL
                        );

    if (!EFI_ERROR (Status)) {
      (*HandleCount)++;
    } else {
      break;
    }
  }

  *HandleArray = AllocateZeroPool (sizeof (SMBIOS_HANDLE) * (*HandleCount));
  if (*HandleArray == NULL) {
    DEBUG ((DEBUG_ERROR, "HandleArray allocate memory resource failed.\n"));
    *HandleCount = 0;
    return;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

  for (Index = 0; Index < (*HandleCount); Index++) {
    Status = mSmbios->GetNext (
                        mSmbios,
                        &SmbiosHandle,
                        &SmbiosType,
                        &Record,
                        NULL
                        );

    if (!EFI_ERROR (Status)) {
      (*HandleArray)[Index] = Record->Handle;
    } else {
      break;
    }
  }
}

