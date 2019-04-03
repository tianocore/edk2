/*++

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  SPDX-License-Identifier: BSD-2-Clause-Patent

                                                                                   

Module Name:

  MiscResetCapabilitiesFunction.c

Abstract:

  ResetCapabilities.
  SMBIOS type 23.

--*/


#include "CommonHeader.h"

#include "MiscSubclassDriver.h"

/**
  This function makes boot time changes to the contents of the
  MiscOemString (Type 11).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(MiscResetCapabilities)
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  SMBIOS_TABLE_TYPE23      *SmbiosRecord;
  EFI_MISC_RESET_CAPABILITIES   *ForType23InputData;

  ForType23InputData = (EFI_MISC_RESET_CAPABILITIES *)RecordData;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }


  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE23) + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE23) + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_RESET;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE23);

  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle    = 0;
  SmbiosRecord->Capabilities  = *(UINT8*)&(ForType23InputData->ResetCapabilities);
  SmbiosRecord->ResetCount    = (UINT16)ForType23InputData->ResetCount;
  SmbiosRecord->ResetLimit    = (UINT16)ForType23InputData->ResetLimit;
  SmbiosRecord->TimerInterval = (UINT16)ForType23InputData->ResetTimerInterval;
  SmbiosRecord->Timeout       = (UINT16)ForType23InputData->ResetTimeout;

  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = SMBIOS_HANDLE_PI_RESERVED;
  Status = Smbios-> Add(
                      Smbios,
                      NULL,
                      &SmbiosHandle,
                      (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                      );
  FreePool(SmbiosRecord);
  return Status;
}

