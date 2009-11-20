/*++
 
Copyright (c) 2009, Intel Corporation. All rights reserved. <BR> 
This software and associated documentation (if any) is furnished
under a license and may only be used or copied in accordance
with the terms of the license. Except as permitted by such
license, no part of this software or documentation may be
reproduced, stored in a retrieval system, or transmitted in any
form or by any means without the express written consent of
Intel Corporation.


Module Name:

  MiscSystemOptionStringFunction.c
  
Abstract: 

  BIOS system option string boot time changes.
  SMBIOS type 12.

--*/

#include "MiscSubclassDriver.h"


/**
  This function makes boot time changes to the contents of the
  MiscSystemOptionString (Type 12).

  @param  RecordData                 Pointer to copy of RecordData from the Data Table.  

  @retval EFI_SUCCESS                All parameters were valid.
  @retval EFI_UNSUPPORTED            Unexpected RecordType value.
  @retval EFI_INVALID_PARAMETER      Invalid parameter was found.

**/
MISC_SMBIOS_TABLE_FUNCTION(SystemOptionString)
{
  CHAR8                             *OptionalStrStart;
  UINTN                             OptStrLen;
  EFI_STRING                        OptionString;
  EFI_STATUS                        Status;
  STRING_REF                        TokenToGet;
  EFI_SMBIOS_HANDLE                 SmbiosHandle;
  SMBIOS_TABLE_TYPE12               *SmbiosRecord;

  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_OPTION_STRING);
  OptionString = HiiGetPackageString(&gEfiCallerIdGuid, TokenToGet, NULL);
  OptStrLen = StrLen(OptionString);
  if (OptStrLen > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }
 
  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE12) + OptStrLen + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE12) + OptStrLen + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE12);
  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle = 0;  

  SmbiosRecord->StringCount = 1;
  OptionalStrStart = (CHAR8*) (SmbiosRecord + 1);
  UnicodeStrToAsciiStr(OptionString, OptionalStrStart);
  //
  // Now we have got the full smbios record, call smbios protocol to add this record.
  //
  SmbiosHandle = 0;
  Status = Smbios-> Add(
                      Smbios, 
                      NULL,
                      &SmbiosHandle, 
                      (EFI_SMBIOS_TABLE_HEADER *) SmbiosRecord
                      );

  FreePool(SmbiosRecord);
  return Status;
}
