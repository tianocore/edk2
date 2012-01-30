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

#include <IndustryStandard/Smbios.h>
#include <Protocol/Smbios.h>


typedef struct {
  SMBIOS_STRUCTURE    *Entry;
  CHAR8               **StringArray;
} SMBIOS_TEMPLATE_ENTRY;


EFI_STATUS
InitializeSmbiosTableFromTemplate (
  IN  SMBIOS_TEMPLATE_ENTRY   *template
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

**/
EFI_STATUS
CreateSmbiosEntry (
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
SmbiosUpdateString (
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
SmbiosUpdateUnicodeString (
  IN  EFI_SMBIOS_HANDLE     SmbiosHandle,
  IN  SMBIOS_TABLE_STRING   StringNumber,
  IN  CHAR16                *String
  );

/**
  Allow caller to read a specific SMBIOS string
  
  @param[in]    Header          SMBIOS record that contains the string. 
  @param[in[    Intance         Instance of SMBIOS string 0 - N-1. 

  @retval NULL                  Instance of Type SMBIOS string was not found. 
  @retval Other                 Pointer to matching SMBIOS string. 
**/
CHAR8 *
SmbiosReadString (
  IN SMBIOS_STRUCTURE *Header,
  IN UINTN            Instance
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
SmbiosGetRecord (
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
SmbiosRemove (
  OUT EFI_SMBIOS_HANDLE SmbiosHandle
  );


EFI_STATUS
EFIAPI
SmbiosGetVersion (
  OUT UINT8   *SmbiosMajorVersion,
  OUT UINT8   *SmbiosMinorVersion
  );


#endif
