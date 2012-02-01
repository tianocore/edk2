/** @file
  Provides library functions for common SMBIOS operations. Only available to DXE
  and UEFI module types.


Copyright (c) 2012, Apple Inc. All rights reserved.
Portitions Copyright (c) 2006 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SMBIOS_LIB_H__
#define _SMBIOS_LIB_H__

#include <IndustryStandard/SmBios.h>
#include <Protocol/Smbios.h>


///
/// Cache copy of the SMBIOS Protocol pointer
///
extern EFI_SMBIOS_PROTOCOL *gSmbios;


///
/// Template for SMBIOS table initialization.
/// The SMBIOS_TABLE_STRING types in the formated area must match the 
/// StringArray sequene. 
///
typedef struct {
  //
  // formatted area of a given SMBIOS record
  //
  SMBIOS_STRUCTURE    *Entry;
  //
  // NULL terminated array of ASCII strings to be added to the SMBIOS record. 
  //
  CHAR8               **StringArray;
} SMBIOS_TEMPLATE_ENTRY;


/**
  Create an initial SMBIOS Table from an array of SMBIOS_TEMPLATE_ENTRY 
  entries. SMBIOS_TEMPLATE_ENTRY.NULL indicates the end of the table.

  @param  Template   Array of SMBIOS_TEMPLATE_ENTRY entries.
 
  @retval EFI_SUCCESS          New SMBIOS tables were created.
  @retval EFI_OUT_OF_RESOURCES New SMBIOS tables were not created. 
**/
EFI_STATUS
EFIAPI
SmbiosLibInitializeFromTemplate (
  IN  SMBIOS_TEMPLATE_ENTRY   *Template
  );



/**
  Create SMBIOS record.

  Converts a fixed SMBIOS structure and an array of pointers to strings into
  an SMBIOS record where the strings are cat'ed on the end of the fixed record
  and terminated via a double NULL and add to SMBIOS table.

  SMBIOS_TABLE_TYPE32 gSmbiosType12 = {
    { EFI_SMBIOS_TYPE_SYSTEM_CONFIGURATION_OPTIONS, sizeof (SMBIOS_TABLE_TYPE12), 0 },
    1 // StringCount
  };
  CHAR8 *gSmbiosType12Strings[] = {
    "Not Found",
    NULL
  };
  
  ...
  AddSmbiosEntryFromTemplate (
    (EFI_SMBIOS_TABLE_HEADER*)&gSmbiosType12, 
    gSmbiosType12Strings
    );

  @param  SmbiosEntry   Fixed SMBIOS structure
  @param  StringArray   Array of strings to convert to an SMBIOS string pack. 
                        NULL is OK.

  @retval EFI_SUCCESS          New SmbiosEntry was added to SMBIOS table.
  @retval EFI_OUT_OF_RESOURCES SmbiosEntry was not added.
**/
EFI_STATUS
EFIAPI
SmbiosLibCreateEntry (
  IN  SMBIOS_STRUCTURE *SmbiosEntry,
  IN  CHAR8            **StringArray 
  );


/**
  Update the string associated with an existing SMBIOS record.
  
  This function allows the update of specific SMBIOS strings. The number of valid strings for any
  SMBIOS record is defined by how many strings were present when Add() was called.
  
  @param[in]    SmbiosHandle    SMBIOS Handle of structure that will have its string updated.
  @param[in]    StringNumber    The non-zero string number of the string to update.
  @param[in]    String          Update the StringNumber string with String.
  
  @retval EFI_SUCCESS           SmbiosHandle had its StringNumber String updated.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not exist. Or String is invalid.
  @retval EFI_UNSUPPORTED       String was not added because it is longer than the SMBIOS Table supports.
  @retval EFI_NOT_FOUND         The StringNumber.is not valid for this SMBIOS record.    
**/
EFI_STATUS
EFIAPI
SmbiosLibUpdateString (
  IN  EFI_SMBIOS_HANDLE     SmbiosHandle,
  IN  SMBIOS_TABLE_STRING   StringNumber,
  IN  CHAR8                 *String
  );

/**
  Update the string associated with an existing SMBIOS record.
  
  This function allows the update of specific SMBIOS strings. The number of valid strings for any
  SMBIOS record is defined by how many strings were present when Add() was called.
  
  @param[in]    SmbiosHandle    SMBIOS Handle of structure that will have its string updated.
  @param[in]    StringNumber    The non-zero string number of the string to update.
  @param[in]    String          Update the StringNumber string with String.
  
  @retval EFI_SUCCESS           SmbiosHandle had its StringNumber String updated.
  @retval EFI_INVALID_PARAMETER SmbiosHandle does not exist. Or String is invalid.
  @retval EFI_UNSUPPORTED       String was not added because it is longer than the SMBIOS Table supports.
  @retval EFI_NOT_FOUND         The StringNumber.is not valid for this SMBIOS record.    
**/
EFI_STATUS
EFIAPI
SmbiosLibUpdateUnicodeString (
  IN  EFI_SMBIOS_HANDLE     SmbiosHandle,
  IN  SMBIOS_TABLE_STRING   StringNumber,
  IN  CHAR16                *String
  );

/**
  Allow caller to read a specific SMBIOS string
  
  @param[in]    Header          SMBIOS record that contains the string. 
  @param[in[    StringNumber    Instance of SMBIOS string 1 - N. 

  @retval NULL                  Instance of Type SMBIOS string was not found. 
  @retval Other                 Pointer to matching SMBIOS string. 
**/
CHAR8 *
EFIAPI
SmbiosLibReadString (
  IN SMBIOS_STRUCTURE   *Header,
  IN EFI_SMBIOS_STRING  StringNumber
  );


/**
  Allow the caller to discover a specific SMBIOS entry, and patch it if necissary. 
  
  @param[in]    Type            Type of the next SMBIOS record to return. 
  @param[in[    Instance        Instance of SMBIOS record 0 - N-1. 
  @param[out]   SmbiosHandle    Returns SMBIOS handle for the matching record. 

  @retval NULL                  Instance of Type SMBIOS record was not found. 
  @retval Other                 Pointer to matching SMBIOS record. 
**/
SMBIOS_STRUCTURE *
EFIAPI
SmbiosLibGetRecord (
  IN  EFI_SMBIOS_TYPE   Type,
  IN  UINTN             Instance,
  OUT EFI_SMBIOS_HANDLE *SmbiosHandle
  );

/**
  Remove an SMBIOS record.
  
  This function removes an SMBIOS record using the handle specified by SmbiosHandle.
  
  @param[in]    SmbiosHandle        The handle of the SMBIOS record to remove.
  
  @retval EFI_SUCCESS               SMBIOS record was removed.
  @retval EFI_INVALID_PARAMETER     SmbiosHandle does not specify a valid SMBIOS record.
**/
EFI_STATUS
EFIAPI
SmbiosLibRemove (
  OUT EFI_SMBIOS_HANDLE SmbiosHandle
  );




#endif
