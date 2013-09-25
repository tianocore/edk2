/** @file
  API for SMBIOS table.

  Copyright (c) 2005 - 2012, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
    

#include "../UefiShellDebug1CommandsLib.h"
#include <Guid/SmBios.h>
#include "LibSmbiosView.h"
#include "SmbiosView.h"

STATIC UINT8                    mInit         = 0;
STATIC SMBIOS_TABLE_ENTRY_POINT *mSmbiosTable = NULL;
STATIC SMBIOS_STRUCTURE_POINTER m_SmbiosStruct;
STATIC SMBIOS_STRUCTURE_POINTER *mSmbiosStruct = &m_SmbiosStruct;

/**
  Init the SMBIOS VIEW API's environment.

  @retval EFI_SUCCESS  Successful to init the SMBIOS VIEW Lib.
**/
EFI_STATUS
LibSmbiosInit (
  VOID
  )
{
  EFI_STATUS  Status;

  //
  // Init only once
  //
  if (mInit == 1) {
    return EFI_SUCCESS;
  }
  //
  // Get SMBIOS table from System Configure table
  //
  Status = GetSystemConfigurationTable (&gEfiSmbiosTableGuid, (VOID**)&mSmbiosTable);

  if (mSmbiosTable == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_CANNOT_GET_TABLE), gShellDebug1HiiHandle);
    return EFI_NOT_FOUND;
  }

  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_GET_TABLE_ERROR), gShellDebug1HiiHandle, Status);
    return Status;
  }
  //
  // Init SMBIOS structure table address
  //
  mSmbiosStruct->Raw  = (UINT8 *) (UINTN) (mSmbiosTable->TableAddress);

  mInit               = 1;
  return EFI_SUCCESS;
}

/**
  Cleanup the Smbios information.
**/
VOID
LibSmbiosCleanup (
  VOID
  )
{
  //
  // Release resources
  //
  if (mSmbiosTable != NULL) {
    mSmbiosTable = NULL;
  }

  mInit = 0;
}

/**
  Get the entry point structure for the table.

  @param[out] EntryPointStructure  The pointer to populate.
**/
VOID
LibSmbiosGetEPS (
  OUT SMBIOS_TABLE_ENTRY_POINT **EntryPointStructure
  )
{
  //
  // return SMBIOS Table address
  //
  *EntryPointStructure = mSmbiosTable;
}

/**
  Return SMBIOS string for the given string number.

  @param[in] Smbios         Pointer to SMBIOS structure.
  @param[in] StringNumber   String number to return. -1 is used to skip all strings and
                            point to the next SMBIOS structure.

  @return Pointer to string, or pointer to next SMBIOS strcuture if StringNumber == -1
**/
CHAR8*
LibGetSmbiosString (
  IN  SMBIOS_STRUCTURE_POINTER    *Smbios,
  IN  UINT16                      StringNumber
  )
{
  UINT16  Index;
  CHAR8   *String;

  ASSERT (Smbios != NULL);

  //
  // Skip over formatted section
  //
  String = (CHAR8 *) (Smbios->Raw + Smbios->Hdr->Length);

  //
  // Look through unformated section
  //
  for (Index = 1; Index <= StringNumber; Index++) {
    if (StringNumber == Index) {
      return String;
    }
    //
    // Skip string
    //
    for (; *String != 0; String++);
    String++;

    if (*String == 0) {
      //
      // If double NULL then we are done.
      //  Return pointer to next structure in Smbios.
      //  if you pass in a -1 you will always get here
      //
      Smbios->Raw = (UINT8 *)++String;
      return NULL;
    }
  }

  return NULL;
}

/**
    Get SMBIOS structure for the given Handle,
    Handle is changed to the next handle or 0xFFFF when the end is
    reached or the handle is not found.

    @param[in, out] Handle     0xFFFF: get the first structure
                               Others: get a structure according to this value.
    @param[out] Buffer         The pointer to the pointer to the structure.
    @param[out] Length         Length of the structure.

    @retval DMI_SUCCESS   Handle is updated with next structure handle or
                          0xFFFF(end-of-list).

    @retval DMI_INVALID_HANDLE  Handle is updated with first structure handle or
                                0xFFFF(end-of-list).
**/
EFI_STATUS
LibGetSmbiosStructure (
  IN  OUT UINT16  *Handle,
  OUT UINT8       **Buffer,
  OUT UINT16      *Length
  )
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINT8                     *Raw;

  if (*Handle == INVALID_HANDLE) {
    *Handle = mSmbiosStruct->Hdr->Handle;
    return DMI_INVALID_HANDLE;
  }

  if ((Buffer == NULL) || (Length == NULL)) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_NO_BUFF_LEN_SPEC), gShellDebug1HiiHandle);
    return DMI_INVALID_HANDLE;
  }

  *Length       = 0;
  Smbios.Hdr    = mSmbiosStruct->Hdr;
  SmbiosEnd.Raw = Smbios.Raw + mSmbiosTable->TableLength;
  while (Smbios.Raw < SmbiosEnd.Raw) {
    if (Smbios.Hdr->Handle == *Handle) {
      Raw = Smbios.Raw;
      //
      // Walk to next structure
      //
      LibGetSmbiosString (&Smbios, (UINT16) (-1));
      //
      // Length = Next structure head - this structure head
      //
      *Length = (UINT16) (Smbios.Raw - Raw);
      *Buffer = Raw;
      //
      // update with the next structure handle.
      //
      if (Smbios.Raw < SmbiosEnd.Raw) {
        *Handle = Smbios.Hdr->Handle;
      } else {
        *Handle = INVALID_HANDLE;
      }
      return DMI_SUCCESS;
    }
    //
    // Walk to next structure
    //
    LibGetSmbiosString (&Smbios, (UINT16) (-1));
  }

  *Handle = INVALID_HANDLE;
  return DMI_INVALID_HANDLE;
}

