/** @file
  API for SMBIOS Plug and Play functions, access to SMBIOS table and structures.

  Copyright (c) 2005 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_SMBIOS_VIEW_H_
#define _LIB_SMBIOS_VIEW_H_

#include "LibSmbios.h"

#define DMI_SUCCESS                     0x00
#define DMI_UNKNOWN_FUNCTION            0x81
#define DMI_FUNCTION_NOT_SUPPORTED      0x82
#define DMI_INVALID_HANDLE              0x83
#define DMI_BAD_PARAMETER               0x84
#define DMI_INVALID_SUBFUNCTION         0x85
#define DMI_NO_CHANGE                   0x86
#define DMI_ADD_STRUCTURE_FAILED        0x87
#define DMI_READ_ONLY                   0x8D
#define DMI_LOCK_NOT_SUPPORTED          0x90
#define DMI_CURRENTLY_LOCKED            0x91
#define DMI_INVALID_LOCK                0x92

#define INVALIDE_HANDLE                 (UINT16) (-1)

#define EFI_SMBIOSERR(val)              EFIERR (0x30000 | val)

#define EFI_SMBIOSERR_FAILURE           EFI_SMBIOSERR (1)
#define EFI_SMBIOSERR_STRUCT_NOT_FOUND  EFI_SMBIOSERR (2)
#define EFI_SMBIOSERR_TYPE_UNKNOWN      EFI_SMBIOSERR (3)
#define EFI_SMBIOSERR_UNSUPPORTED       EFI_SMBIOSERR (4)

/**
  Init the SMBIOS VIEW API's environment.

  @retval EFI_SUCCESS  Successful to init the SMBIOS VIEW Lib.
**/
EFI_STATUS
LibSmbiosInit (
  VOID
  );

/**
  Cleanup the Smbios information.
**/
VOID
LibSmbiosCleanup (
  VOID
  );

/**
  Get the entry point structure for the table.

  @param[out] EntryPointStructure  The pointer to populate.
**/
VOID
LibSmbiosGetEPS (
  OUT SMBIOS_STRUCTURE_TABLE **EntryPointStructure
  );

/**
    Get SMBIOS structure given the Handle,copy data to the Buffer,
    Handle is changed to the next handle or 0xFFFF when the end is
    reached or the handle is not found.

    @param[in,out] Handle     0xFFFF: get the first structure
                              Others: get a structure according to this value.
    @param[in,out] Buffer     The pointer to the caller's memory buffer.
    @param[out] Length        Length of return buffer in bytes.

    @retval DMI_SUCCESS   Buffer contains the required structure data
                          Handle is updated with next structure handle or
                          0xFFFF(end-of-list).

    @retval DMI_INVALID_HANDLE  Buffer not contain the requiring structure data.
                                Handle is updated with next structure handle or
                                0xFFFF(end-of-list).
**/
EFI_STATUS
LibGetSmbiosStructure (
  IN  OUT UINT16  *Handle,
  IN  OUT UINT8   *Buffer,
  OUT UINT16      *Length
  );

/**
  Get a string from the smbios information.

  @param[in] Smbios         The pointer to the smbios information.
  @param[in] StringNumber   The index to the string to get.
  @param[out] Buffer        The buffer to fill with the string when retrieved.
**/
VOID
SmbiosGetPendingString (
  IN  SMBIOS_STRUCTURE_POINTER      *Smbios,
  IN  UINT16                        StringNumber,
  OUT CHAR8                         *Buffer
  );

/**
  Check the structure to see if it is legal.

  @param[in] Smbios    - Pointer to the structure that will be checked.

  @retval DMI_SUCCESS           Structure data is legal.
  @retval DMI_BAD_PARAMETER     Structure data contains bad parameter.
**/
EFI_STATUS
SmbiosCheckStructure (
  IN  SMBIOS_STRUCTURE_POINTER *Smbios
  );

#endif
