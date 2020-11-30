/** @file
  boot information boot time changes.
  SMBIOS type 32.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2009 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SmbiosMisc.h"

/**
  This function makes boot time changes to the contents of the
  MiscBootInformation (Type 32).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/

MISC_SMBIOS_TABLE_FUNCTION(MiscBootInformation)
{
  EFI_STATUS                         Status;
  EFI_SMBIOS_HANDLE                  SmbiosHandle;
  SMBIOS_TABLE_TYPE32                *SmbiosRecord;
  SMBIOS_TABLE_TYPE32                *InputData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  InputData = (SMBIOS_TABLE_TYPE32 *)RecordData;

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocateZeroPool (sizeof (SMBIOS_TABLE_TYPE32) + 1 + 1);
  if (SmbiosRecord == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  (VOID)CopyMem (SmbiosRecord, InputData, sizeof (SMBIOS_TABLE_TYPE32));

  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE32);

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  Status = LogSmbiosData ((UINT8*)SmbiosRecord, &SmbiosHandle);
  if (EFI_ERROR (Status)) {
    DEBUG ((DEBUG_ERROR, "[%a]:[%dL] Smbios Type32 Table Log Failed! %r \n",
            __FUNCTION__, __LINE__, Status));
  }

  FreePool (SmbiosRecord);
  return Status;
}
