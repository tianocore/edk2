/**@file

Copyright (c) 2006 - 2009, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  MiscSubclassDriver.h

Abstract:

  Header file for MiscSubclass Driver.

**/

#ifndef _MISC_SUBCLASS_DRIVER_H
#define _MISC_SUBCLASS_DRIVER_H

#include <FrameworkDxe.h>
#include <WinNtDxe.h>
#include <Guid/DataHubRecords.h>
#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>
#include <Protocol/WinNtIo.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PcdLib.h>
#include <Library/MemoryAllocationLib.h>
#include <MiscDevicePath.h>


//
// Data table entry update function.
//
typedef EFI_STATUS (EFIAPI EFI_MISC_SMBIOS_DATA_FUNCTION) (
  IN  VOID                 *RecordData,
  IN  EFI_SMBIOS_PROTOCOL  *Smbios
  );

//
// Data table entry definition.
//
typedef struct {
  //
  // intermediat input data for SMBIOS record
  //
  VOID                              *RecordData;
  EFI_MISC_SMBIOS_DATA_FUNCTION     *Function;
} EFI_MISC_SMBIOS_DATA_TABLE;

//
// Data Table extern definitions.
//
#define MISC_SMBIOS_TABLE_EXTERNS(NAME1, NAME2, NAME3) \
extern NAME1 NAME2 ## Data; \
extern EFI_MISC_SMBIOS_DATA_FUNCTION NAME3 ## Function


//
// Data Table entries
//
#define MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(NAME1, NAME2) \
{ \
  & NAME1 ## Data, \
  & NAME2 ## Function \
}

//
// Global definition macros.
//
#define MISC_SMBIOS_TABLE_DATA(NAME1, NAME2) \
  NAME1 NAME2 ## Data

#define MISC_SMBIOS_TABLE_FUNCTION(NAME2) \
  EFI_STATUS EFIAPI NAME2 ## Function( \
  IN  VOID                  *RecordData, \
  IN  EFI_SMBIOS_PROTOCOL   *Smbios \
  )


//
// Data Table Array
//
extern EFI_MISC_SMBIOS_DATA_TABLE mMiscSubclassDataTable[];

//
// Data Table Array Entries
//
extern UINTN                        mMiscSubclassDataTableEntries;
extern UINT8                        MiscSubclassStrings[];
extern EFI_HII_HANDLE               mHiiHandle;

//
// Prototypes
//
EFI_STATUS
MiscSubclassDriverEntryPoint (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  );


#endif /* _MISC_SUBCLASS_DRIVER_H */

/* eof - MiscSubclassDriver.h */
