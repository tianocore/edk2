/** @file
  This driver parses the mSmbiosMiscDataTable structure and reports
  any generated data using SMBIOS protocol.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include "SmbiosMisc.h"

STATIC EFI_HANDLE           mSmbiosMiscImageHandle;
STATIC EFI_SMBIOS_PROTOCOL  *mSmbiosMiscSmbios = NULL;

EFI_HII_HANDLE  mSmbiosMiscHiiHandle;

/**
  Standard EFI driver point.  This driver parses the mSmbiosMiscDataTable
  structure and reports any generated data using SMBIOS protocol.

  @param  ImageHandle     Handle for the image of this driver
  @param  SystemTable     Pointer to the EFI System Table

  @retval  EFI_SUCCESS    The data was successfully stored.

**/
EFI_STATUS
EFIAPI
SmbiosMiscEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  UINTN       Index;
  EFI_STATUS  EfiStatus;

  mSmbiosMiscImageHandle = ImageHandle;

  EfiStatus = gBS->LocateProtocol (
                     &gEfiSmbiosProtocolGuid,
                     NULL,
                     (VOID **)&mSmbiosMiscSmbios
                     );
  if (EFI_ERROR (EfiStatus)) {
    DEBUG ((DEBUG_ERROR, "Could not locate SMBIOS protocol.  %r\n", EfiStatus));
    return EfiStatus;
  }

  mSmbiosMiscHiiHandle = HiiAddPackages (
                           &gEfiCallerIdGuid,
                           mSmbiosMiscImageHandle,
                           SmbiosMiscDxeStrings,
                           NULL
                           );
  if (mSmbiosMiscHiiHandle == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < mSmbiosMiscDataTableEntries; ++Index) {
    //
    // If the entry have a function pointer, just log the data.
    //
    if (mSmbiosMiscDataTable[Index].Function != NULL) {
      EfiStatus = (*mSmbiosMiscDataTable[Index].Function)(
  mSmbiosMiscDataTable[Index].RecordData,
  mSmbiosMiscSmbios
  );

      if (EFI_ERROR (EfiStatus)) {
        DEBUG ((
          DEBUG_ERROR,
          "Misc smbios store error.  Index=%d,"
          "ReturnStatus=%r\n",
          Index,
          EfiStatus
          ));
        return EfiStatus;
      }
    }
  }

  return EfiStatus;
}

/**
  Adds an SMBIOS record.

  @param  Buffer        The data for the SMBIOS record.
                        The format of the record is determined by
                        EFI_SMBIOS_TABLE_HEADER.Type. The size of the
                        formatted area is defined by EFI_SMBIOS_TABLE_HEADER.Length
                        and either followed by a double-null (0x0000) or a set
                        of null terminated strings and a null.
  @param  SmbiosHandle  A unique handle will be assigned to the SMBIOS record
                        if not NULL.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed in was already in use.

**/
EFI_STATUS
SmbiosMiscAddRecord (
  IN  UINT8                 *Buffer,
  IN OUT EFI_SMBIOS_HANDLE  *SmbiosHandle OPTIONAL
  )
{
  EFI_STATUS         Status;
  EFI_SMBIOS_HANDLE  Handle;

  Handle = SMBIOS_HANDLE_PI_RESERVED;

  if (SmbiosHandle != NULL) {
    Handle = *SmbiosHandle;
  }

  Status = mSmbiosMiscSmbios->Add (
                                mSmbiosMiscSmbios,
                                NULL,
                                &Handle,
                                (EFI_SMBIOS_TABLE_HEADER *)Buffer
                                );

  if (SmbiosHandle != NULL) {
    *SmbiosHandle = Handle;
  }

  return Status;
}

/** Fetches the number of handles of the specified SMBIOS type.
 *
 *  @param SmbiosType The type of SMBIOS record to look for.
 *
 *  @return The number of handles
 *
**/
STATIC
UINTN
GetHandleCount (
  IN  UINT8  SmbiosType
  )
{
  UINTN                    HandleCount;
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;

  HandleCount = 0;

  // Iterate through entries to get the number
  do {
    Status = mSmbiosMiscSmbios->GetNext (
                                  mSmbiosMiscSmbios,
                                  &SmbiosHandle,
                                  &SmbiosType,
                                  &Record,
                                  NULL
                                  );

    if (Status == EFI_SUCCESS) {
      HandleCount++;
    }
  } while (!EFI_ERROR (Status));

  return HandleCount;
}

/**
  Fetches a list of the specified SMBIOS table types.

  @param[in]  SmbiosType    The type of table to fetch
  @param[out] **HandleArray The array of handles
  @param[out] *HandleCount  Number of handles in the array
**/
VOID
SmbiosMiscGetLinkTypeHandle (
  IN  UINT8          SmbiosType,
  OUT SMBIOS_HANDLE  **HandleArray,
  OUT UINTN          *HandleCount
  )
{
  UINTN                    Index;
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  EFI_SMBIOS_TABLE_HEADER  *Record;

  if (mSmbiosMiscSmbios == NULL) {
    return;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  *HandleCount = GetHandleCount (SmbiosType);

  *HandleArray = AllocateZeroPool (sizeof (SMBIOS_HANDLE) * (*HandleCount));
  if (*HandleArray == NULL) {
    DEBUG ((DEBUG_ERROR, "HandleArray allocate memory resource failed.\n"));
    *HandleCount = 0;
    return;
  }

  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;

  for (Index = 0; Index < (*HandleCount); Index++) {
    Status = mSmbiosMiscSmbios->GetNext (
                                  mSmbiosMiscSmbios,
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
