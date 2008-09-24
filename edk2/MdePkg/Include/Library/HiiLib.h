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


#define ISO_639_2_ENTRY_SIZE            3

/**
  Assemble EFI_HII_PACKAGE_LIST according to the passed in packages.

  If GuidId is NULL, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  NumberOfPackages       Number of packages.
  @param  GuidId                 Package GUID.
  @param  ...                    Variable argument list for packages to be assembled.

  @return Pointer of EFI_HII_PACKAGE_LIST_HEADER.

**/
EFI_HII_PACKAGE_LIST_HEADER *
EFIAPI
HiiLibPreparePackageList (
  IN UINTN                    NumberOfPackages,
  IN CONST EFI_GUID                 *GuidId,
  ...
  );

/**
  This function allocates pool for an EFI_HII_PACKAGE_LIST structure
  with additional space that is big enough to host all packages described by the variable 
  argument list of package pointers.  The allocated structure is initialized using NumberOfPackages, 
  GuidId,  and the variable length argument list of package pointers.

  Then, EFI_HII_PACKAGE_LIST will be register to the default System HII Database. The
  Handle to the newly registered Package List is returned throught HiiHandle.

  If HiiHandle is NULL, then ASSERT.

  @param  NumberOfPackages    The number of HII packages to register.
  @param  GuidId              Package List GUID ID.
  @param  DriverHandle        Optional. If not NULL, the DriverHandle on which an instance of DEVICE_PATH_PROTOCOL is installed.
                              This DriverHandle uniquely defines the device that the added packages are associated with.
  @param  HiiHandle           On output, the HiiHandle is update with the handle which can be used to retrieve the Package 
                              List later. If the functions failed to add the package to the default HII database, this value will
                              be set to NULL.
  @param  ...                 The variable argument list describing all HII Package.

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
  );

/**
  Removes a package list from the default HII database.

  If HiiHandle is NULL, then ASSERT.
  If HiiHandle is not a valid EFI_HII_HANDLE in the default HII database, then ASSERT.

  @param  HiiHandle                The handle that was previously registered to the data base that is requested for removal.
                                             List later.

**/
VOID
EFIAPI
HiiLibRemovePackages (
  IN      EFI_HII_HANDLE      HiiHandle
  );

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
  );

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
  );

/**
  This function try to retrieve string from String package of current language.
  If fails, it try to retrieve string from String package of first language it support.

  If StringSize is NULL, then ASSERT.
  If String is NULL and *StringSize is not 0, then ASSERT.
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

**/
EFI_STATUS
EFIAPI
HiiLibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  );

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

**/
EFI_STATUS
EFIAPI
HiiLibGetStringFromHandle (
  IN  EFI_HII_HANDLE                  HiiHandle,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  );

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
  );

/**
  Determines the handles that are currently active in the database.
  It's the caller's responsibility to free handle buffer.

  If HandleBufferLength is NULL, then ASSERT.
  If HiiHandleBuffer is NULL, then ASSERT.

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
  );

/**
  Extract Hii package list GUID for given HII handle.

  If HiiHandle could not be found in the default HII database, then ASSERT.
  If Guid is NULL, then ASSERT.

  @param  Handle              Hii handle
  @param  Guid                Package list GUID

  @retval EFI_SUCCESS            Successfully extract GUID from Hii database.

**/
EFI_STATUS
EFIAPI
HiiLibExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     EFI_GUID            *Guid
  );

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
  );


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
  );

/**
  This function returns the list of supported languages, in the format specified
  in UEFI specification Appendix M.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.

  @param  HiiHandle              The HII package list handle.

  @retval   !NULL  The supported languages.
  @retval   NULL    If Supported Languages can not be retrived.

**/
CHAR8 *
EFIAPI
HiiLibGetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  );

/**
  This function returns the list of supported 2nd languages, in the format specified
  in UEFI specification Appendix M.

  If HiiHandle is not a valid Handle in the default HII database, then ASSERT.
  If not enough resource to complete the operation, then ASSERT.

  @param  HiiHandle              The HII package list handle.
  @param  FirstLanguage          Pointer to language name buffer.
  
  @return The supported languages.

**/
CHAR8 *
EFIAPI
HiiLibGetSupportedSecondaryLanguages (
  IN EFI_HII_HANDLE           HiiHandle,
  IN CONST CHAR8              *FirstLanguage
  );


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
  );

/**
  Exports the contents of one or all package lists in the HII database into a buffer.

  If Handle is not NULL and not a valid EFI_HII_HANDLE registered in the database, 
  then ASSERT.
  If PackageListHeader is NULL, then ASSERT.
  If PackageListSize is NULL, then ASSERT.

  @param  Handle                 The HII Handle.
  @param  PackageListHeader      A pointer to a buffer that will contain the results of 
                                 the export function.
  @param  PackageListSize        On output, the length of the buffer that is required for the exported data.

  @retval EFI_SUCCESS            Package exported.

  @retval EFI_OUT_OF_RESOURCES   Not enought memory to complete the operations.

**/
EFI_STATUS 
EFIAPI
HiiLibExportPackageLists (
  IN EFI_HII_HANDLE                    Handle,
  OUT EFI_HII_PACKAGE_LIST_HEADER      **PackageListHeader,
  OUT UINTN                            *PackageListSize
  );

/**
  
  This function returns a list of the package handles of the   
  specified type that are currently active in the HII database. The   
  pseudo-type EFI_HII_PACKAGE_TYPE_ALL will cause all package   
  handles to be listed.

  If HandleBufferLength is NULL, then ASSERT.
  If HandleBuffer is NULL, the ASSERT.
  If PackageType is EFI_HII_PACKAGE_TYPE_GUID and PackageGuid is
  NULL, then ASSERT.
  If PackageType is not EFI_HII_PACKAGE_TYPE_GUID and PackageGuid is not
  NULL, then ASSERT.
  
  
  @param PackageType          Specifies the package type of the packages
                              to list or EFI_HII_PACKAGE_TYPE_ALL for
                              all packages to be listed.
  
  @param PackageGuid          If PackageType is
                              EFI_HII_PACKAGE_TYPE_GUID, then this is
                              the pointer to the GUID which must match
                              the Guid field of
                              EFI_HII_PACKAGE_GUID_HEADER. Otherwise, it
                              must be NULL.
  
  @param HandleBufferLength   On output, the length of the handle buffer
                              that is required for the handles found.

  @param HandleBuffer         On output, an array of EFI_HII_HANDLE  instances returned.
                              The caller is responcible to free this pointer allocated.

  @retval EFI_SUCCESS           The matching handles are outputed successfully.
                                HandleBufferLength is updated with the actual length.
  @retval EFI_OUT_OF_RESOURCES  Not enough resource to complete the operation.
  @retval EFI_NOT_FOUND         No matching handle could not be found in database.
**/
EFI_STATUS
EFIAPI
HiiLibListPackageLists (
  IN        UINT8                     PackageType,
  IN CONST  EFI_GUID                  *PackageGuid,
  IN OUT    UINTN                     *HandleBufferLength,
  OUT       EFI_HII_HANDLE            **Handle
  );

/**
  Convert language code from RFC3066 to ISO639-2.

  LanguageRfc3066 contain a single RFC 3066 code such as
  "en-US" or "fr-FR".

  The LanguageRfc3066 must be a buffer large enough
  for ISO_639_2_ENTRY_SIZE characters.

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
  IN  CHAR8   *LanguageRfc3066,
  OUT CHAR8   *LanguageIso639
  );

/**
  Convert language code from ISO639-2 to RFC3066.

  LanguageIso639 contain a single ISO639-2 code such as
  "eng" or "fra".

  The LanguageRfc3066 must be a buffer large enough
  for RFC_3066_ENTRY_SIZE characters.

  If LanguageIso639 is NULL, then ASSERT.
  If LanguageRfc3066 is NULL, then ASSERT.

  @param  LanguageIso639         ISO639-2 language code.
  @param  LanguageRfc3066        RFC3066 language code.

  @retval EFI_SUCCESS            Language code converted.
  @retval EFI_NOT_FOUND          Language code not found.

**/
EFI_STATUS
EFIAPI
ConvertIso639LanguageToRfc3066Language (
  IN  CONST CHAR8   *LanguageIso639,
  OUT CHAR8         *LanguageRfc3066
  );

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
  );

#endif
