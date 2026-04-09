/** @file
  Parse the INI configuration file and pass the information to the update driver
  so that the driver can perform update accordingly.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "SystemFirmwareDxe.h"
#include <Library/IniParsingLib.h>
#include <Library/PrintLib.h>

#define MAX_LINE_LENGTH  512

/**
  Parse Config data file to get the updated data array.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] ConfigHeader    Pointer to the config header.
  @param[in, out] UpdateArray     Pointer to the config of update data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseUpdateDataFile (
  IN      UINT8               *DataBuffer,
  IN      UINTN               BufferSize,
  IN OUT  CONFIG_HEADER       *ConfigHeader,
  IN OUT  UPDATE_CONFIG_DATA  **UpdateArray
  )
{
  EFI_STATUS  Status;
  CHAR8       *SectionName;
  CHAR8       Entry[MAX_LINE_LENGTH];
  UINTN       Num;
  UINT64      Num64;
  UINTN       Index;
  EFI_GUID    FileGuid;
  VOID        *Context;

  //
  // First process the data buffer and get all sections and entries
  //
  Context = OpenIniFile (DataBuffer, BufferSize);
  if (Context == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Now get NumOfUpdate
  //
  Status = GetDecimalUintnFromDataFile (
             Context,
             "Head",
             "NumOfUpdate",
             &Num
             );
  if (EFI_ERROR (Status) || (Num == 0)) {
    DEBUG ((DEBUG_ERROR, "NumOfUpdate not found\n"));
    CloseIniFile (Context);
    return EFI_NOT_FOUND;
  }

  ConfigHeader->NumOfUpdates = Num;
  *UpdateArray               = AllocateZeroPool ((sizeof (UPDATE_CONFIG_DATA) * Num));
  if (*UpdateArray == NULL) {
    CloseIniFile (Context);
    return EFI_OUT_OF_RESOURCES;
  }

  for (Index = 0; Index < ConfigHeader->NumOfUpdates; Index++) {
    //
    // Get the section name of each update
    //
    AsciiStrCpyS (Entry, MAX_LINE_LENGTH, "Update");
    AsciiValueToStringS (
      Entry + AsciiStrnLenS (Entry, MAX_LINE_LENGTH),
      MAX_LINE_LENGTH - AsciiStrnLenS (Entry, MAX_LINE_LENGTH),
      0,
      Index,
      0
      );
    Status = GetStringFromDataFile (
               Context,
               "Head",
               Entry,
               &SectionName
               );
    if (EFI_ERROR (Status) || (SectionName == NULL)) {
      DEBUG ((DEBUG_ERROR, "[%d] %a not found\n", Index, Entry));
      CloseIniFile (Context);
      return EFI_NOT_FOUND;
    }

    //
    // The section name of this update has been found.
    // Now looks for all the config data of this update
    //
    (*UpdateArray)[Index].Index = Index;

    //
    // FirmwareType
    //
    Status = GetDecimalUintnFromDataFile (
               Context,
               SectionName,
               "FirmwareType",
               &Num
               );
    if (EFI_ERROR (Status)) {
      CloseIniFile (Context);
      DEBUG ((DEBUG_ERROR, "[%d] FirmwareType not found\n", Index));
      return EFI_NOT_FOUND;
    }

    (*UpdateArray)[Index].FirmwareType = (PLATFORM_FIRMWARE_TYPE)Num;

    //
    // AddressType
    //
    Status = GetDecimalUintnFromDataFile (
               Context,
               SectionName,
               "AddressType",
               &Num
               );
    if (EFI_ERROR (Status)) {
      CloseIniFile (Context);
      DEBUG ((DEBUG_ERROR, "[%d] AddressType not found\n", Index));
      return EFI_NOT_FOUND;
    }

    (*UpdateArray)[Index].AddressType = (FLASH_ADDRESS_TYPE)Num;

    //
    // BaseAddress
    //
    Status = GetHexUint64FromDataFile (
               Context,
               SectionName,
               "BaseAddress",
               &Num64
               );
    if (EFI_ERROR (Status)) {
      CloseIniFile (Context);
      DEBUG ((DEBUG_ERROR, "[%d] BaseAddress not found\n", Index));
      return EFI_NOT_FOUND;
    }

    (*UpdateArray)[Index].BaseAddress = (EFI_PHYSICAL_ADDRESS)Num64;

    //
    // FileGuid
    //
    Status = GetGuidFromDataFile (
               Context,
               SectionName,
               "FileGuid",
               &FileGuid
               );
    if (EFI_ERROR (Status)) {
      CloseIniFile (Context);
      DEBUG ((DEBUG_ERROR, "[%d] FileGuid not found\n", Index));
      return EFI_NOT_FOUND;
    }

    CopyGuid (&((*UpdateArray)[Index].FileGuid), &FileGuid);

    //
    // Length
    //
    Status = GetHexUintnFromDataFile (
               Context,
               SectionName,
               "Length",
               &Num
               );
    if (EFI_ERROR (Status)) {
      CloseIniFile (Context);
      DEBUG ((DEBUG_ERROR, "[%d] Length not found\n", Index));
      return EFI_NOT_FOUND;
    }

    (*UpdateArray)[Index].Length = (UINTN)Num;

    //
    // ImageOffset
    //
    Status = GetHexUintnFromDataFile (
               Context,
               SectionName,
               "ImageOffset",
               &Num
               );
    if (EFI_ERROR (Status)) {
      CloseIniFile (Context);
      DEBUG ((DEBUG_ERROR, "[%d] ImageOffset not found\n", Index));
      return EFI_NOT_FOUND;
    }

    (*UpdateArray)[Index].ImageOffset = (UINTN)Num;
  }

  //
  // Now all configuration data got. Free those temporary buffers
  //
  CloseIniFile (Context);

  return EFI_SUCCESS;
}
