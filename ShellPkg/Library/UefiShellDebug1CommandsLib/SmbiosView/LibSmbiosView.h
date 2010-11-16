/** @file
  API for SMBIOS Plug and Play functions, access to SMBIOS table and structures.

  Copyright (c) 2005 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _LIB_SMBIOS_VIEW_H
#define _LIB_SMBIOS_VIEW_H

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

EFI_STATUS
LibSmbiosInit (
  VOID
  );

VOID
LibSmbiosCleanup (
  VOID
  );

VOID
LibSmbiosGetEPS (
  SMBIOS_STRUCTURE_TABLE **pEntryPointStructure
  );

VOID
LibSmbiosGetStructHead (
  SMBIOS_STRUCTURE_POINTER *pHead
  );

EFI_STATUS
LibGetSmbiosInfo (
  OUT CHAR8   *dmiBIOSRevision,
  OUT UINT16  *NumStructures,
  OUT UINT16  *StructureSize,
  OUT UINT32  *dmiStorageBase,
  OUT UINT16  *dmiStorageSize
  );

/*++
  Description:
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
EFI_STATUS
LibGetSmbiosStructure (
  IN  OUT UINT16  *Handle,
  IN  OUT UINT8   *Buffer,
  OUT UINT16      *Length
  );

/*++
  Description:
    Get SMBIOS structure given the Handle,copy data to the Buffer,Handle is then the next.

  Arguments:
    Handle:         - 0x0: get the first structure
                    - Others: get a certain structure according to this value.
    Buffter:        - contains the pointer to the caller's memory buffer.

  Returns:
    DMI_SUCCESS         - Buffer contains the required structure data
                        - Handle is updated with next structure handle or 0xFFFF(end-of-list).
    DMI_INVALID_HANDLE  - Buffer not contain the requiring structure data
                        - Handle is updated with next structure handle or 0xFFFF(end-of-list).
**/
VOID
SmbiosGetPendingString (
  IN  SMBIOS_STRUCTURE_POINTER      *Smbios,
  IN  UINT16                        StringNumber,
  OUT CHAR8                         *Buffer
  );

EFI_STATUS
SmbiosCheckStructure (
  IN  SMBIOS_STRUCTURE_POINTER      *Smbios
  );

#endif
