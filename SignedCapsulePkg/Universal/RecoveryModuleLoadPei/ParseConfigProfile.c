/** @file
  Parse the INI configuration file and pass the information to the recovery driver
  so that the driver can perform recovery accordingly.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "RecoveryModuleLoadPei.h"
#include <Library/IniParsingLib.h>
#include <Library/PrintLib.h>

#define MAX_LINE_LENGTH           512

/**
  Parse Config data file to get the updated data array.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] ConfigHeader    Pointer to the config header.
  @param[in, out] RecoveryArray   Pointer to the config of recovery data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseRecoveryDataFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize,
  IN OUT  CONFIG_HEADER                 *ConfigHeader,
  IN OUT  RECOVERY_CONFIG_DATA          **RecoveryArray
  )
{
  EFI_STATUS                            Status;
  CHAR8                                 *SectionName;
  CHAR8                                 Entry[MAX_LINE_LENGTH];
  UINTN                                 Num;
  UINTN                                 Index;
  EFI_GUID                              FileGuid;
  VOID                                  *Context;

  //
  // First process the data buffer and get all sections and entries
  //
  Context = OpenIniFile(DataBuffer, BufferSize);
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now get NumOfUpdate
  //
  Status = GetDecimalUintnFromDataFile(
             Context,
             "Head",
             "NumOfRecovery",
             &Num
             );
  if (EFI_ERROR(Status) || (Num == 0)) {
    DEBUG((DEBUG_ERROR, "NumOfRecovery not found\n"));
    CloseIniFile(Context);
    return EFI_NOT_FOUND;
  }

  ConfigHeader->NumOfRecovery = Num;
  *RecoveryArray = AllocateZeroPool ((sizeof (RECOVERY_CONFIG_DATA) * Num));
  if (*RecoveryArray == NULL) {
    CloseIniFile(Context);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0 ; Index < ConfigHeader->NumOfRecovery; Index++) {
    //
    // Get the section name of each update
    //
    AsciiStrCpyS (Entry, MAX_LINE_LENGTH, "Recovery");
    AsciiValueToStringS (
      Entry + AsciiStrnLenS (Entry, MAX_LINE_LENGTH),
      MAX_LINE_LENGTH - AsciiStrnLenS (Entry, MAX_LINE_LENGTH),
      0,
      Index,
      0
      );
    Status = GetStringFromDataFile(
               Context,
               "Head",
               Entry,
               &SectionName
               );
    if (EFI_ERROR(Status) || (SectionName == NULL)) {
      DEBUG((DEBUG_ERROR, "[%d] %a not found\n", Index, Entry));
      CloseIniFile(Context);
      return EFI_NOT_FOUND;
    }

    //
    // The section name of this update has been found.
    // Now looks for all the config data of this update
    //

    //
    // FileBuid
    //
    Status = GetGuidFromDataFile(
               Context,
               SectionName,
               "FileGuid",
               &FileGuid
               );
    if (EFI_ERROR(Status)) {
      CloseIniFile(Context);
      DEBUG((DEBUG_ERROR, "[%d] FileGuid not found\n", Index));
      return EFI_NOT_FOUND;
    }

    CopyGuid(&((*RecoveryArray)[Index].FileGuid), &FileGuid);

    //
    // Length
    //
    Status = GetHexUintnFromDataFile(
               Context,
               SectionName,
               "Length",
               &Num
               );
    if (EFI_ERROR(Status)) {
      CloseIniFile(Context);
      DEBUG((DEBUG_ERROR, "[%d] Length not found\n", Index));
      return EFI_NOT_FOUND;
    }
    (*RecoveryArray)[Index].Length = Num;

    //
    // ImageOffset
    //
    Status = GetHexUintnFromDataFile(
               Context,
               SectionName,
               "ImageOffset",
               &Num
               );
    if (EFI_ERROR(Status)) {
      CloseIniFile(Context);
      DEBUG((DEBUG_ERROR, "[%d] ImageOffset not found\n", Index));
      return EFI_NOT_FOUND;
    }
    (*RecoveryArray)[Index].ImageOffset = Num;
  }

  //
  // Now all configuration data got. Free those temporary buffers
  //
  CloseIniFile(Context);

  return EFI_SUCCESS;
}

