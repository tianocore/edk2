/** @file
  HII Library implementation that uses DXE protocols and services.

  Copyright (c) 2006 - 2008, Intel Corporation<BR>
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Protocol/DevicePath.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiString.h>
#include <Protocol/FormBrowser2.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiHiiServicesLib.h>
#include <Library/DevicePathLib.h>
#include <Library/PrintLib.h>
#include <Library/NewHiiLib.h>
#include <Library/UefiLib.h>

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// HiiLib Functions
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

//
// Template used to mark the end of a list of packages 
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST EFI_HII_PACKAGE_HEADER  mEndOfPakageList = {
  sizeof (EFI_HII_PACKAGE_HEADER),
  EFI_HII_PACKAGE_END
};

//
// <ConfigHdr> Template
//
GLOBAL_REMOVE_IF_UNREFERENCED CONST CHAR16 mConfigHdrTemplate[] = L"GUID=00000000000000000000000000000000&NAME=0000&PATH=00";

//
// Form Browser2 Protocol
//
EFI_FORM_BROWSER2_PROTOCOL  *mFormBrowser2 = NULL;

/**
  Registers a list of packages in the HII Database and returns the HII Handle
  associated with that registration.  If an HII Handle has already been registered
  with the same PackageListGuid, then NULL is returned.  If there are not enough 
  resources to perform the registration, then NULL is returned.  If an empty list 
  of packages is passed in, then NULL is returned.  If the size of the list of 
  package is 0, then NULL is returned.

  @param[in]  PackageListGuid  An optional parameter that is used to identify 
                               the GUID of the package list.  If this parameter 
                               is NULL, then gEfiCallerIdGuid is used.
  @param[in]  DeviceHandle     Optional. If not NULL, the Device Handle on which 
                               an instance of DEVICE_PATH_PROTOCOL is installed.
                               This Device Handle uniquely defines the device that 
                               the added packages are associated with.
  @param[in]  ...              The variable argument list that contains pointers 
                               to packages terminated by a NULL.

  @retval NULL   A HII Handle has already been registered in the HII Database with
                 the same PackageListGuid.
  @retval NULL   The HII Handle could not be created.
  @retval Other  The HII Handle associated with the newly registered package list.

**/
EFI_HII_HANDLE
EFIAPI
HiiAddPackages (
  IN CONST EFI_GUID    *PackageListGuid,  OPTIONAL
  IN       EFI_HANDLE  DeviceHandle,      OPTIONAL
  ...
  )
{
  EFI_STATUS                   Status;
  EFI_HII_HANDLE               *HiiHandleBuffer;
  VA_LIST                      Args;
  UINT32                       *Package;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageListHeader;
  EFI_HII_HANDLE               HiiHandle;
  UINTN                        Length;
  UINT8                        *Data;

  //
  // If PackageListGuid is NULL, then use gEfiCallerIdGuid as the PackageListGuid
  //
  if (PackageListGuid == NULL) {
    PackageListGuid = &gEfiCallerIdGuid;
  }

  //
  // Check to see if an HII Handle has already been registered with the same 
  // PackageListGuid
  //
  HiiHandleBuffer = HiiGetHiiHandles (PackageListGuid);
  if (HiiHandleBuffer != NULL) {
    FreePool (HiiHandleBuffer);
    return NULL;
  }

  //
  // Calculate the length of all the packages in the variable argument list
  //
  for (Length = 0, VA_START (Args, DeviceHandle); (Package = VA_ARG (Args, UINT32 *)) != NULL; ) {
    Length += (ReadUnaligned32 ((UINT32 *)Package) - sizeof (UINT32));
  }
  VA_END (Args);

  //
  // If there are no packages in the variable argument list or all the packages 
  // are empty, then return a NULL HII Handle
  //
  if (Length == 0) {
    return NULL;
  }

  //
  // Add the length of the Package List Header and the terminating Package Header 
  //
  Length += sizeof (EFI_HII_PACKAGE_LIST_HEADER) + sizeof (EFI_HII_PACKAGE_HEADER);

  //
  // Allocate the storage for the entire Package List
  //
  PackageListHeader = AllocateZeroPool (Length);

  //
  // If the Packahge List can not be allocated, then return a NULL HII Handle
  //
  if (PackageListHeader == NULL) {
    return NULL;
  }

  //
  // Fill in the GUID and Length of the Package List Header
  //
  CopyGuid (&PackageListHeader->PackageListGuid, PackageListGuid);
  PackageListHeader->PackageLength = Length;

  //
  // Initialize a pointer to the beginning if the Package List data
  //
  Data = (UINT8 *)(PackageListHeader + 1);

  //
  // Copy the data from each package in the variable argument list
  //
  for (VA_START (Args, DeviceHandle); (Package = VA_ARG (Args, UINT32 *)) != NULL; ) {
    Length = ReadUnaligned32 ((UINT32 *)Package) - sizeof (UINT32);
    CopyMem (Data, Package + 1, Length);
    Data += Length;
  }
  VA_END (Args);

  //
  // Append a package of type EFI_HII_PACKAGE_END to mark the end of the package list
  //
  CopyMem (Data, &mEndOfPakageList, sizeof (mEndOfPakageList));

  //
  // Register the package list with the HII Database
  //
  Status = gHiiDatabase->NewPackageList (
                           gHiiDatabase, 
                           PackageListHeader, 
                           DeviceHandle, 
                           &HiiHandle
                           );
  if (EFI_ERROR (Status)) {
    HiiHandle = NULL;
  }

  //
  // Free the allocated package list
  //
  FreePool (PackageListHeader);

  //
  // Return the new HII Handle
  //
  return HiiHandle;
}

/**
  Removes a package list from the HII Database.

  If HiiHandle is NULL, then ASSERT().
  If HiiHandle is not a valid EFI_HII_HANDLE in the HII Database, then ASSERT().

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.

**/
VOID
EFIAPI
HiiRemovePackages (
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STATUS Status;

  //
  // ASSERT if HiiHandle is NULL
  //
  ASSERT (HiiHandle != NULL);

  //
  // Remove the package list specific by HiiHandle from the HII Database
  //
  Status = gHiiDatabase->RemovePackageList (gHiiDatabase, HiiHandle);

  //
  // ASSERT if the remove request fails.  Should only occur if the HiiHandle is not valid.
  //
  ASSERT_EFI_ERROR (Status);
}

/**
  Retrieves the array of all the HII Handles in the HII Database.
  This array is terminated with a NULL HII Handle.
  This function allocates the returned array using AllocatePool().
  The caller is responsible for freeing the array with FreePool().

  @param[in]  PackageListGuid  An optional parameter that is used to request 
                               an HII Handle that is associatd with a specific
                               Package List GUID.  If this parameter is NULL
                               then all the HII Handles in the HII Database
                               are returned.  If this parameter is not NULL
                               then at most 1 HII Handle is returned.

  @retval NULL   There are no HII handles in the HII database
  @retval NULL   The array of HII Handles could not be retrieved
  @retval Other  A pointer to the NULL terminated array of HII Handles

**/
EFI_HII_HANDLE *
EFIAPI
HiiGetHiiHandles (
  IN CONST EFI_GUID  *PackageListGuid  OPTIONAL
  )
{
  EFI_STATUS      Status;
  UINT8           PackageType;
  UINTN           HandleBufferLength;
  EFI_HII_HANDLE  TempHiiHandleBuffer;
  EFI_HII_HANDLE  *HiiHandleBuffer;

  //
  // Determine the PackageType for the ListPackageLists() request
  //
  if (PackageListGuid == NULL) {
    PackageType = EFI_HII_PACKAGE_TYPE_ALL;
  } else {
    PackageType = EFI_HII_PACKAGE_TYPE_GUID;
  }

  //
  // Retrieve the size required for the buffer of all HII handles.
  //
  HandleBufferLength = 0;
  Status = gHiiDatabase->ListPackageLists (
                           gHiiDatabase,
                           PackageType,
                           PackageListGuid,
                           &HandleBufferLength,
                           &TempHiiHandleBuffer
                           );

  //
  // If ListPackageLists() returns EFI_SUCCESS for a zero size, 
  // then there are no HII handles in the HII database.  If ListPackageLists() 
  // returns an error other than EFI_BUFFER_TOO_SMALL, then there are no HII 
  // handles in the HII database.
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    //
    // Return NULL if the size can not be retrieved, or if there are no HII 
    // handles in the HII Database
    //
    return NULL;
  }

  //
  // Allocate the array of HII handles to hold all the HII Handles and a NULL terminator
  //
  HiiHandleBuffer = AllocateZeroPool (HandleBufferLength + sizeof (EFI_HII_HANDLE));
  if (HiiHandleBuffer == NULL) {
    //
    // Return NULL if allocation fails.
    //
    return NULL;
  }

  //
  // Retrieve the array of HII Handles in the HII Database
  //
  Status = gHiiDatabase->ListPackageLists (
                           gHiiDatabase,
                           PackageType,
                           PackageListGuid,
                           &HandleBufferLength,
                           HiiHandleBuffer
                           );
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the HII handles can not be retrieved.
    //
    FreePool (HiiHandleBuffer);
    return NULL;
  }

  //
  // Return the NULL terminated array of HII handles in the HII Database
  //
  return HiiHandleBuffer;
}

/**
  Retrieves a pointer to the a Null-terminated ASCII string containing the list 
  of languages that an HII handle in the HII Database supports.  The returned 
  string is allocated using AllocatePool().  The caller is responsible for freeing
  the returned string using FreePool().  The format of the returned string follows
  the language format assumed the HII Database.
  
  If HiiHandle is NULL, then ASSERT().

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.

  @retval NULL   HiiHandle is not registered in the HII database
  @retval NULL   There are not enough resources available to retrieve the suported 
                 languages.
  @retval NULL   The list of suported languages could not be retrieved.
  @retval Other  A pointer to the Null-terminated ASCII string of supported languages.

**/
CHAR8 *
EFIAPI
HiiGetSupportedLanguages (
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  EFI_STATUS  Status;
  UINTN       LanguageSize;
  CHAR8       TempSupportedLanguages;
  CHAR8       *SupportedLanguages;

  //
  // ASSERT if HiiHandle is NULL
  //
  ASSERT (HiiHandle != NULL);

  //
  // Retrieve the size required for the supported languages buffer.
  //
  LanguageSize = 0;
  Status = gHiiString->GetLanguages (gHiiString, HiiHandle, &TempSupportedLanguages, &LanguageSize);

  //
  // If GetLanguages() returns EFI_SUCCESS for a zero size, 
  // then there are no supported languages registered for HiiHandle.  If GetLanguages() 
  // returns an error other than EFI_BUFFER_TOO_SMALL, then HiiHandle is not present
  // in the HII Database
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    //
    // Return NULL if the size can not be retrieved, or if HiiHandle is not in the HII Database
    //
    return NULL;
  }

  //
  // Allocate the supported languages buffer.
  //
  SupportedLanguages = AllocateZeroPool (LanguageSize);
  if (SupportedLanguages == NULL) {
    //
    // Return NULL if allocation fails.
    //
    return NULL;
  }

  //
  // Retrieve the supported languages string
  //
  Status = gHiiString->GetLanguages (gHiiString, HiiHandle, SupportedLanguages, &LanguageSize);
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the supported languages can not be retrieved.
    //
    FreePool (SupportedLanguages);
    return NULL;
  }

  //
  // Return the Null-terminated ASCII string of supported languages
  //
  return SupportedLanguages;
}

/**
  Retrieves a string from a string package in a specific language.  If the language
  is not specified, then a string from a string package in the current platform 
  language is retrieved.  If the string can not be retrieved using the specified 
  language or the current platform language, then the string is retrieved from 
  the string package in the first language the string package supports.  The 
  returned string is allocated using AllocatePool().  The caller is responsible 
  for freeing the allocated buffer using FreePool().
  
  If HiiHandle is NULL, then ASSERT().
  If StringId is 0, then ASSET.

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.
  @param[in]  StringId   The identifier of the string to retrieved from the string 
                         package associated with HiiHandle.
  @param[in]  Language   The language of the string to retrieve.  If this parameter 
                         is NULL, then the current platform language is used.  The 
                         format of Language must follow the language format assumed 
                         the HII Database.

  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
EFIAPI
HiiGetString (
  IN EFI_HII_HANDLE  HiiHandle,
  IN EFI_STRING_ID   StringId,
  IN CONST CHAR8     *Language  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       StringSize;
  CHAR16      TempString;
  EFI_STRING  String;
  CHAR8       *SupportedLanguages;
  CHAR8       *PlatformLanguage;
  CHAR8       *BestLanguage;

  //
  // If HiiHandle is NULL, then ASSERT()
  //
  ASSERT (HiiHandle !=NULL);

  //
  // If StringId is 0, then ASSERT()
  //
  ASSERT (StringId != 0);

  //
  // Initialize all allocated buffers to NULL
  // 
  SupportedLanguages = NULL;
  PlatformLanguage   = NULL;
  BestLanguage       = NULL;
  String             = NULL;

  //
  // Get the languages that the package specified by HiiHandle supports
  //
  SupportedLanguages = HiiGetSupportedLanguages (HiiHandle);
  if (SupportedLanguages == NULL) {
    goto Error;
  }

  //
  // Get the current platform language setting
  //
  PlatformLanguage = GetEfiGlobalVariable (L"PlatformLang");
  if (PlatformLanguage == NULL) {
    goto Error;
  }

  //
  // If Languag is NULL, then set it to an empty string, so it will be 
  // skipped by GetBestLanguage()
  //
  if (Language == NULL) {
    Language = "";
  }

  //
  // Get the best matching language from SupportedLanguages
  //
  BestLanguage = GetBestLanguage (
                  SupportedLanguages, 
                  FALSE,                // RFC 4646 mode
                  Language,             // Highest priority 
                  PlatformLanguage,     // Next highest priority
                  SupportedLanguages,   // Lowest priority 
                  NULL
                  );
  if (BestLanguage == NULL) {
    goto Error;
  }

  //
  // Retrieve the size of the string in the string package for the BestLanguage
  //
  StringSize = 0;
  Status = gHiiString->GetString (
                         gHiiString,
                         BestLanguage,
                         HiiHandle,
                         StringId,
                         &TempString,
                         &StringSize,
                         NULL
                         );
  //
  // If GetString() returns EFI_SUCCESS for a zero size, 
  // then there are no supported languages registered for HiiHandle.  If GetString() 
  // returns an error other than EFI_BUFFER_TOO_SMALL, then HiiHandle is not present
  // in the HII Database
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    goto Error;
  }

  //
  // Allocate a buffer for the return string
  //
  String = AllocateZeroPool (StringSize);
  if (String == NULL) {
    goto Error;
  }

  //
  // Retrieve the string from the string package
  //
  Status = gHiiString->GetString (
                         gHiiString,
                         BestLanguage,
                         HiiHandle,
                         StringId,
                         String,
                         &StringSize,
                         NULL
                         );
  if (EFI_ERROR (Status)) {
    //
    // Free the buffer and return NULL if the supported languages can not be retrieved.
    //
    FreePool (String);
    String = NULL;
  }

Error:
  //
  // Free allocated buffers
  //
  if (SupportedLanguages != NULL) {
    FreePool (SupportedLanguages);
  }
  if (PlatformLanguage != NULL) {
    FreePool (PlatformLanguage);
  }
  if (BestLanguage != NULL) {
    FreePool (BestLanguage);
  }

  //
  // Return the Null-terminated Unicode string
  //
  return String;
}

/**
  Retrieves a string from a string package names by GUID in a specific language.  
  If the language is not specified, then a string from a string package in the 
  current platform  language is retrieved.  If the string can not be retrieved 
  using the specified language or the current platform language, then the string 
  is retrieved from the string package in the first language the string package 
  supports.  The returned string is allocated using AllocatePool().  The caller 
  is responsible for freeing the allocated buffer using FreePool().
  
  If PackageListGuid is NULL, then ASSERT().
  If StringId is 0, then ASSET.

  @param[in]  PackageListGuid  The GUID of a package list that was previously 
                               registered in the HII Database.
  @param[in]  StringId         The identifier of the string to retrieved from the 
                               string package associated with PackageListGuid.
  @param[in]  Language         The language of the string to retrieve.  If this 
                               parameter is NULL, then the current platform 
                               language is used.  The format of Language must 
                               follow the language format assumed the HII Database.

  @retval NULL   The package list specified by PackageListGuid is not present in the
                 HII Database.
  @retval NULL   The string specified by StringId is not present in the string package.
  @retval Other  The string was returned.

**/
EFI_STRING
EFIAPI
HiiGetPackageString (
  IN CONST EFI_GUID  *PackageListGuid,
  IN EFI_STRING_ID   StringId,
  IN CONST CHAR8     *Language  OPTIONAL
  )
{
  EFI_HANDLE  *HiiHandleBuffer;
  EFI_HANDLE  HiiHandle;

  ASSERT (PackageListGuid != NULL);

  HiiHandleBuffer = HiiGetHiiHandles (PackageListGuid);
  if (HiiHandleBuffer == NULL) {
    return NULL;
  }
  HiiHandle = HiiHandleBuffer[0];
  FreePool (HiiHandleBuffer);
  if (HiiHandle == NULL) {
    return NULL;
  }
	return HiiGetString (HiiHandle, StringId, Language);
}

/**
  This function create a new string in String Package or updates an existing 
  string in a String Package.  If StringId is 0, then a new string is added to
  a String Package.  If StringId is not zero, then a string in String Package is
  updated.  If SupportedLanguages is NULL, then the string is added or updated
  for all the languages that the String Package supports.  If SupportedLanguages
  is not NULL, then the string is added or updated for the set of languages 
  specified by SupportedLanguages.
    
  If HiiHandle is NULL, then ASSERT().
  If String is NULL, then ASSERT().

  @param[in]  HiiHandle           A handle that was previously registered in the 
                                  HII Database.
  @param[in]  StringId            If zero, then a new string is created in the 
                                  String Package associated with HiiHandle.  If 
                                  non-zero, then the string specified by StringId 
                                  is updated in the String Package  associated 
                                  with HiiHandle. 
  @param[in]  String              A pointer to the Null-terminated Unicode string 
                                  to add or update in the String Package associated 
                                  with HiiHandle.
  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string of 
                                  language codes.  If this parameter is NULL, then 
                                  String is added or updated in the String Package 
                                  associated with HiiHandle for all the languages 
                                  that the String Package supports.  If this 
                                  parameter is not NULL, then then String is added 
                                  or updated in the String Package associated with 
                                  HiiHandle for the set oflanguages specified by 
                                  SupportedLanguages.  The format of 
                                  SupportedLanguages must follow the language 
                                  format assumed the HII Database.

  @retval 0      The string could not be added or updated in the String Package.
  @retval Other  The EFI_STRING_ID of the newly added or updated string.

**/
EFI_STRING_ID
EFIAPI
HiiSetString (
  IN EFI_HII_HANDLE    HiiHandle,
  IN EFI_STRING_ID     StringId,            OPTIONAL
  IN CONST EFI_STRING  String,
  IN CONST CHAR8       *SupportedLanguages  OPTIONAL
  )
{
  EFI_STATUS  Status;
  CHAR8       *AllocatedLanguages;
  CHAR8       *Supported;
  CHAR8       *Language;

  ASSERT (HiiHandle != NULL);
  ASSERT (String != NULL);

  if (SupportedLanguages == NULL) {
    //
    // Retrieve the languages that the package specified by HiiHandle supports
    //
    AllocatedLanguages = HiiGetSupportedLanguages (HiiHandle);
  } else {
    //
    // Allocate a copy of the SupportLanguages string that passed in
    //
    AllocatedLanguages = AllocateCopyPool (AsciiStrLen (SupportedLanguages), SupportedLanguages);
  }

  //
  // If there are not enough resources for the supported languages string, then return a StringId of 0
  //
  if (AllocatedLanguages == NULL) {
    return (EFI_STRING_ID)(0);
  }

  //
  // Loop through each language that the string supports
  //
  for (Supported = AllocatedLanguages; *Supported != '\0'; ) {
    //
    // Cache a pointer to the beginning of the current language in the list of languages
    //
    Language = Supported;

    //
    // Search for the next language seperator and replace it with a Null-terminator
    //
    for (; *Supported != 0 && *Supported != ';'; Supported++);
    *(Supported++) = '\0';

    //
    // If StringId is 0, then call NewString().  Otherwise, call SetString()
    //
    if (StringId == (EFI_STRING_ID)(0)) {
      Status = gHiiString->NewString (gHiiString, HiiHandle, &StringId, Language, NULL, String, NULL);
    } else {
      Status = gHiiString->SetString (gHiiString, HiiHandle, StringId, Language, String, NULL);
    }

    //
    // If there was an error, then break out of the loop and return a StringId of 0
    //
    if (EFI_ERROR (Status)) {
      StringId = (EFI_STRING_ID)(0);
      break;
    }
  }

  //
  // Free the buffer of supported languages
  //
  FreePool (AllocatedLanguages);

  //
  // Return the StringId of the new or updated string
  //
  return StringId;
}

/**
  Validates the config data associated with an HII handle in the HII Database.
    
  If HiiHandle is NULL, then ASSERT().

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.

  @retval TRUE   The config data associated with HiiHandle passes all validation
                 checks.
  @retval FALSE  The config data associated with HiiHandle failed one or more 
                 validation checks.

**/
BOOLEAN
EFIAPI
HiiValidateDataFromHiiHandle (
  IN EFI_HII_HANDLE  HiiHandle
  )
{
  ASSERT (HiiHandle != NULL);
  //
  // Needs to be implemented.
  //
  return TRUE;
}

/**
  Converts all hex dtring characters in range ['A'..'F'] to ['a'..'f'] for 
  hex digits that appear between a '=' and a '&' in a config string.

  If String is NULL, then ASSERT().

  @param[in] String  Pointer to a Null-terminated Unicode string.

  @return  Pointer to the Null-terminated Unicode result string.

**/
EFI_STRING
EFIAPI
InternalHiiLowerConfigString (
  IN EFI_STRING  ConfigString
  )
{
  EFI_STRING  String;
  BOOLEAN     Lower;

  ASSERT (ConfigString != NULL);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
    if (*String == L'=') {
      Lower = TRUE;
    } else if (*String == L'&') {
      Lower = FALSE;
    } else if (Lower && *String > L'A' && *String <= L'F') {
      *String = *String - L'A' + L'a';
    }
  }

  return ConfigString;
}

/**
  Uses the BlockToConfig() service of the Config Routing Protocol to 
  convert <ConfigRequest> and a buffer to a <ConfigResp>

  If ConfigRequest is NULL, then ASSERT().
  If Block is NULL, then ASSERT().

  @param[in] ConfigRequest  Pointer to a Null-terminated Unicode string.
  @param[in] Block          Pointer to a block of data.
  @param[in] BlockSize      The zie, in bytes, of Block.

  @retval NULL   The <ConfigResp> string could not be generated.
  @retval Other  Pointer to the Null-terminated Unicode <ConfigResp> string.

**/
EFI_STRING
EFIAPI
InternalHiiBlockToConfig (
  IN CONST EFI_STRING  ConfigRequest,
  IN CONST UINT8       *Block,
  IN UINTN             BlockSize
  )
{
  EFI_STATUS  Status;
  EFI_STRING  ConfigResp;
  CHAR16      *Progress;

  ASSERT (ConfigRequest != NULL);
  ASSERT (Block != NULL);

  //
  // Convert <ConfigRequest> to <ConfigResp>
  //
  Status = gHiiConfigRouting->BlockToConfig (
                                gHiiConfigRouting,
                                ConfigRequest,
                                Block,
                                BlockSize,
                                &ConfigResp,
                                &Progress
                                );
  if (EFI_ERROR (Status)) {
    return NULL;
  }
  return ConfigResp;
}

/**
  Uses the ConfigToBlock() service of the Config Routing Protocol to 
  convert <ConfigResp> to a block.  The block is allocated using
  AllocatePool().  The caller is responsible for freeing the block
  using FreePool().

  If ConfigResp is NULL, then ASSERT().

  @param[in] ConfigResp  Pointer to a Null-terminated Unicode string.
  @param[in] BufferSize  Length in bytes of buffer to hold retrived data. 

  @retval NULL   The block could not be generated..
  @retval Other  Pointer to the allocated block.

**/
UINT8 *
EFIAPI
InternalHiiConfigToBlock (
  IN  EFI_STRING  ConfigResp,
  IN  UINTN       BlockSize
  )
{
  EFI_STATUS  Status;
  CHAR16      *Progress;
  UINT8       *Block;

  ASSERT (ConfigResp != NULL);

  //
  // Allocate a buffer to hold the <ConfigResp> conversion
  //
  Block = AllocateZeroPool (BlockSize);
  if (Block == NULL) {
    return NULL;
  }

  //
  // Convert <ConfigResp> to a buffer
  //
  Status = gHiiConfigRouting->ConfigToBlock (
                                gHiiConfigRouting,
                                ConfigResp,
                                Block,
                                &BlockSize,
                                &Progress
                                );
  if (EFI_ERROR (Status)) {
    FreePool (Block);
    return NULL;
  }

  //
  // Return converted buffer
  //
  return Block;
}

/**
  Uses the BrowserCallback() service of the Form Browser Protocol to retrieve 
  or set uncommitted data.  If sata i being retrieved, then the buffer is 
  allocated using AllocatePool().  The caller is then responsible for freeing 
  the buffer using FreePool().

  @param[in]  VariableName    Pointer to a Null-terminated Unicode string.  This 
                              is an optional parameter that may be NULL.
  @param[in]  VariableGuid    Pointer to an EFI_GUID structure.  This is an optional 
                              parameter that may be NULL.
  @param[in]  SetResultsData  If not NULL, then this parameter specified the buffer
                              of uncommited data to set.  If this parameter is NULL,
                              then the caller is requesting to get the uncommited data
                              from the Form Browser.

  @retval NULL   The uncommitted data could not be retrieved.
  @retval Other  A pointer to a buffer containing the uncommitted data.

**/
EFI_STRING
EFIAPI
InternalHiiBrowserCallback (
  IN CONST EFI_GUID    *VariableGuid,  OPTIONAL
  IN CONST CHAR16      *VariableName,  OPTIONAL
  IN CONST EFI_STRING  SetResultsData  OPTIONAL
  )
{
  EFI_STATUS  Status;
  UINTN       ResultsDataSize;
  EFI_STRING  ResultsData;
  CHAR16      TempResultsData;

  //
  // Locate protocols
  //
  if (mFormBrowser2 == NULL) {
    Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &mFormBrowser2);
    if (EFI_ERROR (Status) || mFormBrowser2 == NULL) {
      return NULL;
    }
  }

  ResultsDataSize = 0;

  if (SetResultsData != NULL) {
    //
    // Request to to set data in the uncommitted browser state information
    //
    ResultsData = SetResultsData;
  } else {
    //
    // Retrieve the length of the buffer required ResultsData from the Browser Callback
    //
    Status = mFormBrowser2->BrowserCallback (
                              mFormBrowser2,
                              &ResultsDataSize,
                              &TempResultsData,
                              TRUE,
                              VariableGuid,
                              VariableName
                              );
    if (Status != EFI_BUFFER_TOO_SMALL) {
      return NULL;
    }

    //
    // Allocate the ResultsData buffer
    //
    ResultsData = AllocateZeroPool (ResultsDataSize);
    if (ResultsData == NULL) {
      return NULL;
    }
  }

  //
  // Retrieve or set the ResultsData from the Browser Callback
  //
  Status = mFormBrowser2->BrowserCallback (
                            mFormBrowser2,
                            &ResultsDataSize,
                            ResultsData,
                            (BOOLEAN)(SetResultsData == NULL),
                            VariableGuid,
                            VariableName
                            );
  if (EFI_ERROR (Status)) {
    return NULL;
  }

  return ResultsData;
}

/**
  Allocates and returns a Null-terminated Unicode <ConfigHdr> string using routing 
  information that includes a GUID, an optional Unicode string name, and a device
  path.  The string returned is allocated with AllocatePool().  The caller is 
  responsible for freeing the allocated string with FreePool().
  
  The format of a <ConfigHdr> is as follows:

    GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize<Null>

  @param[in]  Guid          Pointer to an EFI_GUID that is the routing information
                            GUID.  Each of the 16 bytes in Guid is converted to 
                            a 2 Unicode character hexidecimal string.  This is 
                            an optional parameter that may be NULL.
  @param[in]  Name          Pointer to a Null-terminated Unicode string that is 
                            the routing information NAME.  This is an optional 
                            parameter that may be NULL.  Each 16-bit Unicode 
                            character in Name is converted to a 4 character Unicode 
                            hexidecimal string.                        
  @param[in]  DriverHandle  The driver handle which supports a Device Path Protocol
                            that is the routing information PATH.  Each byte of
                            the Device Path associated with DriverHandle is converted
                            to a 2 Unicode character hexidecimal string.

  @retval NULL   DriverHandle does not support the Device Path Protocol.
  @retval NULL   DriverHandle does not support the Device Path Protocol.
  @retval Other  A pointer to the Null-terminate Unicode <ConfigHdr> string

**/
EFI_STRING
EFIAPI
HiiConstructConfigHdr (
  IN CONST EFI_GUID  *Guid,  OPTIONAL
  IN CONST CHAR16    *Name,  OPTIONAL
  IN EFI_HANDLE      DriverHandle
  )
{
  UINTN                     NameLength;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     DevicePathSize;
  CHAR16                    *String;
  CHAR16                    *ReturnString;
  UINTN                     Index;
  UINT8                     *Buffer;

  //
  // Compute the length of Name in Unicode characters.  
  // If Name is NULL, then the length is 0.
  //
  NameLength = 0;
  if (Name != NULL) {
    NameLength = StrLen (Name);
  }

  //
  // Retrieve DevicePath Protocol associated with DriverHandle
  //
  DevicePath = DevicePathFromHandle (DriverHandle);
  if (DevicePath == NULL) {
    return NULL;
  }

  //
  // Compute the size of the device path in bytes
  //
  DevicePathSize = GetDevicePathSize (DevicePath);

  //
  // GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize <Null>
  // | 5 | sizeof (EFI_GUID) * 2 | 6 | NameStrLen*4 | 6 | DevicePathSize * 2 | 1 |
  //
  String = AllocateZeroPool ((5 + sizeof (EFI_GUID) * 2 + 6 + NameLength * 4 + 6 + DevicePathSize * 2 + 1) * sizeof (CHAR16));
  if (String == NULL) {
    return NULL;
  }

  //
  // Start with L"GUID="
  //
  ReturnString = StrCpy (String, L"GUID=");
  String += StrLen (String);

  if (Guid != NULL) {
    //
    // Append Guid converted to <HexCh>32
    //
    for (Index = 0, Buffer = (UINT8 *)Guid; Index < sizeof (EFI_GUID); Index++) {
      String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, *(Buffer++), 2);
    }
  }
  
  //
  // Append L"&NAME="
  //
  StrCpy (String, L"&NAME=");
  String += StrLen (String);

  if (Name != NULL) {
    //
    // Append Name converted to <Char>NameLength
    //
    for (; *Name != L'\0'; Name++) {
      String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, *Name, 4);
    }
  }

  //
  // Append L"&PATH="
  //
  StrCpy (String, L"&PATH=");
  String += StrLen (String);

  //
  // Append the device path associated with DriverHandle converted to <HexChar>DevicePathSize
  //
  for (Index = 0, Buffer = (UINT8 *)DevicePath; Index < DevicePathSize; Index++) {
    String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, *(Buffer++), 2);
  }

  //
  // Null terminate the Unicode string
  //
  *String = L'\0';

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  return InternalHiiLowerConfigString (ReturnString);
}

/**
  Allocates and returns a Null-terminated Unicode <ConfigAltResp> string.

  If Guid is NULL, then ASSERT().
  If Name is NULL, then ASSERT().
  If BlockNameArray is NULL, then ASSERT().

  @param[in] Guid               GUID of the buffer storage.
  @param[in] Name               Name of the buffer storage.
  @param[in] DriverHandle       The DriverHandle that support a Device Path
                                Protocol.    
  @param[in] BufferStorage      Content of the buffer storage.
  @param[in] BufferStorageSize  Length in bytes of the buffer storage.
  @param[in] BlockNameArray     Array generated by VFR compiler.  This array
                                contains a UINT32 value that is the length
                                of BlockNameArray in bytes, followed by pairs
                                of 16-bit values that are the offset and length
                                values used to contruct a <ConfigRequest> string.
  @param[in]  ...               A variable argument list that contains pairs of 16-bit
                                ALTCFG identifiers and pointers to DefaultValueArrays.
                                The variable argument list is terminated by a NULL 
                                DefaultValueArray pointer.  A DefaultValueArray 
                                contains a UINT32 value that is the length, in bytes,
                                of the DefaultValueArray.  The UINT32 length value 
                                is followed by a series of records that contain
                                a 16-bit WIDTH value followed by a byte array with 
                                WIDTH entries.  The records must be parsed from
                                beginning to end until the UINT32 length limit
                                is reached.  

  @retval NULL          There are not enough resources to process the request.
  @retval NULL          A <ConfigResp> could not be retrieved from the Config 
                        Routing Protocol.
  @retval Other         A pointer to the Null-terminate Unicode <ConfigAltResp>
                        string.

**/
EFI_STRING
EFIAPI
HiiConstructConfigAltResp (
  IN CONST EFI_GUID  *Guid,
  IN CONST CHAR16    *Name,
  IN EFI_HANDLE      DriverHandle,
  IN CONST VOID      *BufferStorage,
  IN UINTN           BufferStorageSize,
  IN CONST VOID      *BlockNameArray, 
  ...
  )
{
  UINTN         Length;
  CHAR16        *String;
  CHAR16        *ConfigHdr;
  UINT8         *Buffer;
  UINT8         *BufferEnd;
  CHAR16        *ConfigRequest;
  EFI_STRING    ConfigResp;
  EFI_STRING    ConfigAltResp;
  VA_LIST       Args;
  UINTN         AltCfgId;
  UINT16        Width;

  ASSERT (Guid != NULL);
  ASSERT (Name != NULL);
  ASSERT (BlockNameArray != NULL);

  //
  // Initialize local variables
  //
  ConfigHdr     = NULL;
  ConfigRequest = NULL; 
  ConfigResp    = NULL;

  //
  // Construct <ConfigHdr> : "GUID=...&NAME=...&PATH=..."
  //
  ConfigHdr = HiiConstructConfigHdr (Guid, Name, DriverHandle);
  if (ConfigHdr == NULL) {
    goto Exit;
  }

  //
  // Compute the length of the entire request starting with <ConfigHdr> and a 
  // Null-terminator
  //
  Length = StrLen (ConfigHdr) + 1;

  //
  // Determine the size <BlockName> Offset/Width pairs
  //
  Buffer = (UINT8 *)BlockNameArray;
  BufferEnd = Buffer + ReadUnaligned32 ((UINT32 *)Buffer);
  Buffer += sizeof (UINT32);

  //
  // Add <BlockName> length that is composed of one or more Offset/Width pairs
  //
  // <BlockName> ::= &OFFSET=1234&WIDTH=1234
  //                 |  8   | 4 |   7  | 4 |
  //
  Length += (((BufferEnd - Buffer) / (sizeof (UINT16) + sizeof (UINT16))) * (8 + 4 + 7 + 4));

  //
  // Allocate buffer for the entire <ConfigRequest>
  //
  ConfigRequest = AllocateZeroPool (Length * sizeof (CHAR16));
  if (ConfigRequest == NULL) {
    goto Exit;
  }
  String = ConfigRequest;

  //
  // Start with <ConfigHdr>
  //
  StrCpy (String, ConfigHdr);
  String += StrLen (String);

  //
  // Loop through all the Offset/Width pairs and append them to ConfigRequest
  //
  while (Buffer < BufferEnd) {
    //
    // Append &OFFSET=XXXX&WIDTH=YYYY
    //
    UnicodeSPrint (
      String, 
      (8 + 4 + 7 + 4) * sizeof (CHAR16), 
      L"&OFFSET=%04X&WIDTH=%04X", 
      ReadUnaligned16 ((UINT16 *)Buffer), 
      ReadUnaligned16 ((UINT16 *)(Buffer + sizeof (UINT16)))
      );
    String += StrLen (String);
    Buffer += (sizeof (UINT16) + sizeof (UINT16));
  }

  //
  // Get the <ConfigResp>
  //
  ConfigResp = InternalHiiBlockToConfig (ConfigRequest, BufferStorage, BufferStorageSize);
  if (ConfigResp == NULL) {
    goto Exit;
  }

  //
  // Compute the length of the entire response starting with <ConfigResp> and a 
  // Null-terminator
  //
  Length = StrLen (ConfigResp) + 1;

  //
  // Add the length associated with each pair of variable argument parameters
  //
  VA_START (Args, BlockNameArray);
  while (TRUE) {
    AltCfgId = VA_ARG (Args, UINT16);
    Buffer   = VA_ARG (Args, UINT8 *);
    if (Buffer == NULL) {
      break;
    }

    //
    // Add length for "&<ConfigHdr>&ALTCFG=XXXX"
    //                |1| StrLen (ConfigHdr) | 8 | 4 |
    //
    Length += (1 + StrLen (ConfigHdr) + 8 + 4);

    BufferEnd = Buffer + ReadUnaligned32 ((UINT32 *)Buffer);
    Buffer += sizeof (UINT32);
    while (Buffer < BufferEnd) {
      //
      // Extract Width field
      //
      Width = ReadUnaligned16 ((UINT16 *)(Buffer + sizeof (UINT16)));

      //
      // Add length for "&OFFSET=XXXX&WIDTH=YYYY&VALUE=zzzzzzzzzzzz"
      //                |    8  | 4 |   7  | 4 |   7  | Width * 2 |
      //
      Length += (8 + 4 + 7 + 4 + 7 + Width * 2);

      //
      // Update Buffer to the next record
      //
      Buffer += (sizeof (UINT16) + sizeof (UINT16) + Width);
    }
  }
  VA_END (Args);

  //
  // Allocate a buffer for the entire response
  //
  ConfigAltResp = AllocateZeroPool (Length * sizeof (CHAR16));
  if (ConfigAltResp == NULL) {
    goto Exit;
  }
  String = ConfigAltResp;

  //
  // Add <ConfigResp>
  //
  StrCpy (String, ConfigResp);
  String += StrLen (String);

  //
  // Add <AltResp> for each pair of variable argument parameters
  //
  VA_START (Args, BlockNameArray);
  while (TRUE) {
    AltCfgId = VA_ARG (Args, UINT16);
    Buffer   = VA_ARG (Args, UINT8 *);
    if (Buffer == NULL) {
      break;
    }

    //
    // Add <AltConfigHdr> of the form "&<ConfigHdr>&ALTCFG=XXXX"
    //                                |1| StrLen (ConfigHdr) | 8 | 4 |
    //
    UnicodeSPrint (
      String, 
      (1 + StrLen (ConfigHdr) + 8 + 4) * sizeof (CHAR16), 
      L"&%s&ALTCFG=%04X", 
      ConfigHdr, 
      AltCfgId
      );
    String += StrLen (String);

    //
    // Add <ConfigBody> ::= <ConfigElement>*
    //
    BufferEnd = Buffer + ReadUnaligned32 ((UINT32 *)Buffer);
    Buffer += sizeof (UINT32);
    while (Buffer < BufferEnd) {
      //
      // Extract Width field
      //
      Width = ReadUnaligned16 ((UINT16 *)(Buffer + sizeof (UINT16)));

      //
      // Add <BlockConfig>
      //
      UnicodeSPrint (
        String, 
        (8 + 4 + 7 + 4 + 7 + Width * 2) * sizeof (CHAR16),
        L"&OFFSET=%04X&WIDTH=%04X&VALUE=", 
        ReadUnaligned16 ((UINT16 *)Buffer), 
        Width
        );
      String += StrLen (String);

      //
      // Update Buffer to point to the value in the current record
      //
      Buffer += (sizeof (UINT16) + sizeof (UINT16));

      //
      // Convert Value to a hex string in "%x" format
      //   NOTE: This is in the opposite byte that GUID and PATH use
      //
      for (; Width > 0; Width--) {
        String += UnicodeValueToString (String, PREFIX_ZERO | RADIX_HEX, Buffer[Width - 1], 2);
      }
      //
      // Update Buffer to the next record
      //
      Buffer += Width;
    }
  }
  VA_END (Args);

  //
  // Convert all hex digits in range [A-F] in the configuration header to [a-f]
  //
  return InternalHiiLowerConfigString (ConfigAltResp);

Exit:
  if (ConfigHdr != NULL) {
    FreePool (ConfigHdr);
  }
  if (ConfigRequest != NULL) {
    FreePool (ConfigRequest);
  }
  if (ConfigResp != NULL) {
    FreePool (ConfigResp);
  }

  return NULL;
}

/**
  Determines if two values in config strings match.

  Compares the substring between StartSearchString and StopSearchString in 
  FirstString to the substring between StartSearchString and StopSearchString 
  in SecondString.  If the two substrings match, then TRUE is returned.  If the
  two substrings do not match, then FALSE is returned.

  If FirstString is NULL, then ASSERT().
  If SecondString is NULL, then ASSERT().
  If StartSearchString is NULL, then ASSERT().
  If StopSearchString is NULL, then ASSERT().

  @param FirstString        Pointer to the first Null-terminated Unicode string.
  @param SecondString       Pointer to the second Null-terminated Unicode string.
  @param StartSearchString  Pointer to the Null-terminated Unicode string that 
                            marks the start of the value string to compare.
  @param StopSearchString   Pointer to the Null-terminated Unicode string that 
                            marks the end of the vakue string to compare.

  @retval FALSE             StartSearchString is not present in FirstString. 
  @retval FALSE             StartSearchString is not present in SecondString.
  @retval FALSE             StopSearchString is not present in FirstString. 
  @retval FALSE             StopSearchString is not present in SecondString.
  @retval FALSE             The length of the substring in FirstString is not the 
                            same length as the substring in SecondString.
  @retval FALSE             The value string in FirstString does not matche the 
                            value string in SecondString.
  @retval TRUE              The value string in FirstString matches the value 
                            string in SecondString.

**/
BOOLEAN
EFIAPI
InternalHiiCompareSubString (
  IN CHAR16  *FirstString,
  IN CHAR16  *SecondString,
  IN CHAR16  *StartSearchString,
  IN CHAR16  *StopSearchString
  )
{
  CHAR16  *EndFirstString;
  CHAR16  *EndSecondString;

  ASSERT (FirstString != NULL);
  ASSERT (SecondString != NULL);
  ASSERT (StartSearchString != NULL);
  ASSERT (StopSearchString != NULL);

  FirstString = StrStr (FirstString, StartSearchString);
  if (FirstString == NULL) {
    return FALSE;
  }

  SecondString = StrStr (SecondString, StartSearchString);
  if (SecondString == NULL) {
    return FALSE;
  }

  EndFirstString = StrStr (FirstString, StopSearchString);
  if (EndFirstString == NULL) {
    return FALSE;
  }

  EndSecondString = StrStr (SecondString, StopSearchString);
  if (EndSecondString == NULL) {
    return FALSE;
  }

  if ((EndFirstString - FirstString) != (EndSecondString - SecondString)) {
    return FALSE;
  }

  return (BOOLEAN)(StrnCmp (FirstString, SecondString, EndFirstString - FirstString) == 0);
}

/**
  Determines if the routing data specified by GUID and NAME match a <ConfigHdr>.

  If ConfigHdr is NULL, then ASSERT().

  @param[in] ConfigHdr  Either <ConfigRequest> or <ConfigResp>.
  @param[in] Guid       GUID of the storage.
  @param[in] Name       NAME of the storage.

  @retval TRUE   Routing information matches <ConfigHdr>.
  @retval FALSE  Routing information does not match <ConfigHdr>.

**/
BOOLEAN
EFIAPI
HiiIsConfigHdrMatch (
  IN CONST EFI_STRING  ConfigHdr,
  IN CONST EFI_GUID    *Guid,     OPTIONAL
  IN CONST CHAR16      *Name      OPTIONAL
  )
{
  EFI_STRING  CompareConfigHdr;
  BOOLEAN     Result;

  ASSERT (ConfigHdr != NULL);

  //
  // Use Guid and Name to generate a <ConfigHdr> string
  //
  CompareConfigHdr = HiiConstructConfigHdr (Guid, Name, NULL);
  if (CompareConfigHdr == NULL) {
    return FALSE;
  }

  Result = TRUE;
  if (Guid != NULL) {
    //
    // Compare GUID value strings
    //
    Result = InternalHiiCompareSubString (ConfigHdr, CompareConfigHdr, L"GUID=", L"&NAME=");
  }

  if (Result && Name != NULL) {
    //
    // Compare NAME value strings
    //
    Result = InternalHiiCompareSubString (ConfigHdr, CompareConfigHdr, L"&NAME=", L"&PATH=");
  }

  //
  // Free the <ConfigHdr> string
  //
  FreePool (CompareConfigHdr);

  return Result;
}

/**
  Retrieves uncommited data from the Form Browser and converts it to a binary
  buffer.  The returned buffer is allocated using AllocatePool().  The caller
  is responsible for freeing the returned buffer using FreePool().

  @param[in]  VariableName  Pointer to a Null-terminated Unicode string.  This 
                            is an optional parameter that may be NULL.
  @param[in]  VariableGuid  Pointer to an EFI_GUID structure.  This is an optional 
                            parameter that may be NULL.
  @param[in]  BufferSize    Length in bytes of buffer to hold retrived data. 

  @retval NULL   The uncommitted data could not be retrieved.
  @retval Other  A pointer to a buffer containing the uncommitted data.

**/
UINT8 *
EFIAPI
HiiGetBrowserData (
  IN CONST EFI_GUID  *VariableGuid,  OPTIONAL
  IN CONST CHAR16    *VariableName,  OPTIONAL
  IN UINTN           BlockSize
  )
{
  EFI_STRING  ResultsData;
  UINTN       Size;
  EFI_STRING  ConfigResp;
  UINT8       *Block;

  //
  // Retrieve the results data from the Browser Callback
  //
  ResultsData = InternalHiiBrowserCallback (VariableGuid, VariableName, NULL);
  if (ResultsData == NULL) {
    return NULL;
  }

  //
  // Construct <ConfigResp>
  //
  Size = (StrLen (mConfigHdrTemplate) + 1 + StrLen (ResultsData) + 1) * sizeof (CHAR16);
  ConfigResp = AllocateZeroPool (Size);
  UnicodeSPrint (ConfigResp, Size, L"%s&%s", mConfigHdrTemplate, ResultsData);
  
  //
  // Free the allocated buffer
  //
  FreePool (ResultsData);
  if (ConfigResp == NULL) {
    return NULL;
  }

  //
  // Convert <ConfigResp> to a buffer
  //
  Block = InternalHiiConfigToBlock (ConfigResp, BlockSize);
  FreePool (ConfigResp);

  return Block;
}

/**
  Updates uncommitted data in the Form Browser.

  If Buffer is NULL, then ASSERT().

  @param[in]  VariableName    Pointer to a Null-terminated Unicode string.  This
                              is an optional parameter that may be NULL.
  @param[in]  VariableGuid    Pointer to an EFI_GUID structure.  This is an optional
                              parameter that may be NULL.
  @param[in]  BufferSize      Length, in bytes, of Buffer.
  @param[in]  Buffer          Buffer of data to commit.
  @param[in]  RequestElement  An optional field to specify which part of the
                              buffer data will be send back to Browser. If NULL,
                              the whole buffer of data will be committed to
                              Browser. 
                              <RequestElement> ::= &OFFSET=<Number>&WIDTH=<Number>*

  @retval FALSE  The uncommitted data could not be updated.
  @retval TRUE   The uncommitted data was updated.

**/
BOOLEAN
EFIAPI
HiiSetBrowserData (
  IN CONST EFI_GUID  *VariableGuid, OPTIONAL
  IN CONST CHAR16    *VariableName, OPTIONAL
  IN UINTN           BufferSize,
  IN CONST UINT8     *Buffer,
  IN CONST CHAR16    *RequestElement  OPTIONAL
  )
{
  UINTN       Size;
  EFI_STRING  ConfigRequest;
  EFI_STRING  ConfigResp;
  EFI_STRING  ResultsData;

  ASSERT (Buffer != NULL);

  //
  // Construct <ConfigRequest>
  //
  if (RequestElement == NULL) {
    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template 
    // followed by "&OFFSET=0&WIDTH=WWWWWWWWWWWWWWWW" followed by a Null-terminator
    //
    Size = (StrLen (mConfigHdrTemplate) + 32 + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    UnicodeSPrint (ConfigRequest, Size, L"%s&OFFSET=0&WIDTH=%016LX", mConfigHdrTemplate, (UINT64)BufferSize);
  } else {
    //
    // Allocate and fill a buffer large enough to hold the <ConfigHdr> template 
    // followed by <RequestElement> followed by a Null-terminator
    //
    Size = (StrLen (mConfigHdrTemplate) + StrLen (RequestElement) + 1) * sizeof (CHAR16);
    ConfigRequest = AllocateZeroPool (Size);
    UnicodeSPrint (ConfigRequest, Size, L"%s%s", mConfigHdrTemplate, RequestElement);
  }
  if (ConfigRequest == NULL) {
    return FALSE;
  }

  //
  // Convert <ConfigRequest> to <ConfigResp>
  //
  ConfigResp = InternalHiiBlockToConfig (ConfigRequest, Buffer, BufferSize);
  FreePool (ConfigRequest);
  if (ConfigResp == NULL) {
    return FALSE;
  }

  //
  // Set data in the uncommitted browser state information
  //
  ResultsData = InternalHiiBrowserCallback (VariableGuid, VariableName, ConfigResp + StrLen(mConfigHdrTemplate) + 1);
  FreePool (ConfigResp);

  return (BOOLEAN)(ResultsData != NULL);
}

/////////////////////////////////////////
/////////////////////////////////////////
/// IFR Functions
/////////////////////////////////////////
/////////////////////////////////////////

#define HII_LIB_OPCODE_ALLOCATION_SIZE  0x200

typedef struct {
  UINT8  *Buffer;
  UINTN  BufferSize;
  UINTN  Position;
} HII_LIB_OPCODE_BUFFER;

///
/// Lookup table that converts EFI_IFR_TYPE_X enum values to a width in bytes
///
GLOBAL_REMOVE_IF_UNREFERENCED CONST UINT8 mHiiDefaultTypeToWidth[] = {
  1, // EFI_IFR_TYPE_NUM_SIZE_8
  2, // EFI_IFR_TYPE_NUM_SIZE_16
  4, // EFI_IFR_TYPE_NUM_SIZE_32
  8, // EFI_IFR_TYPE_NUM_SIZE_64
  1, // EFI_IFR_TYPE_BOOLEAN
  3, // EFI_IFR_TYPE_TIME
  4, // EFI_IFR_TYPE_DATE
  2  // EFI_IFR_TYPE_STRING
};

/**
  Allocates and returns a new OpCode Handle.  OpCode Handles must be freed with 
  HiiFreeOpCodeHandle().

  @retval NULL   There are not enough resources to allocate a new OpCode Handle.
  @retval Other  A new OpCode handle.

**/
VOID *
EFIAPI
HiiAllocateOpCodeHandle (
  VOID
  )
{
  HII_LIB_OPCODE_BUFFER  *OpCodeBuffer;

  OpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)AllocatePool (sizeof (HII_LIB_OPCODE_BUFFER));
  if (OpCodeBuffer == NULL) {
    return NULL;
  }
  OpCodeBuffer->Buffer = (UINT8 *)AllocatePool (HII_LIB_OPCODE_ALLOCATION_SIZE);
  if (OpCodeBuffer->Buffer == NULL) {
    FreePool (OpCodeBuffer);
    return NULL;
  }
  OpCodeBuffer->BufferSize = HII_LIB_OPCODE_ALLOCATION_SIZE;
  OpCodeBuffer->Position = 0;
  return (VOID *)OpCodeBuffer;
}

/**
  Frees an OpCode Handle that was peviously allocated with HiiAllocateOpCodeHandle().
  When an OpCode Handle is freed, all of the opcodes associated with the OpCode
  Handle are also freed.

  If OpCodeHandle is NULL, then ASSERT().

**/
VOID
EFIAPI
HiiFreeOpCodeHandle (
  VOID  *OpCodeHandle
  )
{
  HII_LIB_OPCODE_BUFFER  *OpCodeBuffer;

  ASSERT (OpCodeHandle != NULL);

  OpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)OpCodeHandle;
  if (OpCodeBuffer->Buffer != NULL) {
    FreePool (OpCodeBuffer->Buffer);
  }
  FreePool (OpCodeBuffer);
}

UINTN
EFIAPI
InternalHiiOpCodeHandlePosition (
  IN VOID  *OpCodeHandle
  )
{
  return ((HII_LIB_OPCODE_BUFFER  *)OpCodeHandle)->Position;
}

UINT8 *
EFIAPI
InternalHiiOpCodeHandleBuffer (
  IN VOID  *OpCodeHandle
  )
{
  return ((HII_LIB_OPCODE_BUFFER  *)OpCodeHandle)->Buffer;
}

UINT8 *
EFIAPI
InternalHiiGrowOpCodeHandle (
  VOID   *OpCodeHandle,
  UINTN  Size
  )
{
  HII_LIB_OPCODE_BUFFER  *OpCodeBuffer;
  UINT8                  *Buffer;

  ASSERT (OpCodeHandle != NULL);

  OpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)OpCodeHandle;
  if (OpCodeBuffer->Position + Size > OpCodeBuffer->BufferSize) {
    Buffer = ReallocatePool (
              OpCodeBuffer->BufferSize, 
              OpCodeBuffer->BufferSize + (Size + HII_LIB_OPCODE_ALLOCATION_SIZE),
              OpCodeBuffer->Buffer
              );
    if (Buffer == NULL) {
      return NULL;
    }
    OpCodeBuffer->Buffer = Buffer;
    OpCodeBuffer->BufferSize += (Size + HII_LIB_OPCODE_ALLOCATION_SIZE);
  }
  Buffer = OpCodeBuffer->Buffer + OpCodeBuffer->Position;
  OpCodeBuffer->Position += Size;
  return Buffer;
}

UINT8 *
EFIAPI
InternalHiiCreateOpCodeExtended (
  IN VOID   *OpCodeHandle,
  IN VOID   *OpCodeTemplate,
  IN UINT8  OpCode,
  IN UINTN  OpCodeSize,
  IN UINTN  ExtensionSize,
  IN UINT8  Scope
  )
{
  EFI_IFR_OP_HEADER  *Header;
  UINT8              *Buffer;

  ASSERT (OpCodeTemplate != NULL);
  ASSERT ((OpCodeSize + ExtensionSize) <= 0x7F);

  Header = (EFI_IFR_OP_HEADER *)OpCodeTemplate;
  Header->OpCode = OpCode;
  Header->Scope  = Scope;
  Header->Length = (UINT8)(OpCodeSize + ExtensionSize);
  Buffer = InternalHiiGrowOpCodeHandle (OpCodeHandle, Header->Length);
  return (UINT8 *)CopyMem (Buffer, Header, OpCodeSize);
}

UINT8 *
EFIAPI
InternalHiiCreateOpCode (
  IN VOID   *OpCodeHandle,
  IN VOID   *OpCodeTemplate,
  IN UINT8  OpCode,
  IN UINTN  OpCodeSize
  )
{
  return InternalHiiCreateOpCodeExtended (OpCodeHandle, OpCodeTemplate, OpCode, OpCodeSize, 0, 0);
}

/**
  Append raw opcodes to an OpCodeHandle.

  If OpCodeHandle is NULL, then ASSERT().
  If RawBuffer is NULL, then ASSERT();

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.
  @param[in]  RawBuffer      Buffer of opcodes to append.
  @param[in]  RawBufferSize  The size, in bytes, of Buffer.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the appended opcodes.

**/
UINT8 *
EFIAPI
InternalHiiCreateRawOpCodes (
  IN VOID   *OpCodeHandle,
  IN UINT8  *RawBuffer,
  IN UINTN  RawBufferSize
  )
{
  UINT8  *Buffer;

  ASSERT (RawBuffer != NULL);

  Buffer = InternalHiiGrowOpCodeHandle (OpCodeHandle, RawBufferSize);
  return (UINT8 *)CopyMem (Buffer, RawBuffer, RawBufferSize);
}

/**
  Append opcodes from one OpCode Handle to another OpCode handle.

  If OpCodeHandle is NULL, then ASSERT().
  If RawOpCodeHandle is NULL, then ASSERT();

  @param[in]  OpCodeHandle     Handle to the buffer of opcodes.
  @param[in]  RawOpCodeHandle  Handle to the buffer of opcodes.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the appended opcodes.

**/
UINT8 *
EFIAPI
InternalHiiAppendOpCodes (
  IN VOID  *OpCodeHandle,
  IN VOID  *RawOpCodeHandle
  )
{
  HII_LIB_OPCODE_BUFFER  *RawOpCodeBuffer;

  ASSERT (RawOpCodeHandle != NULL);

  RawOpCodeBuffer = (HII_LIB_OPCODE_BUFFER *)RawOpCodeHandle;
  return InternalHiiCreateRawOpCodes (OpCodeHandle, RawOpCodeBuffer->Buffer, RawOpCodeBuffer->Position);
}

/**
  Create EFI_IFR_END_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateEndOpCode (
  IN VOID  *OpCodeHandle
  )
{
  EFI_IFR_END  OpCode;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_END_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_ONE_OF_OPTION_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If Type is invalid, then ASSERT().
  If Flags is invalid, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  StringId      StringId for the option
  @param[in]  Flags         Flags for the option
  @param[in]  Type          Type for the option
  @param[in]  Value         Value for the option

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateOneOfOptionOpCode (
  IN VOID    *OpCodeHandle,
  IN UINT16  StringId,
  IN UINT8   Flags,
  IN UINT8   Type,
  IN UINT64  Value
  )
{
  EFI_IFR_ONE_OF_OPTION  OpCode;

  ASSERT (Type < EFI_IFR_TYPE_OTHER);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Option = StringId;
  OpCode.Flags  = (UINT8) (Flags & (EFI_IFR_OPTION_DEFAULT | EFI_IFR_OPTION_DEFAULT_MFG));
  OpCode.Type   = Type;
  CopyMem (&OpCode.Value, &Value, mHiiDefaultTypeToWidth[Type]);

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_ONE_OF_OPTION_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_DEFAULT_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If Type is invalid, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  DefaultId     DefaultId for the default
  @param[in]  Type          Type for the default
  @param[in]  Value         Value for the default

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateDefaultOpCode (
  IN VOID    *OpCodeHandle,
  IN UINT16  DefaultId,
  IN UINT8   Type,
  IN UINT64  Value
  )
{
  EFI_IFR_DEFAULT  OpCode;

  ASSERT (Type < EFI_IFR_TYPE_OTHER);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Type      = Type;
  OpCode.DefaultId = DefaultId;
  CopyMem (&OpCode.Value, &Value, mHiiDefaultTypeToWidth[Type]);

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_DEFAULT_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_GUID opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If Guid is NULL, then ASSERT().
  If OpCodeSize < sizeof (EFI_IFR_GUID), then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  Guid          Pointer to EFI_GUID of this guided opcode.
  @param[in]  GuidOpCode    Pointer to an EFI_IFR_GUID opcode.  This is an 
                            optional parameter that may be NULL.  If this
                            parameter is NULL, then the GUID extension 
                            region of the created opcode is filled with zeros.
                            If this parameter is not NULL, then the GUID 
                            extension region of GuidData will be copied to 
                            the GUID extension region of the created opcode.
  @param[in]  OpCodeSize    The size, in bytes, of created opcode.  This value 
                            must be >= sizeof(EFI_IFR_GUID).

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateGuidOpCode (
  IN VOID            *OpCodeHandle,
  IN CONST EFI_GUID  *Guid,
  IN CONST VOID      *GuidOpCode,    OPTIONAL
  IN UINTN           OpCodeSize
  )
{
  EFI_IFR_GUID  OpCode;
  EFI_IFR_GUID  *OpCodePointer;

  ASSERT (Guid != NULL);
  ASSERT (OpCodeSize >= sizeof (OpCode));

  ZeroMem (&OpCode, sizeof (OpCode));
  CopyGuid (&OpCode.Guid, Guid);

  OpCodePointer = (EFI_IFR_GUID *)InternalHiiCreateOpCodeExtended (
                                    OpCodeHandle, 
                                    &OpCode,
                                    EFI_IFR_GUID_OP,
                                    sizeof (OpCode),
                                    OpCodeSize - sizeof (OpCode),
                                    0
                                    );
  if (OpCodePointer != NULL && GuidOpCode != NULL) {
    CopyMem (OpCodePointer + 1, (EFI_IFR_GUID *)GuidOpCode + 1, OpCodeSize - sizeof (OpCode));
  }
  return (UINT8 *)OpCodePointer;
}

/**
  Create EFI_IFR_ACTION_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  QuestionId      Question ID
  @param[in]  Prompt          String ID for Prompt
  @param[in]  Help            String ID for Help
  @param[in]  QuestionFlags   Flags in Question Header
  @param[in]  QuestionConfig  String ID for configuration

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateActionOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN EFI_STRING_ID    QuestionConfig
  )
{
  EFI_IFR_ACTION  OpCode;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.QuestionId    = QuestionId;
  OpCode.Question.Header.Prompt = Prompt;
  OpCode.Question.Header.Help   = Help;
  OpCode.Question.Flags         = QuestionFlags;
  OpCode.QuestionConfig         = QuestionConfig;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_ACTION_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_SUBTITLE_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in Flags, then ASSERT().
  If Scope > 1, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  Prompt      String ID for Prompt
  @param[in]  Help        String ID for Help
  @param[in]  Flags       Subtitle opcode flags
  @param[in]  Scope       1 if this opcpde is the beginning of a new scope.
                          0 if this opcode is within the current scope.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateSubTitleOpCode (
  IN VOID           *OpCodeHandle,
  IN EFI_STRING_ID  Prompt,
  IN EFI_STRING_ID  Help,
  IN UINT8          Flags,
  IN UINT8          Scope
  )
{
  EFI_IFR_SUBTITLE  OpCode;

  ASSERT (Scope <= 1);
  ASSERT ((Flags & (~(EFI_IFR_FLAGS_HORIZONTAL))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Statement.Prompt = Prompt;
  OpCode.Statement.Help   = Help;
  OpCode.Flags            = Flags;

  return InternalHiiCreateOpCodeExtended (
           OpCodeHandle, 
           &OpCode,
           EFI_IFR_SUBTITLE_OP, 
           sizeof (OpCode), 
           0, 
           Scope
           );
}

/**
  Create EFI_IFR_REF_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().

  @param[in]  OpCodeHandle   Handle to the buffer of opcodes.
  @param[in]  FormId         Destination Form ID
  @param[in]  Prompt         String ID for Prompt
  @param[in]  Help           String ID for Help
  @param[in]  QuestionFlags  Flags in Question Header
  @param[in]  QuestionId     Question ID

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateGotoOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_FORM_ID      FormId,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN EFI_QUESTION_ID  QuestionId
  )
{
  EFI_IFR_REF  OpCode;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt = Prompt;
  OpCode.Question.Header.Help   = Help;
  OpCode.Question.QuestionId    = QuestionId;
  OpCode.Question.Flags         = QuestionFlags;
  OpCode.FormId                 = FormId;

  return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_REF_OP, sizeof (OpCode));
}

/**
  Create EFI_IFR_CHECKBOX_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in CheckBoxFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  CheckBoxFlags         Flags for checkbox opcode
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateCheckBoxOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            CheckBoxFlags,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_CHECKBOX  OpCode;
  UINTN             Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = CheckBoxFlags;

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_CHECKBOX_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_CHECKBOX_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_NUMERIC_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in NumericFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  NumericFlags          Flags for numeric opcode
  @param[in]  Minimum               Numeric minimum value
  @param[in]  Maximum               Numeric maximum value
  @param[in]  Step                  Numeric step for edit
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateNumericOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            NumericFlags,
  IN UINT64           Minimum,
  IN UINT64           Maximum,
  IN UINT64           Step,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_NUMERIC  OpCode;
  UINTN            Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = NumericFlags;

  switch (NumericFlags & EFI_IFR_NUMERIC_SIZE) {
  case EFI_IFR_NUMERIC_SIZE_1:
    OpCode.data.u8.MinValue = (UINT8)Minimum;
    OpCode.data.u8.MaxValue = (UINT8)Maximum;
    OpCode.data.u8.Step     = (UINT8)Step;
    break;

  case EFI_IFR_NUMERIC_SIZE_2:
    OpCode.data.u16.MinValue = (UINT16)Minimum;
    OpCode.data.u16.MaxValue = (UINT16)Maximum;
    OpCode.data.u16.Step     = (UINT16)Step;
    break;

  case EFI_IFR_NUMERIC_SIZE_4:
    OpCode.data.u32.MinValue = (UINT32)Minimum;
    OpCode.data.u32.MaxValue = (UINT32)Maximum;
    OpCode.data.u32.Step     = (UINT32)Step;
    break;

  case EFI_IFR_NUMERIC_SIZE_8:
    OpCode.data.u64.MinValue = Minimum;
    OpCode.data.u64.MaxValue = Maximum;
    OpCode.data.u64.Step     = Step;
    break;
  }

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_NUMERIC_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_NUMERIC_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_STRING_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in StringFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  StringFlags           Flags for string opcode
  @param[in]  MinSize               String minimum length
  @param[in]  MaxSize               String maximum length
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateStringOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            StringFlags,
  IN UINT8            MinSize,
  IN UINT8            MaxSize,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_STRING  OpCode;
  UINTN           Position;

  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.MinSize                         = MinSize;
  OpCode.MaxSize                         = MaxSize;
  OpCode.Flags                           = (UINT8) (StringFlags & EFI_IFR_STRING_MULTI_LINE);

  if (DefaultsOpCodeHandle == NULL) {
    return InternalHiiCreateOpCode (OpCodeHandle, &OpCode, EFI_IFR_STRING_OP, sizeof (OpCode));
  }

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_STRING_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_ONE_OF_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in OneOfFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  OneOfFlags            Flags for oneof opcode
  @param[in]  OptionsOpCodeHandle   Handle for a buffer of ONE_OF_OPTION opcodes.
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateOneOfOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            OneOfFlags,
  IN VOID             *OptionsOpCodeHandle,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_ONE_OF  OpCode;
  UINTN           Position;

  ASSERT (OptionsOpCodeHandle != NULL);
  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.Flags                           = OneOfFlags;

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_ONE_OF_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, OptionsOpCodeHandle);
  if (DefaultsOpCodeHandle != NULL) {
    InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  }
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  Create EFI_IFR_ORDERED_LIST_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in OrderedListFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID
  @param[in]  VarOffset             Offset in Storage
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  OrderedListFlags      Flags for ordered list opcode
  @param[in]  DataType              Type for option value
  @param[in]  MaxContainers         Maximum count for options in this ordered list
  @param[in]  OptionsOpCodeHandle   Handle for a buffer of ONE_OF_OPTION opcodes.
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateOrderedListOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,
  IN UINT16           VarOffset,
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            OrderedListFlags,
  IN UINT8            DataType,
  IN UINT8            MaxContainers,
  IN VOID             *OptionsOpCodeHandle,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  )
{
  EFI_IFR_ORDERED_LIST  OpCode;
  UINTN                 Position;

  ASSERT (OptionsOpCodeHandle != NULL);
  ASSERT ((QuestionFlags & (~(EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY))) == 0);

  ZeroMem (&OpCode, sizeof (OpCode));
  OpCode.Question.Header.Prompt          = Prompt;
  OpCode.Question.Header.Help            = Help;
  OpCode.Question.QuestionId             = QuestionId;
  OpCode.Question.VarStoreId             = VarStoreId;
  OpCode.Question.VarStoreInfo.VarOffset = VarOffset;
  OpCode.Question.Flags                  = QuestionFlags;
  OpCode.MaxContainers                   = MaxContainers;
  OpCode.Flags                           = OrderedListFlags;

  Position = InternalHiiOpCodeHandlePosition (OpCodeHandle);
  InternalHiiCreateOpCodeExtended (OpCodeHandle, &OpCode, EFI_IFR_ORDERED_LIST_OP, sizeof (OpCode), 0, 1);
  InternalHiiAppendOpCodes (OpCodeHandle, OptionsOpCodeHandle);
  if (DefaultsOpCodeHandle != NULL) {
    InternalHiiAppendOpCodes (OpCodeHandle, DefaultsOpCodeHandle);
  }
  HiiCreateEndOpCode (OpCodeHandle);
  return InternalHiiOpCodeHandleBuffer (OpCodeHandle) + Position;
}

/**
  This is the internal worker function to update the data in
  a form specified by FormSetGuid, FormId and Label.

  @param FormSetGuid     The optional Formset GUID.
  @param FormId          The Form ID.
  @param Package         The package header.

  @param TempPacakge     The resultant package.

  @retval EFI_SUCCESS    The function completes successfully.

**/
EFI_STATUS
EFIAPI
InternalHiiUpdateFormPackageData (
  IN  EFI_GUID               *FormSetGuid, OPTIONAL
  IN  EFI_FORM_ID            FormId,
  IN  EFI_HII_PACKAGE_HEADER *Package,
  IN  HII_LIB_OPCODE_BUFFER  *OpCodeBufferStart,
  IN  HII_LIB_OPCODE_BUFFER  *OpCodeBufferEnd,    OPTIONAL
  OUT EFI_HII_PACKAGE_HEADER *TempPackage
  )
{
  UINTN                     AddSize;
  UINT8                     *BufferPos;
  EFI_HII_PACKAGE_HEADER    PackageHeader;
  UINTN                     Offset;
  EFI_IFR_OP_HEADER         *IfrOpHdr;
  EFI_IFR_OP_HEADER         *UpdateIfrOpHdr;
  BOOLEAN                   GetFormSet;
  BOOLEAN                   GetForm;
  BOOLEAN                   Updated;
  EFI_IFR_OP_HEADER         *AddOpCode;
  UINT32                    UpdatePackageLength;

  CopyMem (TempPackage, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  UpdatePackageLength = sizeof (EFI_HII_PACKAGE_HEADER);
  BufferPos           = (UINT8 *) (TempPackage + 1);

  CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  IfrOpHdr   = (EFI_IFR_OP_HEADER *)((UINT8 *) Package + sizeof (EFI_HII_PACKAGE_HEADER));
  Offset     = sizeof (EFI_HII_PACKAGE_HEADER);
  GetFormSet = (BOOLEAN) ((FormSetGuid == NULL) ? TRUE : FALSE);
  GetForm    = FALSE;
  Updated    = FALSE;

  while (Offset < PackageHeader.Length) {
    CopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
    BufferPos           += IfrOpHdr->Length;
    UpdatePackageLength += IfrOpHdr->Length;
    
    //
    // Find the matched FormSet and Form
    //
    if ((IfrOpHdr->OpCode == EFI_IFR_FORM_SET_OP) && (FormSetGuid != NULL)) {
      if (CompareGuid((GUID *)(VOID *)&((EFI_IFR_FORM_SET *) IfrOpHdr)->Guid, FormSetGuid)) {
        GetFormSet = TRUE;
      } else {
        GetFormSet = FALSE;
      }
    } else if (IfrOpHdr->OpCode == EFI_IFR_FORM_OP) {
      if (CompareMem (&((EFI_IFR_FORM *) IfrOpHdr)->FormId, &FormId, sizeof (EFI_FORM_ID)) == 0) {
        GetForm = TRUE;
      } else {
        GetForm = FALSE;
      }
    }
    
    //
    // The matched Form is found, and Update data in this form
    //
    if (GetFormSet && GetForm && !Updated) {
      UpdateIfrOpHdr = (EFI_IFR_OP_HEADER *) OpCodeBufferStart->Buffer;
      if ((UpdateIfrOpHdr->Length == IfrOpHdr->Length) && \
          (CompareMem (IfrOpHdr, UpdateIfrOpHdr, UpdateIfrOpHdr->Length) == 0)) {
        //
        // Remove the original data when End OpCode buffer exist.
        //
        if (OpCodeBufferEnd != NULL) {
          Offset        += IfrOpHdr->Length;
          IfrOpHdr       = (EFI_IFR_OP_HEADER *) ((UINT8 *) (IfrOpHdr) + IfrOpHdr->Length);
          UpdateIfrOpHdr = (EFI_IFR_OP_HEADER *) OpCodeBufferEnd->Buffer;
          while (Offset < PackageHeader.Length) {
            //
            // Search the matched end opcode
            //
            if ((UpdateIfrOpHdr->Length == IfrOpHdr->Length) && \
                (CompareMem (IfrOpHdr, UpdateIfrOpHdr, UpdateIfrOpHdr->Length) == 0)) {
              break;
            }
            //
            // Go to the next Op-Code
            //
            Offset        += IfrOpHdr->Length;
            IfrOpHdr       = (EFI_IFR_OP_HEADER *) ((UINT8 *) (IfrOpHdr) + IfrOpHdr->Length);
          }
          
          if (Offset >= PackageHeader.Length) {
            //
            // The end opcode is not found.
            //
            return EFI_NOT_FOUND;
          }
        }
        //
        // Insert the updated data
        //
        UpdateIfrOpHdr = (EFI_IFR_OP_HEADER *) OpCodeBufferStart->Buffer;
        AddOpCode      = (EFI_IFR_OP_HEADER *) (OpCodeBufferStart->Buffer + UpdateIfrOpHdr->Length);
        AddSize        = UpdateIfrOpHdr->Length;
        while (AddSize < OpCodeBufferStart->Position) {
          CopyMem (BufferPos, AddOpCode, AddOpCode->Length);
          BufferPos           += AddOpCode->Length;
          UpdatePackageLength += AddOpCode->Length;

          AddOpCode = (EFI_IFR_OP_HEADER *) ((UINT8 *) (AddOpCode) + AddOpCode->Length);
          AddSize += AddOpCode->Length;          
        }

        if (OpCodeBufferEnd != NULL) {
          //
          // Add the end opcode
          //
          CopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
          BufferPos           += IfrOpHdr->Length;
          UpdatePackageLength += IfrOpHdr->Length;
        }
        //
        // Set update flag
        //
        Updated = TRUE;
      }
    }

    //
    // Go to the next Op-Code
    //
    Offset   += IfrOpHdr->Length;
    IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
  }
  
  if (!Updated) {
    //
    // The updated opcode buffer is not found.
    //
    return EFI_NOT_FOUND;
  }
  //
  // Update the package length.
  //
  PackageHeader.Length = UpdatePackageLength;
  CopyMem (TempPackage, &PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));

  return EFI_SUCCESS;
}

/**
  This function updates a form that has previously been registered with the HII 
  Database.  This function will perform at most one update operation.
    
  The form to update is specified by Handle, FormSetGuid, and FormId.  Binary 
  comparisons of IFR opcodes are performed from the beginning of the form being 
  updated until an IFR opcode is found that exactly matches the first IFR opcode 
  specifed by StartOpCodeHandle.  The following rules are used to determine if
  an insert, replace, or delete operation is performed.
  
  1) If no matches are found, then NULL is returned.  
  2) If a match is found, and EndOpCodeHandle is NULL, then all of the IFR opcodes
     from StartOpcodeHandle except the first opcode are inserted immediately after 
     the matching IFR opcode in the form beng updated.
  3) If a match is found, and EndOpCodeHandle is not NULL, then a search is made 
     from the matching IFR opcode until an IFR opcode exatly matches the first 
     IFR opcode specified by EndOpCodeHandle.  If no match is found for the first
     IFR opcode specified by EndOpCodeHandle, then NULL is returned.  If a match
     is found, then all of the IFR opcodes between the start match and the end 
     match are deleted from the form being updated and all of the IFR opcodes
     from StartOpcodeHandle except the first opcode are inserted immediately after 
     the matching start IFR opcode.  If StartOpCcodeHandle only contains one
     IFR instruction, then the result of ths operation will delete all of the IFR
     opcodes between the start end matches.

  If HiiHandle is NULL, then ASSERT().
  If StartOpCodeHandle is NULL, then ASSERT().

  @param[in]  HiiHandle          The HII Handle of the form to update.
  @param[in]  FormSetGuid        The Formset GUID of the form to update.  This
                                 is an optional parameter that may be NULL.
                                 If it is NULL, all FormSet will be updated.
  @param[in]  FormId             The ID of the form to update.
  @param[in]  StartOpCodeHandle  An OpCode Handle that contains the set of IFR 
                                 opcodes to be inserted or replaced in the form.
                                 The first IFR instruction in StartOpCodeHandle 
                                 is used to find matching IFR opcode in the 
                                 form. 
  @param[in]  EndOpCodeHandle    An OpCcode Handle that contains the IFR opcode
                                 that marks the end of a replace operation in
                                 the form.  This is an optional parameter that
                                 may be NULL.  If it is NULL, then an the IFR
                                 opcodes specified by StartOpCodeHandle are 
                                 inserted into the form.
  
  @retval EFI_OUT_OF_RESOURCES   No enough memory resource is allocated.
  @retval EFI_NOT_FOUND          The following cases will return EFI_NOT_FOUND.
                                 1) The form specified by HiiHandle, FormSetGuid, 
                                 and FormId could not be found in the HII Database.
                                 2) No IFR opcodes in the target form match the first
                                 IFR opcode in StartOpCodeHandle.
                                 3) EndOpCOde is not NULL, and no IFR opcodes in the 
                                 target form following a matching start opcode match 
                                 the first IFR opcode in EndOpCodeHandle.
  @retval EFI_SUCCESS            The matched form is updated by StartOpcode.

**/
EFI_STATUS
EFIAPI
HiiUpdateForm (
  IN EFI_HII_HANDLE  HiiHandle,           
  IN EFI_GUID        *FormSetGuid,        OPTIONAL
  IN EFI_FORM_ID     FormId,
  IN VOID            *StartOpcodeHandle,
  IN VOID            *EndOpcodeHandle     OPTIONAL
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT32                       PackageListLength;  
  UINT32                       Offset;
  EFI_HII_PACKAGE_LIST_HEADER  *UpdatePackageList;
  UINTN                        BufferSize;
  UINT8                        *UpdateBufferPos;
  EFI_HII_PACKAGE_HEADER       *Package;
  EFI_HII_PACKAGE_HEADER       *TempPacakge;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  BOOLEAN                      Updated;
  HII_LIB_OPCODE_BUFFER        *OpCodeBufferStart;
  HII_LIB_OPCODE_BUFFER        *OpCodeBufferEnd;
  
  //
  // Input update data can't be NULL.
  //
  ASSERT (HiiHandle != NULL);
  ASSERT (StartOpcodeHandle != NULL);
  UpdatePackageList = NULL;
  TempPacakge       = NULL;
  HiiPackageList    = NULL;
  
  //
  // Restrive buffer data from Opcode Handle
  //
  OpCodeBufferStart = (HII_LIB_OPCODE_BUFFER *) StartOpcodeHandle;
  OpCodeBufferEnd   = (HII_LIB_OPCODE_BUFFER *) EndOpcodeHandle;
  
  //
  // Get the orginal package list
  //
  BufferSize = 0;
  HiiPackageList   = NULL;
  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  //
  // The return status should always be EFI_BUFFER_TOO_SMALL as input buffer's size is 0.
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  HiiPackageList = AllocatePool (BufferSize);
  if (HiiPackageList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  Status = gHiiDatabase->ExportPackageLists (gHiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  if (EFI_ERROR (Status)) {
    goto Finish;
  }

  //
  // Calculate and allocate space for retrieval of IFR data
  //
  BufferSize += OpCodeBufferStart->Position;
  UpdatePackageList = AllocateZeroPool (BufferSize);
  if (UpdatePackageList == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }
  
  //
  // Allocate temp buffer to store the temp updated package buffer
  //
  TempPacakge = AllocateZeroPool (BufferSize);
  if (TempPacakge == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Finish;
  }

  UpdateBufferPos = (UINT8 *) UpdatePackageList;

  //
  // Copy the package list header
  //
  CopyMem (UpdateBufferPos, HiiPackageList, sizeof (EFI_HII_PACKAGE_LIST_HEADER));
  UpdateBufferPos += sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  
  //
  // Go through each package to find the matched pacakge and update one by one
  //
  Updated = FALSE;
  Offset  = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);
  while (Offset < PackageListLength) {
    Package = (EFI_HII_PACKAGE_HEADER *) (((UINT8 *) HiiPackageList) + Offset);
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    Offset += Package->Length;

    if (Package->Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Check this package is the matched package.
      //
      Status = InternalHiiUpdateFormPackageData (FormSetGuid, FormId, Package, OpCodeBufferStart, OpCodeBufferEnd, TempPacakge);
      //
      // The matched package is found. Its pacakge buffer will be updated by the input new data.
      //
      if (!EFI_ERROR(Status)) {
        //
        // Set Update Flag
        //        
        Updated = TRUE;
        //
        // Add updated package buffer
        //
        Package = TempPacakge;
      }
    }

    //
    // Add pacakge buffer
    //
    CopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    CopyMem (UpdateBufferPos, Package, PackageHeader.Length);
    UpdateBufferPos += PackageHeader.Length;
  }
  
  if (Updated) {
    //
    // Update package list length
    //
    BufferSize = UpdateBufferPos - (UINT8 *) UpdatePackageList;
    WriteUnaligned32 (&UpdatePackageList->PackageLength, (UINT32) BufferSize);
    
    //
    // Update Pacakge to show form
    //
    Status = gHiiDatabase->UpdatePackageList (gHiiDatabase, HiiHandle, UpdatePackageList);
  } else {
    //
    // Not matched form is found and updated.
    //
    Status = EFI_NOT_FOUND;
  }

Finish:
  if (HiiPackageList != NULL) {
    FreePool (HiiPackageList);
  }
  
  if (UpdatePackageList != NULL) {
    FreePool (UpdatePackageList);
  }
  
  if (TempPacakge != NULL) {
    FreePool (TempPacakge);
  }

  return Status; 
}

/**
  Configure the buffer accrording to ConfigBody strings in the format of
  <Length:4 bytes>, <Offset: 2 bytes>, <Width:2 bytes>, <Data:n bytes>.
  This ConfigBody strings is generated by UEFI VfrCompiler for the default
  values in a Form Set. The name of the ConfigBody strings is VfrMyIfrNVDataDefault0000
  constructed following this rule: 
   "Vfr" + varstore.name + "Default" + defaultstore.attributes.
  Check the generated C file in Output for details.

  @param  Buffer                 The start address of buffer.
  @param  BufferSize             The size of buffer.
  @param  Number                 The number of the strings.
  @param  ...                    Variable argument list for default value in <AltResp> format 
                                 generated by the tool.

  @retval EFI_BUFFER_TOO_SMALL   the BufferSize is too small to operate.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or BufferSize is 0.
  @retval EFI_SUCCESS            Operation successful.

**/
EFI_STATUS
EFIAPI
IfrLibExtractDefault(
  IN VOID                         *Buffer,
  IN UINTN                        *BufferSize,
  UINTN                           Number,
  ...
  )
{
  VA_LIST                         Args;
  UINTN                           Index;
  UINT32                          TotalLen;
  UINT8                           *BufCfgArray;
  UINT8                           *BufferPos;
  UINT16                          Offset;
  UINT16                          Width;
  UINT8                           *Value;

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = 0;
  Width  = 0;
  Value  = NULL;

  VA_START (Args, Number);
  for (Index = 0; Index < Number; Index++) {
    BufCfgArray = (UINT8 *) VA_ARG (Args, VOID *);
    TotalLen = ReadUnaligned32 ((UINT32 *)BufCfgArray);
    BufferPos = BufCfgArray + sizeof (UINT32);

    while ((UINT32)(BufferPos - BufCfgArray) < TotalLen) {
      Offset = ReadUnaligned16 ((UINT16 *)BufferPos);
      BufferPos += sizeof (UINT16);
      Width = ReadUnaligned16 ((UINT16 *)BufferPos);
      BufferPos += sizeof (UINT16);
      Value = BufferPos;
      BufferPos += Width;

      if ((UINTN)(Offset + Width) > *BufferSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      CopyMem ((UINT8 *)Buffer + Offset, Value, Width);
    }
  }
  VA_END (Args);

  *BufferSize = (UINTN)Offset;

  return EFI_SUCCESS;
}
