/** @file
  API for SMBIOS table.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
    

#include "../UefiShellDebug1CommandsLib.h"
#include <Guid/Smbios.h>
#include "LIbSmbios.h"
#include "LibSmbiosView.h"
#include "smbiosview.h"

STATIC UINT8                    mInit         = 0;
STATIC SMBIOS_STRUCTURE_TABLE   *mSmbiosTable = NULL;
STATIC SMBIOS_STRUCTURE_POINTER m_SmbiosStruct;
STATIC SMBIOS_STRUCTURE_POINTER *mSmbiosStruct = &m_SmbiosStruct;

EFI_STATUS
LibSmbiosInit (
  VOID
  )
/*++

Routine Description:
    Init the SMBIOS VIEW API's environment.

  Arguments:
    None

Returns:
    EFI_SUCCESS       - Successful to init the SMBIOS VIEW Lib
    Others            - Cannot get SMBIOS Table

**/
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

VOID
LibSmbiosGetEPS (
  SMBIOS_STRUCTURE_TABLE **pEntryPointStructure
  )
{
  //
  // return SMBIOS Table address
  //
  *pEntryPointStructure = mSmbiosTable;
}

VOID
LibSmbiosGetStructHead (
  SMBIOS_STRUCTURE_POINTER *pHead
  )
{
  //
  // return SMBIOS structure table address
  //
  pHead = mSmbiosStruct;
}

EFI_STATUS
LibGetSmbiosInfo (
  OUT CHAR8   *dmiBIOSRevision,
  OUT UINT16  *NumStructures,
  OUT UINT16  *StructureSize,
  OUT UINT32  *dmiStorageBase,
  OUT UINT16  *dmiStorageSize
  )
/*++

Routine Description:
    Get SMBIOS Information.

  Arguments:
    dmiBIOSRevision   - Revision of the SMBIOS Extensions.
    NumStructures     - Max. Number of Structures the BIOS will return.
    StructureSize     - Size of largest SMBIOS Structure.
    dmiStorageBase    - 32-bit physical base address for memory mapped SMBIOS data.
    dmiStorageSize    - Size of the memory-mapped SMBIOS data.

  Returns:
    DMI_SUCCESS                 - successful.
    DMI_FUNCTION_NOT_SUPPORTED  - Does not support SMBIOS calling interface capability.

**/
{
  //
  // If no SMIBOS table, unsupported.
  //
  if (mSmbiosTable == NULL) {
    return DMI_FUNCTION_NOT_SUPPORTED;
  }

  *dmiBIOSRevision  = mSmbiosTable->SmbiosBcdRevision;
  *NumStructures    = mSmbiosTable->NumberOfSmbiosStructures;
  *StructureSize    = mSmbiosTable->MaxStructureSize;
  *dmiStorageBase   = mSmbiosTable->TableAddress;
  *dmiStorageSize   = mSmbiosTable->TableLength;

  return DMI_SUCCESS;
}

EFI_STATUS
LibGetSmbiosStructure (
  IN  OUT UINT16  *Handle,
  IN  OUT UINT8   *Buffer,
  OUT UINT16      *Length
  )
/*++

  Routine Description:
    Get SMBIOS structure given the Handle,copy data to the Buffer,
    Handle is changed to the next handle or 0xFFFF when the end is
    reached or the handle is not found.

  Arguments:
    Handle:         - 0xFFFF: get the first structure
                    - Others: get a structure according to this value.
    Buffter:        - The pointer to the caller's memory buffer.
    Length:         - Length of return buffer in bytes.

  Returns:
    DMI_SUCCESS         - Buffer contains the required structure data
                        - Handle is updated with next structure handle or
                          0xFFFF(end-of-list).

    DMI_INVALID_HANDLE  - Buffer not contain the requiring structure data
                        - Handle is updated with next structure handle or
                          0xFFFF(end-of-list).
**/
{
  SMBIOS_STRUCTURE_POINTER  Smbios;
  SMBIOS_STRUCTURE_POINTER  SmbiosEnd;
  UINT8                     *Raw;

  if (*Handle == INVALIDE_HANDLE) {
    *Handle = mSmbiosStruct->Hdr->Handle;
    return DMI_INVALID_HANDLE;
  }

  if (Buffer == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_NO_BUFF_SPEC), gShellDebug1HiiHandle);
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
      CopyMem (Buffer, Raw, *Length);
      //
      // update with the next structure handle.
      //
      if (Smbios.Raw < SmbiosEnd.Raw) {
        *Handle = Smbios.Hdr->Handle;
      } else {
        *Handle = INVALIDE_HANDLE;
      }
      return DMI_SUCCESS;
    }
    //
    // Walk to next structure
    //
    LibGetSmbiosString (&Smbios, (UINT16) (-1));
  }

  *Handle = INVALIDE_HANDLE;
  return DMI_INVALID_HANDLE;
}

EFI_STATUS
SmbiosCheckStructure (
  IN  SMBIOS_STRUCTURE_POINTER *Smbios
  )
/*++

  Routine Description:
    Check the structure to see if it is legal.

  Arguments:
    Smbios    - Pointer to the structure that will be checked.

  Returns:
    DMI_SUCCESS     - Structure data is legal.
    DMI_BAD_PARAMETER - Structure data contains bad parameter

**/
{
  //
  // If key != value, then error.
  //
#define CHECK_VALUE(key, value) (((key) == (value)) ? EFI_SUCCESS : DMI_BAD_PARAMETER)

  EFI_STATUS  Status;
  //
  // Assume staus is EFI_SUCCESS,
  // but if check is error, then EFI_ERROR.
  //
  Status = EFI_SUCCESS;

  switch (Smbios->Hdr->Type) {
  case 0:
    break;

  case 1:
    if (Smbios->Type1->Hdr.Length == 0x08 || Smbios->Type0->Hdr.Length == 0x19) {
      Status = EFI_SUCCESS;
    } else {
      Status = DMI_BAD_PARAMETER;
    }
    break;

  case 2:
    Status = CHECK_VALUE (Smbios->Type2->Hdr.Length, 0x08);
    break;

  case 6:
    Status = CHECK_VALUE (Smbios->Type6->Hdr.Length, 0x0C);
    break;

  case 11:
    Status = CHECK_VALUE (Smbios->Type11->Hdr.Length, 0x05);
    break;

  case 12:
    Status = CHECK_VALUE (Smbios->Type12->Hdr.Length, 0x05);
    break;

  case 13:
    Status = CHECK_VALUE (Smbios->Type13->Hdr.Length, 0x16);
    break;

  case 16:
    Status = CHECK_VALUE (Smbios->Type16->Hdr.Length, 0x0F);
    break;

  case 19:
    Status = CHECK_VALUE (Smbios->Type19->Hdr.Length, 0x0F);
    break;

  case 20:
    Status = CHECK_VALUE (Smbios->Type20->Hdr.Length, 0x13);
    break;

  case 32:
    //
    // Because EFI_SUCCESS == 0,
    // So errors added up is also error.
    //
    Status = CHECK_VALUE (Smbios->Type32->Reserved[0], 0x00) +
      CHECK_VALUE (Smbios->Type32->Reserved[1], 0x00) +
      CHECK_VALUE (Smbios->Type32->Reserved[2], 0x00) +
      CHECK_VALUE (Smbios->Type32->Reserved[3], 0x00) +
      CHECK_VALUE (Smbios->Type32->Reserved[4], 0x00) +
      CHECK_VALUE (Smbios->Type32->Reserved[5], 0x00);
    break;

  default:
    Status = DMI_BAD_PARAMETER;
  }

  return Status;
}

VOID
SmbiosGetPendingString (
  IN  SMBIOS_STRUCTURE_POINTER      *Smbios,
  IN  UINT16                        StringNumber,
  OUT CHAR8                         *Buffer
  )
{
  CHAR8 *String;
  if (Buffer == NULL) {
    ShellPrintHiiEx(-1,-1,NULL,STRING_TOKEN (STR_SMBIOSVIEW_LIBSMBIOSVIEW_NO_BUF_SPEC_WHEN_STRUCT), gShellDebug1HiiHandle);
    return ;
  }
  //
  // Get string and copy to buffer.
  // Caller should provide the buffer.
  //
  String = LibGetSmbiosString (Smbios, StringNumber);
  if (String != NULL) {
    CopyMem (Buffer, String, AsciiStrLen(String));
  } else {
    Buffer = NULL;
  }
}
