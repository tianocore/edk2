/** @file
  Header file for the SmbiosMisc Driver.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_MISC_H_
#define SMBIOS_MISC_H_

#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/HiiLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/OemMiscLib.h>
#include <Library/PcdLib.h>
#include <Library/PrintLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Guid/DebugMask.h>


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
  // intermediate input data for SMBIOS record
  //
  VOID                              *RecordData;
  EFI_MISC_SMBIOS_DATA_FUNCTION     *Function;
} EFI_MISC_SMBIOS_DATA_TABLE;


//
// SMBIOS table extern definitions
//
#define MISC_SMBIOS_TABLE_EXTERNS(NAME1, NAME2, NAME3) \
extern NAME1 NAME2 ## Data; \
extern EFI_MISC_SMBIOS_DATA_FUNCTION NAME3 ## Function;


//
// SMBIOS data table entries
//
// This is used to define a pair of table structure pointer and functions
// in order to iterate through the list of tables, populate them and add
// them into the system.
#define MISC_SMBIOS_TABLE_ENTRY_DATA_AND_FUNCTION(NAME1, NAME2) \
{ \
  & NAME1 ## Data, \
    NAME2 ## Function \
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
// Data Table Array Entries
//
extern EFI_HII_HANDLE               mHiiHandle;

typedef struct _EFI_TYPE13_BIOS_LANGUAGE_INFORMATION_STRING{
  UINT8                               *LanguageSignature;
  EFI_STRING_ID                       InstallableLanguageLongString;
  EFI_STRING_ID                       InstallableLanguageAbbreviateString;
} EFI_TYPE13_BIOS_LANGUAGE_INFORMATION_STRING;


/**
 Logs SMBIOS record.

 @param [in]   Buffer         Pointer to the data buffer.
 @param [in]   SmbiosHandle   Pointer for retrieve handle.

**/
EFI_STATUS
LogSmbiosData (
  IN       UINT8                      *Buffer,
  IN  OUT  EFI_SMBIOS_HANDLE          *SmbiosHandle
  );

/**
 Get Link Type Handle.

 @param [in]   SmbiosType     Get this Type from SMBIOS table
 @param [out]  HandleArray    Pointer to handle array which will be freed by caller
 @param [out]  HandleCount    Pointer to handle count

**/
VOID
GetLinkTypeHandle(
  IN  UINT8                 SmbiosType,
  OUT UINT16                **HandleArray,
  OUT UINTN                 *HandleCount
  );

//
// Data Table Array
//
extern EFI_MISC_SMBIOS_DATA_TABLE   mSmbiosMiscDataTable[];

//
// Data Table Array Entries
//
extern UINTN   mSmbiosMiscDataTableEntries;
extern UINT8   SmbiosMiscDxeStrings[];

#endif // SMBIOS_MISC_H_
