/*++

Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
                                                                                   
  This program and the accompanying materials are licensed and made available under
  the terms and conditions of the BSD License that accompanies this distribution.  
  The full text of the license may be found at                                     
  http://opensource.org/licenses/bsd-license.php.                                  
                                                                                   
  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,            
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.    
                                                                                   

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

MISC_SMBIOS_TABLE_FUNCTION(SystemLanguageString)
{
  EFI_STATUS               Status;
  EFI_SMBIOS_HANDLE        SmbiosHandle;
  SMBIOS_TABLE_TYPE13      *SmbiosRecord;
  UINTN                    StrLeng;
  CHAR8                    *OptionalStrStart;
  EFI_STRING               Str;
  STRING_REF               TokenToGet;


  //
  // First check for invalid parameters.
  //
  if (RecordData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  TokenToGet = STRING_TOKEN (STR_MISC_SYSTEM_LANGUAGE_EN_US);
  Str = SmbiosMiscGetString (TokenToGet);
  StrLeng = StrLen(Str);
  if (StrLeng > SMBIOS_STRING_MAX_LENGTH) {
    return EFI_UNSUPPORTED;
  }

  //
  // Two zeros following the last string.
  //
  SmbiosRecord = AllocatePool(sizeof (SMBIOS_TABLE_TYPE13) + StrLeng + 1 + 1);
  ZeroMem(SmbiosRecord, sizeof (SMBIOS_TABLE_TYPE13) + StrLeng + 1 + 1);

  SmbiosRecord->Hdr.Type = EFI_SMBIOS_TYPE_BIOS_LANGUAGE_INFORMATION;
  SmbiosRecord->Hdr.Length = sizeof (SMBIOS_TABLE_TYPE13);

  //
  // Make handle chosen by smbios protocol.add automatically.
  //
  SmbiosRecord->Hdr.Handle    = 0;
  SmbiosRecord->InstallableLanguages = 1;
  SmbiosRecord->Flags   = 1;
  SmbiosRecord->CurrentLanguages = 1;
  OptionalStrStart = (CHAR8 *)(SmbiosRecord + 1);
  UnicodeStrToAsciiStr(Str, OptionalStrStart);

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

