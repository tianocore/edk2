/** @file
  Header file for the SmbiosMisc Driver.

  Based on files under Nt32Pkg/MiscSubClassPlatformDxe/

  Copyright (c) 2021, NUVIA Inc. All rights reserved.<BR>
  Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2015, Hisilicon Limited. All rights reserved.<BR>
  Copyright (c) 2015, Linaro Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef SMBIOS_MISC_H_
#define SMBIOS_MISC_H_

#include <Protocol/Smbios.h>
#include <IndustryStandard/SmBios.h>

//
// Data table entry update function.
//
typedef EFI_STATUS (EFIAPI SMBIOS_MISC_DATA_FUNCTION) (
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
  VOID                          *RecordData;
  SMBIOS_MISC_DATA_FUNCTION     *Function;
} SMBIOS_MISC_DATA_TABLE;


//
// SMBIOS table extern definitions
//
#define SMBIOS_MISC_TABLE_EXTERNS(NAME1, NAME2, NAME3) \
extern NAME1 NAME2 ## Data; \
extern SMBIOS_MISC_DATA_FUNCTION NAME3 ## Function;


//
// SMBIOS data table entries
//
// This is used to define a pair of table structure pointer and functions
// in order to iterate through the list of tables, populate them and add
// them into the system.
#define SMBIOS_MISC_TABLE_ENTRY_DATA_AND_FUNCTION(NAME1, NAME2) \
{ \
  & NAME1 ## Data, \
    NAME2 ## Function \
}

//
// Global definition macros.
//
#define SMBIOS_MISC_TABLE_DATA(NAME1, NAME2) \
  NAME1 NAME2 ## Data

#define SMBIOS_MISC_TABLE_FUNCTION(NAME2) \
  EFI_STATUS EFIAPI NAME2 ## Function( \
  IN  VOID                  *RecordData, \
  IN  EFI_SMBIOS_PROTOCOL   *Smbios \
  )

//
// Data Table Array Entries
//
extern EFI_HII_HANDLE               mSmbiosMiscHiiHandle;

typedef struct _SMBIOS_TYPE13_BIOS_LANGUAGE_INFORMATION_STRING{
  UINT8                               *LanguageSignature;
  EFI_STRING_ID                       InstallableLanguageLongString;
  EFI_STRING_ID                       InstallableLanguageAbbreviateString;
} SMBIOS_TYPE13_BIOS_LANGUAGE_INFORMATION_STRING;


/**
  Adds an SMBIOS record.

  @param  Buffer        The data for the SMBIOS record.
                        The format of the record is determined by
                        EFI_SMBIOS_TABLE_HEADER.Type. The size of the
                        formatted area is defined by EFI_SMBIOS_TABLE_HEADER.Length
                        and either followed by a double-null (0x0000) or a set
                        of null terminated strings and a null.
  @param  SmbiosHandle  A unique handle will be assigned to the SMBIOS record
                        if not NULL.

  @retval EFI_SUCCESS           Record was added.
  @retval EFI_OUT_OF_RESOURCES  Record was not added due to lack of system resources.
  @retval EFI_ALREADY_STARTED   The SmbiosHandle passed in was already in use.

**/
EFI_STATUS
SmbiosMiscAddRecord (
  IN       UINT8                      *Buffer,
  IN  OUT  EFI_SMBIOS_HANDLE          *SmbiosHandle OPTIONAL
  );

/**
 Get Link Type Handle.

 @param [in]   SmbiosType     Get this Type from SMBIOS table
 @param [out]  HandleArray    Pointer to handle array which will be freed by caller
 @param [out]  HandleCount    Pointer to handle count

**/
VOID
SmbiosMiscGetLinkTypeHandle(
  IN  UINT8                 SmbiosType,
  OUT UINT16                **HandleArray,
  OUT UINTN                 *HandleCount
  );

//
// Data Table Array
//
extern SMBIOS_MISC_DATA_TABLE   mSmbiosMiscDataTable[];

//
// Data Table Array Entries
//
extern UINTN   mSmbiosMiscDataTableEntries;
extern UINT8   mSmbiosMiscDxeStrings[];

#endif // SMBIOS_MISC_H_
