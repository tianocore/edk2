/** @file
  Public include file for the HII Library

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials are licensed and made available under 
the terms and conditions of the BSD License that accompanies this distribution.  
The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php.                                            

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

**/

#ifndef __HII_LIB_H__
#define __HII_LIB_H__

////////////////////////////////////////////////////////
////////////////////////////////////////////////////////
// HiiLib Functions
////////////////////////////////////////////////////////
////////////////////////////////////////////////////////

/**
  Registers a list of packages in the HII Database and returns the HII Handle
  associated with that registration.  If an HII Handle has already been registered
  with the same PackageListGuid and DeviceHandle, then NULL is returned.  If there
  are not enough resources to perform the registration, then NULL is returned.
  If an empty list of packages is passed in, then NULL is returned.  If the size of
  the list of package is 0, then NULL is returned.

  The variable arguments are pointers that point to package headers defined 
  by UEFI VFR compiler and StringGather tool.

  #pragma pack (push, 1)
  typedef struct {
    UINT32                  BinaryLength;
    EFI_HII_PACKAGE_HEADER  PackageHeader;
  } EDKII_AUTOGEN_PACKAGES_HEADER;
  #pragma pack (pop)
  
  @param[in]  PackageListGuid  The GUID of the package list.
  @param[in]  DeviceHandle     If not NULL, the Device Handle on which 
                               an instance of DEVICE_PATH_PROTOCOL is installed.
                               This Device Handle uniquely defines the device that 
                               the added packages are associated with.
  @param[in]  ...              The variable argument list that contains pointers 
                               to packages terminated by a NULL.

  @retval NULL   An HII Handle has already been registered in the HII Database with
                 the same PackageListGuid and DeviceHandle.
  @retval NULL   The HII Handle could not be created.
  @retval NULL   An empty list of packages was passed in.
  @retval NULL   All packages are empty.
  @retval Other  The HII Handle associated with the newly registered package list.

**/
EFI_HII_HANDLE
EFIAPI
HiiAddPackages (
  IN CONST EFI_GUID    *PackageListGuid,
  IN       EFI_HANDLE  DeviceHandle  OPTIONAL,
  ...
  )
;

/**
  Removes a package list from the HII database.

  If HiiHandle is NULL, then ASSERT().
  If HiiHandle is not a valid EFI_HII_HANDLE in the HII database, then ASSERT().

  @param[in]  HiiHandle   The handle that was previously registered in the HII database

**/
VOID
EFIAPI
HiiRemovePackages (
  IN      EFI_HII_HANDLE      HiiHandle
  )
;

/**
  This function creates a new string in String Package or updates an existing 
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
                                  is updated in the String Package associated 
                                  with HiiHandle. 
  @param[in]  String              A pointer to the Null-terminated Unicode string 
                                  to add or update in the String Package associated 
                                  with HiiHandle.
  @param[in]  SupportedLanguages  A pointer to a Null-terminated ASCII string of 
                                  language codes.  If this parameter is NULL, then 
                                  String is added or updated in the String Package 
                                  associated with HiiHandle for all the languages 
                                  that the String Package supports.  If this 
                                  parameter is not NULL, then String is added 
                                  or updated in the String Package associated with 
                                  HiiHandle for the set of languages specified by 
                                  SupportedLanguages.  The format of 
                                  SupportedLanguages must follow the language 
                                  format assumed in the HII Database.

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
;

/**
  Retrieves a string from a string package in a specific language.  If the language
  is not specified, then a string from a string package in the current platform 
  language is retrieved.  If the string cannot be retrieved using the specified 
  language or the current platform language, then the string is retrieved from 
  the string package in the first language the string package supports.  The 
  returned string is allocated using AllocatePool().  The caller is responsible 
  for freeing the allocated buffer using FreePool().
  
  If HiiHandle is NULL, then ASSERT().
  If StringId is 0, then ASSERT().

  @param[in]  HiiHandle  A handle that was previously registered in the HII Database.
  @param[in]  StringId   The identifier of the string to retrieved from the string 
                         package associated with HiiHandle.
  @param[in]  Language   The language of the string to retrieve.  If this parameter 
                         is NULL, then the current platform language is used.  The 
                         format of Language must follow the language format assumed in
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
;

/**
  Retrieves a string from a string package named by GUID, in the specified language.  
  If the language is not specified, then a string from a string package in the 
  current platform  language is retrieved.  If the string cannot be retrieved 
  using the specified language or the current platform language, then the string 
  is retrieved from the string package in the first language the string package 
  supports.  The returned string is allocated using AllocatePool().  The caller 
  is responsible for freeing the allocated buffer using FreePool().
  
  If PackageListGuid is NULL, then ASSERT().
  If StringId is 0, then ASSERT().

  @param[in]  PackageListGuid  The GUID of a package list that was previously 
                               registered in the HII Database.
  @param[in]  StringId         The identifier of the string to retrieved from the 
                               string package associated with PackageListGuid.
  @param[in]  Language         The language of the string to retrieve.  If this 
                               parameter is NULL, then the current platform 
                               language is used.  The format of Language must 
                               follow the language format assumed in the HII Database.

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
;

/**
  Retrieves the array of all the HII Handles or the HII handles of a specific
  package list GUID in the HII Database.
  This array is terminated with a NULL HII Handle.
  This function allocates the returned array using AllocatePool().
  The caller is responsible for freeing the array with FreePool().

  @param[in]  PackageListGuid  An optional parameter that is used to request 
                               HII Handles associated with a specific
                               Package List GUID.  If this parameter is NULL,
                               then all the HII Handles in the HII Database
                               are returned.  If this parameter is not NULL,
                               then zero or more HII Handles associated with 
                               PackageListGuid are returned.

  @retval NULL   No HII handles were found in the HII database
  @retval NULL   The array of HII Handles could not be retrieved
  @retval Other  A pointer to the NULL terminated array of HII Handles

**/
EFI_HII_HANDLE *
EFIAPI
HiiGetHiiHandles (
  IN CONST EFI_GUID  *PackageListGuid  OPTIONAL
  )
;

/**
  Retrieves a pointer to a Null-terminated ASCII string containing the list 
  of languages that an HII handle in the HII Database supports.  The returned 
  string is allocated using AllocatePool().  The caller is responsible for freeing
  the returned string using FreePool().  The format of the returned string follows
  the language format assumed in the HII Database.
  
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
  IN EFI_HII_HANDLE           HiiHandle
  )
;

/**
  Allocates and returns a Null-terminated Unicode <ConfigHdr> string using routing 
  information that includes a GUID, an optional Unicode string name, and a device
  path. The string returned is allocated with AllocatePool().  The caller is 
  responsible for freeing the allocated string with FreePool().
  
  The format of a <ConfigHdr> is as follows:

    GUID=<HexCh>32&NAME=<Char>NameLength&PATH=<HexChar>DevicePathSize<Null>

  @param[in]  Guid          The pointer to an EFI_GUID that is the routing information
                            GUID.  Each of the 16 bytes in Guid is converted to 
                            a 2 Unicode character hexidecimal string.  This is 
                            an optional parameter that may be NULL.
  @param[in]  Name          The pointer to a Null-terminated Unicode string that is 
                            the routing information NAME.  This is an optional 
                            parameter that may be NULL.  Each 16-bit Unicode 
                            character in Name is converted to a 4 character Unicode 
                            hexidecimal string.                        
  @param[in]  DriverHandle  The driver handle that supports a Device Path Protocol
                            that is the routing information PATH.  Each byte of
                            the Device Path associated with DriverHandle is converted
                            to a two (Unicode) character hexidecimal string.

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
  Reset the default value specified by DefaultId to the driver
  configuration specified by the Request string. 

  NULL request string support depends on the ExportConfig interface of
  HiiConfigRouting protocol in UEFI specification.
  
  @param Request    A null-terminated Unicode string in 
                    <MultiConfigRequest> format. It can be NULL.
                    If it is NULL, all configurations for the
                    entirety of the current HII database will be reset.
  @param DefaultId  Specifies the type of defaults to retrieve.
  
  @retval TURE    The default value was set successfully.
  @retval FALSE   The default value was not found.
**/
BOOLEAN
EFIAPI                               
HiiSetToDefaults (     
  IN CONST EFI_STRING  Request,  OPTIONAL
  IN UINT16            DefaultId
  );

/**
  Validate the current configuration by parsing the IFR opcode in HII form.

  NULL request string support depends on the ExportConfig interface of
  HiiConfigRouting protocol in the UEFI specification.
  
  @param  Request   A null-terminated Unicode string in 
                    <MultiConfigRequest> format. It can be NULL.
                    If it is NULL, all current configurations for the
                    entirety of the current HII database will be validated.
  
  @retval TRUE    The current configuration is valid.
  @retval FALSE   The current configuration is invalid.
**/
BOOLEAN
EFIAPI                               
HiiValidateSettings (
  IN CONST EFI_STRING  Request  OPTIONAL
  );

/**
  Determines if the routing data specified by GUID and NAME match a <ConfigHdr>.

  If ConfigHdr is NULL, then ASSERT().

  @param[in] ConfigHdr  Either <ConfigRequest> or <ConfigResp>.
  @param[in] Guid       The GUID of the storage.
  @param[in] Name       The NAME of the storage.

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
  Retrieves uncommitted data from the Form Browser and converts it to a binary
  buffer.

  @param[in]  VariableGuid  The pointer to an EFI_GUID structure.  This is an optional 
                            parameter that may be NULL.
  @param[in]  VariableName  The pointer to a Null-terminated Unicode string.  This 
                            is an optional parameter that may be NULL.
  @param[in]  BufferSize    The length in bytes of buffer to hold retrieved data. 
  @param[out] Buffer        The buffer of data to be updated.

  @retval FALSE  The uncommitted data could not be retrieved.
  @retval TRUE   The uncommitted data was retrieved.

**/
BOOLEAN
EFIAPI
HiiGetBrowserData (
  IN CONST EFI_GUID  *VariableGuid,  OPTIONAL
  IN CONST CHAR16    *VariableName,  OPTIONAL
  IN UINTN           BufferSize,
  OUT UINT8          *Buffer
  );

/**
  Updates uncommitted data in the Form Browser.

  If Buffer is NULL, then ASSERT().

  @param[in]  VariableGuid    The pointer to an EFI_GUID structure.  This is an optional
                              parameter that may be NULL.
  @param[in]  VariableName    The pointer to a Null-terminated Unicode string.  This
                              is an optional parameter that may be NULL.
  @param[in]  BufferSize      The length, in bytes, of Buffer.
  @param[in]  Buffer          The buffer of data to commit.
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
  16..23.  This format was selected because it can be easily translated to 
  an EFI_HII_TIME structure in an EFI_IFR_TYPE_VALUE union.

  @param  Hour    The hour value to be encoded.
  @param  Minute  The minute value to be encoded.
  @param  Second  The second value to be encoded.

  @return A 64-bit containing Hour, Minute, and Second.
**/
#define EFI_HII_TIME_UINT64(Hour, Minute, Second) \
  (UINT64)((Hour & 0xff) | ((Minute & 0xff) << 8) | ((Second & 0xff) << 16))

/**
  Returns a UINT64 value that contains bit fields for Year, Month, and Day.
  The lower 16-bits of Year are placed in bits 0..15.  The lower 8-bits of Month 
  are placed in bits 16..23, and the lower 8-bits of Day are placed in bits 
  24..31.  This format was selected because it can be easily translated to 
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
  Frees an OpCode Handle that was previously allocated with HiiAllocateOpCodeHandle().
  When an OpCode Handle is freed, all of the opcodes associated with the OpCode
  Handle are also freed.

  If OpCodeHandle is NULL, then ASSERT().

  @param[in]  OpCodeHandle   The handle to the buffer of opcodes.

**/
VOID
EFIAPI
HiiFreeOpCodeHandle (
  VOID  *OpCodeHandle
  );

/**
  Append raw opcodes to an OpCodeHandle.

  If OpCodeHandle is NULL, then ASSERT().
  If RawBuffer is NULL, then ASSERT();

  @param[in]  OpCodeHandle   The handle to the buffer of opcodes.
  @param[in]  RawBuffer      The buffer of opcodes to append.
  @param[in]  RawBufferSize  The size, in bytes, of Buffer.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the appended opcodes.

**/
UINT8 *
EFIAPI
HiiCreateRawOpCodes (
  IN VOID   *OpCodeHandle,
  IN UINT8  *RawBuffer,
  IN UINTN  RawBufferSize
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

  @param[in]  OpCodeHandle  The handle to the buffer of opcodes.
  @param[in]  StringId      StringId for the option.
  @param[in]  Flags         The flags for the option.
  @param[in]  Type          The type for the option.
  @param[in]  Value         The value for the option.

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

  @param[in]  OpCodeHandle  The handle to the buffer of opcodes.
  @param[in]  DefaultId     The DefaultId for the default.
  @param[in]  Type          The type for the default.
  @param[in]  Value         The value for the default.

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

  @param[in]  OpCodeHandle  The handle to the buffer of opcodes.
  @param[in]  Guid          The pointer to EFI_GUID of this guided opcode.
  @param[in]  GuidOpCode    The pointer to an EFI_IFR_GUID opcode.  This is an 
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

  @param[in]  OpCodeHandle  The handle to the buffer of opcodes.
  @param[in]  QuestionId      The Question ID.
  @param[in]  Prompt          The String ID for Prompt.
  @param[in]  Help            The String ID for Help.
  @param[in]  QuestionFlags   The flags in the Question Header.
  @param[in]  QuestionConfig  The String ID for the configuration.

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

  @param[in]  OpCodeHandle  The handle to the buffer of opcodes.
  @param[in]  Prompt      The string ID for Prompt.
  @param[in]  Help        The string ID for Help.
  @param[in]  Flags       The subtitle opcode flags.
  @param[in]  Scope       1 if this opcode is the beginning of a new scope.
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

  @param[in]  OpCodeHandle   The handle to the buffer of opcodes.
  @param[in]  FormId         The Destination Form ID.
  @param[in]  Prompt         The string ID for Prompt.
  @param[in]  Help           The string ID for Help.
  @param[in]  QuestionFlags  The flags in Question Header
  @param[in]  QuestionId     Question ID.

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

  @param[in]  OpCodeHandle          The handle to the buffer of opcodes.
  @param[in]  QuestionId            The question ID.
  @param[in]  VarStoreId            The storage ID.
  @param[in]  VarOffset             The offset in Storage.
  @param[in]  Prompt                The string ID for Prompt.
  @param[in]  Help                  The string ID for Help.
  @param[in]  QuestionFlags         The flags in Question Header.
  @param[in]  CheckBoxFlags         The flags for checkbox opcode.
  @param[in]  DefaultsOpCodeHandle  The handle for a buffer of DEFAULT opcodes.  This
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

  @param[in]  OpCodeHandle          The handle to the buffer of opcodes.
  @param[in]  QuestionId            The question ID.
  @param[in]  VarStoreId            The storage ID.
  @param[in]  VarOffset             The offset in Storage.
  @param[in]  Prompt                The string ID for Prompt.
  @param[in]  Help                  The string ID for Help.
  @param[in]  QuestionFlags         The flags in Question Header.
  @param[in]  NumericFlags          The flags for a numeric opcode.
  @param[in]  Minimum               The numeric minimum value.
  @param[in]  Maximum               The numeric maximum value.
  @param[in]  Step                  The numeric step for edit.
  @param[in]  DefaultsOpCodeHandle  The handle for a buffer of DEFAULT opcodes.  This
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

  @param[in]  OpCodeHandle          The handle to the buffer of opcodes.
  @param[in]  QuestionId            The question ID.
  @param[in]  VarStoreId            The storage ID.
  @param[in]  VarOffset             The offset in Storage.
  @param[in]  Prompt                The string ID for Prompt.
  @param[in]  Help                  The string ID for Help.
  @param[in]  QuestionFlags         The flags in Question Header.
  @param[in]  StringFlags           The flags for a string opcode.
  @param[in]  MinSize               The string minimum length.
  @param[in]  MaxSize               The string maximum length.
  @param[in]  DefaultsOpCodeHandle  The handle for a buffer of DEFAULT opcodes.  This
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

  @param[in]  OpCodeHandle          The handle to the buffer of opcodes.
  @param[in]  QuestionId            The question ID.
  @param[in]  VarStoreId            The storage ID.
  @param[in]  VarOffset             The offset in Storage.
  @param[in]  Prompt                The string ID for Prompt.
  @param[in]  Help                  The string ID for Help.
  @param[in]  QuestionFlags         The flags in Question Header.
  @param[in]  OneOfFlags            The flags for a oneof opcode.
  @param[in]  OptionsOpCodeHandle   The handle for a buffer of ONE_OF_OPTION opcodes.
  @param[in]  DefaultsOpCodeHandle  The handle for a buffer of DEFAULT opcodes.  This
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

  @param[in]  OpCodeHandle          The handle to the buffer of opcodes.
  @param[in]  QuestionId            The question ID.
  @param[in]  VarStoreId            The storage ID.
  @param[in]  VarOffset             The offset in Storage.
  @param[in]  Prompt                The string ID for Prompt.
  @param[in]  Help                  The string ID for Help.
  @param[in]  QuestionFlags         The flags in Question Header.
  @param[in]  OrderedListFlags      The flags for an ordered list opcode.
  @param[in]  DataType              The type for option value.
  @param[in]  MaxContainers         Maximum count for options in this ordered list
  @param[in]  OptionsOpCodeHandle   The handle for a buffer of ONE_OF_OPTION opcodes.
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
  Create EFI_IFR_TEXT_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().

  @param[in]  OpCodeHandle  Handle to the buffer of opcodes.
  @param[in]  Prompt        String ID for Prompt.
  @param[in]  Help          String ID for Help.
  @param[in]  TextTwo       String ID for TextTwo.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateTextOpCode (
  IN VOID           *OpCodeHandle,
  IN EFI_STRING_ID  Prompt,
  IN EFI_STRING_ID  Help,
  IN EFI_STRING_ID  TextTwo
  );

/**
  Create EFI_IFR_DATE_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in DateFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID, optional. If DateFlags is not
                                    QF_DATE_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  VarOffset             Offset in Storage, optional. If DateFlags is not
                                    QF_DATE_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  DateFlags             Flags for date opcode
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateDateOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,   OPTIONAL
  IN UINT16           VarOffset,    OPTIONAL
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            DateFlags,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  );

/**
  Create EFI_IFR_TIME_OP opcode.

  If OpCodeHandle is NULL, then ASSERT().
  If any reserved bits are set in QuestionFlags, then ASSERT().
  If any reserved bits are set in TimeFlags, then ASSERT().

  @param[in]  OpCodeHandle          Handle to the buffer of opcodes.
  @param[in]  QuestionId            Question ID
  @param[in]  VarStoreId            Storage ID, optional. If TimeFlags is not
                                    QF_TIME_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  VarOffset             Offset in Storage, optional. If TimeFlags is not
                                    QF_TIME_STORAGE_NORMAL, this parameter is ignored.
  @param[in]  Prompt                String ID for Prompt
  @param[in]  Help                  String ID for Help
  @param[in]  QuestionFlags         Flags in Question Header
  @param[in]  TimeFlags             Flags for time opcode
  @param[in]  DefaultsOpCodeHandle  Handle for a buffer of DEFAULT opcodes.  This
                                    is an optional parameter that may be NULL.

  @retval NULL   There is not enough space left in Buffer to add the opcode.
  @retval Other  A pointer to the created opcode.

**/
UINT8 *
EFIAPI
HiiCreateTimeOpCode (
  IN VOID             *OpCodeHandle,
  IN EFI_QUESTION_ID  QuestionId,
  IN EFI_VARSTORE_ID  VarStoreId,   OPTIONAL
  IN UINT16           VarOffset,    OPTIONAL
  IN EFI_STRING_ID    Prompt,
  IN EFI_STRING_ID    Help,
  IN UINT8            QuestionFlags,
  IN UINT8            TimeFlags,
  IN VOID             *DefaultsOpCodeHandle  OPTIONAL
  );

/**
  This function updates a form that has previously been registered with the HII 
  Database.  This function will perform at most one update operation.
    
  The form to update is specified by Handle, FormSetGuid, and FormId.  Binary 
  comparisons of IFR opcodes are performed from the beginning of the form being 
  updated until an IFR opcode is found that exactly matches the first IFR opcode 
  specified by StartOpCodeHandle.  The following rules are used to determine if
  an insert, replace, or delete operation is performed:
  
  1) If no matches are found, then NULL is returned.  
  2) If a match is found, and EndOpCodeHandle is NULL, then all of the IFR opcodes
     from StartOpCodeHandle except the first opcode are inserted immediately after 
     the matching IFR opcode in the form to be updated.
  3) If a match is found, and EndOpCodeHandle is not NULL, then a search is made 
     from the matching IFR opcode until an IFR opcode exactly matches the first 
     IFR opcode specified by EndOpCodeHandle.  If no match is found for the first
     IFR opcode specified by EndOpCodeHandle, then NULL is returned.  If a match
     is found, then all of the IFR opcodes between the start match and the end 
     match are deleted from the form being updated and all of the IFR opcodes
     from StartOpCodeHandle except the first opcode are inserted immediately after 
     the matching start IFR opcode.  If StartOpCcodeHandle only contains one
     IFR instruction, then the result of this operation will delete all of the IFR
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
                                 may be NULL.  If it is NULL, then the IFR
                                 opcodes specified by StartOpCodeHandle are 
                                 inserted into the form.
  
  @retval EFI_OUT_OF_RESOURCES   Not enough memory resources are allocated.
  @retval EFI_NOT_FOUND          The following cases will return EFI_NOT_FOUND:
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
  IN VOID            *StartOpCodeHandle,
  IN VOID            *EndOpCodeHandle     OPTIONAL
  );

#endif
