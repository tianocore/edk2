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
  )
;

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

  @param  Handle              Hii handle
  @param  Guid                Package list GUID

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

  @param  HiiHandle              The HII package list handle.

  @retval   !NULL  The supported languages.
  @retval   NULL    If Supported Languages can not be retrived.

**/
CHAR8 *
EFIAPI
HiiLibGetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
;

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
  )
;

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
  )
;

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
  )
;

/**
  Convert language code from ISO639-2 to RFC3066 and return the converted language.
  Caller is responsible for freeing the allocated buffer.

  LanguageIso639 contain a single ISO639-2 code such as
  "eng" or "fra".

  If LanguageIso639 is NULL, then ASSERT.
  If LanguageRfc3066 is NULL, then ASSERT.

  @param  LanguageIso639         ISO639-2 language code.

  @return the allocated buffer or NULL, if the language is not found.

**/
CHAR8*
EFIAPI
ConvertIso639LanguageToRfc3066Language (
  IN  CONST CHAR8   *LanguageIso639
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


////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// HiiLib Functions
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/////////////////////////////////////////
/////////////////////////////////////////
/// IFR Functions
/////////////////////////////////////////
/////////////////////////////////////////

/**
  Returns a UINT64 value that contains bitfields for Hour, Minute, and Second.
  The lower 8-bits of Hour are placed in bits 0..7.  The lower 8-bits of Minute 
  are placed in bits 8..15, and the lower 8-bits of Second are placed in bits 
  16..23.  This format is selected because it can be easily translated to 
  an EFI_HII_TIME structure in an EFI_IFR_TYPE_VALUE union.

  @param  Hour    The hour value to be encoded.
  @param  Minute  The miniute value to be encoded.
  @param  Second  The second value to be encoded.

  @return A 64-bit containing Hour, Minute, and Second.
**/
#define EFI_HII_TIME_UINT64(Hour, Minute, Second) \
  (UINT64)((Hour & 0xff) | ((Minute & 0xff) << 8) | ((Second & 0xff) << 16))

/**
  Returns a UINT64 value that contains bitfields for Year, Month, and Day.
  The lower 16-bits of Year are placed in bits 0..15.  The lower 8-bits of Month 
  are placed in bits 16..23, and the lower 8-bits of Day are placed in bits 
  24..31.  This format is selected because it can be easily translated to 
  an EFI_HII_DATE structure in an EFI_IFR_TYPE_VALUE union.

  @param  Year   The year value to be encoded.
  @param  Month  The month value to be encoded.
  @param  Day    The day value to be encoded.

  @return A 64-bit containing Year, Month, and Day.
**/
#define EFI_HII_DATE_UINT64(Year, Month, Day) \
  (UINT64)((Year & 0xffff) | ((Month & 0xff) << 16) | ((Day & 0xff) << 24))

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

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
  );

/**
  Configure the buffer accrording to ConfigBody strings in the format of
  <Length:4 bytes>, <Offset: 2 bytes>, <Width:2 bytes>, <Data:n bytes>.
  This ConfigBody strings is generated by EDKII UEFI VfrCompiler for the default
  values in a Form Set. The name of the ConfigBody strings is VfrMyIfrNVDataDefault0000
  constructed following this rule: 
   "Vfr" + varstore.name + "Default" + defaultstore.attributes.
  Check the generated C file in Output for details.

  @param  Buffer                 the start address of buffer.
  @param  BufferSize             the size of buffer.
  @param  Number                 the number of the ConfigBody strings.
  @param  ...                    the ConfigBody strings

  @retval EFI_BUFFER_TOO_SMALL   the BufferSize is too small to operate.
  @retval EFI_INVALID_PARAMETER  Buffer is NULL or BufferSize is 0.
  @retval EFI_SUCCESS            Operation successful.

**/
EFI_STATUS
EFIAPI
HiiIfrLibExtractDefault(
  IN VOID                         *Buffer,
  IN UINTN                        *BufferSize,
  UINTN                           Number,
  ...
  );

#endif
