/** @file
  Public include file for the HII Library

  Copyright (c) 2007 - 2008, Intel Corporation                                                         
  All rights reserved. This program and the accompanying materials                          
  are licensed and made available under the terms and conditions of the BSD License         
  which accompanies this distribution.  The full text of the license may be found at        
  http://opensource.org/licenses/bsd-license.php                                            

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __HII_LIB_H__
#define __HII_LIB_H__

//
// Limited buffer size recommended by RFC4646 (4.3.  Length Considerations)
// (42 characters plus a NULL terminator)
//
#define RFC_3066_ENTRY_SIZE             (42 + 1)

#define ISO_639_2_ENTRY_SIZE            3

/**
  Assemble EFI_HII_PACKAGE_LIST according to the passed in packages.

  If GuidId is NULL, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  NumberOfPackages       Number of packages.
  @param  GuidId                          Package GUID.
  @param  ...                                Variable argument list for packages to be assembled.

  @return EFI_HII_PACKAGE_LIST_HEADER Pointer of EFI_HII_PACKAGE_LIST_HEADER. The function will ASSERT if system has
                                                                not enough resource to complete the operation.

**/
EFI_HII_PACKAGE_LIST_HEADER *
EFIAPI
HiiLibPreparePackageList (
  IN UINTN                    NumberOfPackages,
  IN CONST EFI_GUID                 *GuidId,
  ...
  )
;

/**
  This function allocates pool for an EFI_HII_PACKAGE_LIST structure
  with additional space that is big enough to host all packages described by the variable 
  argument list of package pointers.  The allocated structure is initialized using NumberOfPackages, 
  GuidId,  and the variable length argument list of package pointers.

  Then, EFI_HII_PACKAGE_LIST will be register to the default System HII Database. The
  Handle to the newly registered Package List is returned throught HiiHandle.

  If HiiHandle is NULL, then ASSERT.

  @param  NumberOfPackages  The number of HII packages to register.
  @param  GuidId                    Package List GUID ID.
  @param  EFI_HANDLE            Optional. If not NULL, the DriverHandle on which an instance of DEVICE_PATH_PROTOCOL is installed.
                                             This DriverHandle uniquely defines the device that the added packages are associated with.
  @param  HiiHandle                On output, the HiiHandle is update with the handle which can be used to retrieve the Package 
                                             List later. If the functions failed to add the package to the default HII database, this value will
                                             be set to NULL.
  @param  ...                          The variable argument list describing all HII Package.

  @return  EFI_SUCCESS         If the packages are successfully added to the default HII database.
  @return  EFI_OUT_OF_RESOURCE Not enough resource to complete the operation.

**/
EFI_STATUS
EFIAPI
HiiLibAddPackages (
  IN       UINTN               NumberOfPackages,
  IN CONST EFI_GUID            *GuidId,
  IN       EFI_HANDLE          DriverHandle, OPTIONAL
  OUT      EFI_HII_HANDLE      *HiiHandle,
  ...
  )
;

EFI_STATUS
EFIAPI
HiiLibAddFontPackageToHiiDatabase (
  IN       UINTN               FontSize,
  IN CONST UINT8               *FontBinary,
  IN CONST EFI_GUID            *GuidId,
  OUT      EFI_HII_HANDLE      *HiiHandle OPTIONAL
  )
;

/**
  Removes a package list from the default HII database.

  If HiiHandle is NULL, then ASSERT.
  If HiiHandle is not a valid EFI_HII_HANDLE in the default HII database, then ASSERT.

  @param  HiiHandle                The handle that was previously registered to the data base that is requested for removal.
                                             List later.

  @return  VOID

**/
VOID
EFIAPI
HiiLibRemovePackages (
  IN      EFI_HII_HANDLE      HiiHandle
  )
;

/**
  This function adds the string into String Package of each language
  supported by the package list.

  If String is NULL, then ASSERT.
  If StringId is NULL, the ASSERT.
  If PackageList could not be found in the default HII database, then ASSERT.

  @param  PackageList            Handle of the package list where this string will
                                            be added.
  @param  StringId               On return, contains the new strings id, which is
                                          unique within PackageList.
  @param  String                 Points to the new null-terminated string.

  @retval EFI_SUCCESS             The new string was added successfully.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.

**/
EFI_STATUS
EFIAPI
HiiLibNewString (
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST EFI_STRING                String
  )
;

/**
  This function update the specified string in String Package of each language
  supported by the package list.

  If String is NULL, then ASSERT.
  If PackageList could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  PackageList            Handle of the package list where this string will
                                            be added.
  @param  StringId               Ths String Id to be updated.
  @param  String                 Points to the new null-terminated string.

  @retval EFI_SUCCESS            The new string was added successfully.
  @retval EFI_OUT_OF_RESOURCES   Could not add the string due to lack of resources.

**/
EFI_STATUS
EFIAPI
HiiLibSetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  IN  CONST EFI_STRING                String
  )
;

/**
  This function try to retrieve string from String package of current language.
  If fails, it try to retrieve string from String package of first language it support.

  If String is NULL, then ASSERT.
  If StringSize is NULL, then ASSERT.
  If PackageList could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  PackageList     The package list in the HII database to search for
                                     the specified string.
  @param  StringId          The string's id, which is unique within
                                      PackageList.
  @param  String             Points to the new null-terminated string.
  @param  StringSize       On entry, points to the size of the buffer pointed
                                 to by String, in bytes. On return, points to the
                                 length of the string, in bytes.

  @retval EFI_SUCCESS            The string was returned successfully.
  @retval EFI_NOT_FOUND          The string specified by StringId is not available.
  @retval EFI_BUFFER_TOO_SMALL   The buffer specified by StringLength is too small
                                 to hold the string.
  @retval EFI_INVALID_PARAMETER  The String or StringSize was NULL.

**/

EFI_STATUS
EFIAPI
HiiLibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  )
;

/**
  Get string specified by StringId form the HiiHandle. The caller
  is responsible to free the *String.

  If String is NULL, then ASSERT.
  If HiiHandle could not be found in the default HII database, then ASSERT.
  If StringId is not found in PackageList, then ASSERT.

  @param  HiiHandle              The HII handle of package list.
  @param  StringId               The String ID.
  @param  String                 The output string.

  @retval EFI_NOT_FOUND          String is not found.
  @retval EFI_SUCCESS            Operation is successful.
  @retval EFI_OUT_OF_RESOURCES   There is not enought memory in the system.
  @retval EFI_INVALID_PARAMETER  The String is NULL.

**/
EFI_STATUS
EFIAPI
HiiLibGetStringFromHandle (
  IN  EFI_HII_HANDLE                  HiiHandle,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
;

/**
  Get the string given the StringId and String package Producer's Guid. The caller
  is responsible to free the *String.

  If PackageList with the matching ProducerGuid is not found, then ASSERT.
  If PackageList with the matching ProducerGuid is found but no String is
  specified by StringId is found, then ASSERT.

  @param  ProducerGuid           The Guid of String package list.
  @param  StringId               The String ID.
  @param  String                 The output string.

  @retval EFI_SUCCESS            Operation is successful.
  @retval EFI_OUT_OF_RESOURCES   There is not enought memory in the system.

**/
EFI_STATUS
EFIAPI
HiiLibGetStringFromToken (
  IN  EFI_GUID                        *ProducerGuid,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
;

/**
  Determines the handles that are currently active in the database.
  It's the caller's responsibility to free handle buffer.

  If HandleBufferLength is NULL, then ASSERT.
  If HiiHandleBuffer is NULL, then ASSERT.

  @param  HiiDatabase            A pointer to the EFI_HII_DATABASE_PROTOCOL
                                 instance.
  @param  HandleBufferLength     On input, a pointer to the length of the handle
                                 buffer. On output, the length of the handle buffer
                                 that is required for the handles found.
  @param  HiiHandleBuffer        Pointer to an array of Hii Handles returned.

  @retval EFI_SUCCESS            Get an array of Hii Handles successfully.

**/
EFI_STATUS
EFIAPI
HiiLibGetHiiHandles (
  IN OUT UINTN                     *HandleBufferLength,
  OUT    EFI_HII_HANDLE            **HiiHandleBuffer
  )
;

/**
  Extract Hii package list GUID for given HII handle.

  If HiiHandle could not be found in the default HII database, then ASSERT.
  If Guid is NULL, then ASSERT.

  @param  HiiHandle              Hii handle
  @param  Guid                   Package list GUID

  @retval EFI_SUCCESS            Successfully extract GUID from Hii database.

**/
EFI_STATUS
EFIAPI
HiiLibExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     EFI_GUID            *Guid
  )
;

/**
  Find HII Handle in the default HII database associated with given Device Path.

  If DevicePath is NULL, then ASSERT.

  @param  DevicePath             Device Path associated with the HII package list
                                 handle.

  @retval Handle                 HII package list Handle associated with the Device
                                        Path.
  @retval NULL                   Hii Package list handle is not found.

**/
EFI_HII_HANDLE
EFIAPI
HiiLibDevicePathToHiiHandle (
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
;


/**
  Determine what is the current language setting. The space reserved for Lang
  must be at least RFC_3066_ENTRY_SIZE bytes;

  If Lang is NULL, then ASSERT.

  @param  Lang                   Pointer of system language. Lang will always be filled with 
                                         a valid RFC 3066 language string. If "PlatformLang" is not
                                         set in the system, the default language specifed by PcdUefiVariableDefaultPlatformLang
                                         is returned.

  @return  EFI_SUCCESS     If the EFI Variable with "PlatformLang" is set and return in Lang.
  @return  EFI_NOT_FOUND If the EFI Variable with "PlatformLang" is not set, but a valid default language is return in Lang.

**/
EFI_STATUS
EFIAPI
HiiLibGetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
;

/**
  Get next language from language code list (with separator ';').

  If LangCode is NULL, then ASSERT.
  If Lang is NULL, then ASSERT.

  @param  LangCode    On input: point to first language in the list. On
                                 output: point to next language in the list, or
                                 NULL if no more language in the list.
  @param  Lang           The first language in the list.

**/
VOID
EFIAPI
HiiLibGetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
;

/**
  This function returns the list of supported languages, in the format specified
  in UEFI specification Appendix M.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  HiiHandle              The HII package list handle.

  @return The supported languages.

**/
CHAR8 *
EFIAPI
HiiLibGetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
;

/**
  This function returns the number of supported languages on HiiHandle.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  HiiHandle              The HII package list handle.

  @return The  number of supported languages.

**/
UINT16
EFIAPI
HiiLibGetSupportedLanguageNumber (
  IN EFI_HII_HANDLE           HiiHandle
  )
;

/**
  Convert language code from RFC3066 to ISO639-2.

  If LanguageRfc3066 is NULL, then ASSERT.
  If LanguageIso639 is NULL, then ASSERT.

  @param  LanguageRfc3066        RFC3066 language code.
  @param  LanguageIso639         ISO639-2 language code.

  @retval EFI_SUCCESS            Language code converted.
  @retval EFI_NOT_FOUND          Language code not found.

**/
EFI_STATUS
EFIAPI
ConvertRfc3066LanguageToIso639Language (
  CHAR8   *LanguageRfc3066,
  CHAR8   *LanguageIso639
  )
;

/**
  Convert language code list from RFC3066 to ISO639-2, e.g. "en-US;fr-FR" will
  be converted to "engfra".

  If SupportedLanguages is NULL, then ASSERT.

  @param  SupportedLanguages     The RFC3066 language list.

  @return The ISO639-2 language list.

**/
CHAR8 *
EFIAPI
Rfc3066ToIso639 (
  CHAR8  *SupportedLanguages
  )
;

#endif
