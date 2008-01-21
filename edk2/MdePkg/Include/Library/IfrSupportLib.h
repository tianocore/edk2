/*++

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UefiIfrLibrary.h

Abstract:

  The file contain all library function for Ifr Operations.

--*/

#ifndef _IFRLIBRARY_H
#define _IFRLIBRARY_H


#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/SimpleTextOut.h>

#include <Guid/GlobalVariable.h>

#define IFR_LIB_DEFAULT_STRING_SIZE     0x200

//
// The architectural variable "Lang" and "LangCodes" are deprecated in UEFI
// specification. While, UEFI specification also states that these deprecated
// variables may be provided for backwards compatibility.
// If "LANG_SUPPORT" is defined, "Lang" and "LangCodes" will be produced;
// If "LANG_SUPPORT" is undefined, "Lang" and "LangCodes" will not be produced.
//
#define LANG_SUPPORT

#define EFI_LANGUAGE_VARIABLE           L"Lang"
#define EFI_LANGUAGE_CODES_VARIABLE     L"LangCodes"

#define UEFI_LANGUAGE_VARIABLE          L"PlatformLang"
#define UEFI_LANGUAGE_CODES_VARIABLE    L"PlatformLangCodes"

//
// Limited buffer size recommended by RFC4646 (4.3.  Length Considerations)
// (42 characters plus a NULL terminator)
//
#define RFC_3066_ENTRY_SIZE             (42 + 1)
#define ISO_639_2_ENTRY_SIZE            3

#define INVALID_VARSTORE_ID             0

#define QUESTION_FLAGS              (EFI_IFR_FLAG_READ_ONLY | EFI_IFR_FLAG_CALLBACK | EFI_IFR_FLAG_RESET_REQUIRED | EFI_IFR_FLAG_OPTIONS_ONLY)
#define QUESTION_FLAGS_MASK         (~QUESTION_FLAGS)

extern EFI_HII_DATABASE_PROTOCOL *gIfrLibHiiDatabase;
extern EFI_HII_STRING_PROTOCOL   *gIfrLibHiiString;

#pragma pack(1)
typedef struct {
  EFI_STRING_ID       StringToken;
  EFI_IFR_TYPE_VALUE  Value;
  UINT8               Flags;
} IFR_OPTION;
#pragma pack()

typedef struct {
  //
  // Buffer size allocated for Data.
  //
  UINT32                BufferSize;

  //
  // Offset in Data to append the newly created opcode binary.
  // It will be adjusted automatically in Create***OpCode(), and should be
  // initialized to 0 before invocation of a serial of Create***OpCode()
  //
  UINT32                Offset;

  //
  // The destination buffer for created op-codes
  //
  UINT8                 *Data;
} EFI_HII_UPDATE_DATA;


//
// Exported Library functions
//
EFI_STATUS
CreateEndOpCode (
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
/*++

Routine Description:
  Create EFI_IFR_END_OP opcode.

Arguments:
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateDefaultOpCode (
  IN     EFI_IFR_TYPE_VALUE  *Value,
  IN     UINT8               Type,
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
/*++

Routine Description:
  Create EFI_IFR_DEFAULT_OP opcode.

Arguments:
  Value           - Value for the default
  Type            - Type for the default
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateActionOpCode (
  IN     EFI_QUESTION_ID      QuestionId,
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     UINT8                QuestionFlags,
  IN     EFI_STRING_ID        QuestionConfig,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
/*++

Routine Description:
  Create EFI_IFR_ACTION_OP opcode.

Arguments:
  QuestionId      - Question ID
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  QuestionConfig  - String ID for configuration
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateSubTitleOpCode (
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               Flags,
  IN      UINT8               Scope,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
/*++

Routine Description:
  Create EFI_IFR_SUBTITLE_OP opcode.

Arguments:
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  Flags           - Subtitle opcode flags
  Scope           - Subtitle Scope bit
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateTextOpCode (
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      EFI_STRING_ID       TextTwo,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  )
/*++

Routine Description:
  Create EFI_IFR_TEXT_OP opcode.

Arguments:
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  TextTwo         - String ID for text two
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateGotoOpCode (
  IN      EFI_FORM_ID         FormId,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      EFI_QUESTION_ID     QuestionId,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  )
/*++

Routine Description:
  Create EFI_IFR_REF_OP opcode.

Arguments:
  FormId          - Destination Form ID
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  QuestionId      - Question ID
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateOneOfOptionOpCode (
  IN     UINTN                OptionCount,
  IN     IFR_OPTION           *OptionsList,
  IN     UINT8                Type,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
;

EFI_STATUS
CreateOneOfOpCode (
  IN     EFI_QUESTION_ID      QuestionId,
  IN     EFI_VARSTORE_ID      VarStoreId,
  IN     UINT16               VarOffset,
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     UINT8                QuestionFlags,
  IN     UINT8                OneOfFlags,
  IN     IFR_OPTION           *OptionsList,
  IN     UINTN                OptionCount,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
/*++

Routine Description:
  Create EFI_IFR_ONE_OF_OP opcode.

Arguments:
  QuestionId      - Question ID
  VarStoreId      - Storage ID
  VarOffset       - Offset in Storage
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  OneOfFlags      - Flags for oneof opcode
  OptionsList     - List of options
  OptionCount     - Number of options in option list
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateOrderedListOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               Flags,
  IN      UINT8               DataType,
  IN      UINT8               MaxContainers,
  IN      IFR_OPTION          *OptionsList,
  IN     UINTN                OptionCount,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
/*++

Routine Description:
  Create EFI_IFR_ORDERED_LIST_OP opcode.

Arguments:
  QuestionId      - Question ID
  VarStoreId      - Storage ID
  VarOffset       - Offset in Storage
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  Flags           - Flags for ordered list opcode
  DataType        - Type for option value
  MaxContainers   - Maximum count for options in this ordered list
  OptionsList     - List of options
  OptionCount     - Number of options in option list
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateCheckBoxOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               CheckBoxFlags,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
/*++

Routine Description:
  Create EFI_IFR_CHECKBOX_OP opcode.

Arguments:
  QuestionId      - Question ID
  VarStoreId      - Storage ID
  VarOffset       - Offset in Storage
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  CheckBoxFlags   - Flags for checkbox opcode
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateNumericOpCode (
  IN     EFI_QUESTION_ID     QuestionId,
  IN     EFI_VARSTORE_ID     VarStoreId,
  IN     UINT16              VarOffset,
  IN     EFI_STRING_ID       Prompt,
  IN     EFI_STRING_ID       Help,
  IN     UINT8               QuestionFlags,
  IN     UINT8               NumericFlags,
  IN     UINT64              Minimum,
  IN     UINT64              Maximum,
  IN     UINT64              Step,
  IN     UINT64              Default,
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
/*++

Routine Description:
  Create EFI_IFR_NUMERIC_OP opcode.

Arguments:
  QuestionId      - Question ID
  VarStoreId      - Storage ID
  VarOffset       - Offset in Storage
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  NumericFlags    - Flags for numeric opcode
  Minimum         - Numeric minimum value
  Maximum         - Numeric maximum value
  Step            - Numeric step for edit
  Default         - Numeric default value
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateStringOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               StringFlags,
  IN      UINT8               MinSize,
  IN      UINT8               MaxSize,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
/*++

Routine Description:
  Create EFI_IFR_STRING_OP opcode.

Arguments:
  QuestionId      - Question ID
  VarStoreId      - Storage ID
  VarOffset       - Offset in Storage
  Prompt          - String ID for Prompt
  Help            - String ID for Help
  QuestionFlags   - Flags in Question Header
  StringFlags     - Flags for string opcode
  MinSize         - String minimum length
  MaxSize         - String maximum length
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_STATUS
CreateBannerOpCode (
  IN      EFI_STRING_ID       Title,
  IN      UINT16              LineNumber,
  IN      UINT8               Alignment,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  )
/*++

Routine Description:
  Create GUIDed opcode for banner.

Arguments:
  Title           - String ID for title
  LineNumber      - Line number for this banner
  Alignment       - Alignment for this banner, left, center or right
  Data            - Destination for the created opcode binary

Returns:
  EFI_SUCCESS     - Opcode create success

--*/
;

EFI_HII_PACKAGE_LIST_HEADER *
PreparePackageList (
  IN UINTN                    NumberOfPackages,
  IN EFI_GUID                 *GuidId,
  ...
  )
/*++

Routine Description:
  Assemble EFI_HII_PACKAGE_LIST according to the passed in packages.

Arguments:
  NumberOfPackages  -  Number of packages.
  GuidId            -  Package GUID.

Returns:
  Pointer of EFI_HII_PACKAGE_LIST_HEADER.

--*/
;

EFI_HII_HANDLE
DevicePathToHiiHandle (
  IN EFI_HII_DATABASE_PROTOCOL  *HiiDatabase,
  IN EFI_DEVICE_PATH_PROTOCOL   *DevicePath
  )
/*++

Routine Description:
  Find HII Handle associated with given Device Path.

Arguments:
  HiiDatabase - Point to EFI_HII_DATABASE_PROTOCOL instance.
  DevicePath  - Device Path associated with the HII package list handle.

Returns:
  Handle - HII package list Handle associated with the Device Path.
  NULL   - Hii Package list handle is not found.

--*/
;

EFI_STATUS
ExtractDefault(
  IN VOID                         *Buffer,
  IN UINTN                        *BufferSize,
  UINTN                           Number,
  ...
  )
/*++

  Routine Description:
    Configure the buffer accrording to ConfigBody strings.

  Arguments:
    DefaultId             - the ID of default.
    Buffer                - the start address of buffer.
    BufferSize            - the size of buffer.
    Number                - the number of the strings.

  Returns:
    EFI_BUFFER_TOO_SMALL  - the BufferSize is too small to operate.
    EFI_INVALID_PARAMETER - Buffer is NULL or BufferSize is 0.
    EFI_SUCCESS           - Operation successful.

--*/
;

EFI_STATUS
ExtractGuidFromHiiHandle (
  IN      EFI_HII_HANDLE      Handle,
  OUT     EFI_GUID            *Guid
  )
/*++

Routine Description:
  Extract Hii package list GUID for given HII handle.

Arguments:
  HiiHandle     - Hii handle
  Guid          - Package list GUID

Returns:
  EFI_SUCCESS   - Successfully extract GUID from Hii database.

--*/
;

EFI_STATUS
BufferToHexString (
  IN OUT CHAR16    *Str,
  IN UINT8         *Buffer,
  IN UINTN         BufferSize
  )
/*++

Routine Description:
  Converts binary buffer to Unicode string in reversed byte order to BufToHexString().

Arguments:
  Str        -  String for output
  Buffer     -  Binary buffer.
  BufferSize -  Size of the buffer in bytes.

Returns:
  EFI_SUCCESS    -  The function completed successfully.

--*/
;

EFI_STATUS
HexStringToBuffer (
  IN OUT UINT8         *Buffer,
  IN OUT UINTN         *BufferSize,
  IN CHAR16            *Str
  )
/*++

Routine Description:
  Converts Hex String to binary buffer in reversed byte order to HexStringToBuf().

Arguments:
    Buffer     - Pointer to buffer that receives the data.
    BufferSize - Length in bytes of the buffer to hold converted data.
                 If routine return with EFI_SUCCESS, containing length of converted data.
                 If routine return with EFI_BUFFER_TOO_SMALL, containg length of buffer desired.
    Str        - String to be converted from.

Returns:
  EFI_SUCCESS    -  The function completed successfully.

--*/
;

EFI_STATUS
ConstructConfigHdr (
  IN OUT CHAR16                *ConfigHdr,
  IN OUT UINTN                 *StrBufferLen,
  IN EFI_GUID                  *Guid,
  IN CHAR16                    *Name, OPTIONAL
  IN EFI_HANDLE                *DriverHandle
  )
/*++

Routine Description:
  Construct <ConfigHdr> using routing information GUID/NAME/PATH.

Arguments:
  ConfigHdr    - Pointer to the ConfigHdr string.
  StrBufferLen - On input: Length in bytes of buffer to hold the ConfigHdr string. Includes tailing '\0' character.
                 On output:
                    If return EFI_SUCCESS, containing length of ConfigHdr string buffer.
                    If return EFI_BUFFER_TOO_SMALL, containg length of string buffer desired.
  Guid         - Routing information: GUID.
  Name         - Routing information: NAME.
  DriverHandle  - Driver handle which contains the routing information: PATH.

Returns:
  EFI_SUCCESS          - Routine success.
  EFI_BUFFER_TOO_SMALL - The ConfigHdr string buffer is too small.

--*/
;

BOOLEAN
FindBlockName (
  IN OUT CHAR16                *String,
  UINTN                        Offset,
  UINTN                        Width
  )
/*++

Routine Description:
  Search BlockName "&OFFSET=Offset&WIDTH=Width" in a string.

Arguments:
  String       - The string to be searched in.
  Offset       - Offset in BlockName.
  Width        - Width in BlockName.

Returns:
  TRUE         - Block name found.
  FALSE        - Block name not found.

--*/
;

EFI_STATUS
GetBrowserData (
  EFI_GUID                   *VariableGuid, OPTIONAL
  CHAR16                     *VariableName, OPTIONAL
  UINTN                      *BufferSize,
  UINT8                      *Buffer
  )
/*++

Routine Description:
  This routine is invoked by ConfigAccess.Callback() to retrived uncommitted data from Form Browser.

Arguments:
  VariableGuid  - An optional field to indicate the target variable GUID name to use.
  VariableName  - An optional field to indicate the target human-readable variable name.
  BufferSize    - On input: Length in bytes of buffer to hold retrived data.
                  On output:
                    If return EFI_BUFFER_TOO_SMALL, containg length of buffer desired.
  Buffer        - Buffer to hold retrived data.

Returns:
  EFI_SUCCESS          - Routine success.
  EFI_BUFFER_TOO_SMALL - The intput buffer is too small.

--*/
;

EFI_STATUS
GetHiiHandles (
  IN OUT UINTN                     *HandleBufferLength,
  OUT    EFI_HII_HANDLE            **HiiHandleBuffer
  )
/*++

Routine Description:
  Determines the handles that are currently active in the database.
  It's the caller's responsibility to free handle buffer.

Arguments:
  HiiDatabase           - A pointer to the EFI_HII_DATABASE_PROTOCOL instance.
  HandleBufferLength    - On input, a pointer to the length of the handle buffer. On output,
                          the length of the handle buffer that is required for the handles found.
  HiiHandleBuffer       - Pointer to an array of Hii Handles returned.

Returns:
  EFI_SUCCESS           - Get an array of Hii Handles successfully.
  EFI_INVALID_PARAMETER - Hii is NULL.
  EFI_NOT_FOUND         - Database not found.

--*/
;

EFI_STATUS
SetBrowserData (
  EFI_GUID                   *VariableGuid, OPTIONAL
  CHAR16                     *VariableName, OPTIONAL
  UINTN                      BufferSize,
  UINT8                      *Buffer,
  CHAR16                     *RequestElement  OPTIONAL
  )
/*++

Routine Description:
  This routine is invoked by ConfigAccess.Callback() to update uncommitted data of Form Browser.

Arguments:
  VariableGuid   - An optional field to indicate the target variable GUID name to use.
  VariableName   - An optional field to indicate the target human-readable variable name.
  BufferSize     - Length in bytes of buffer to hold retrived data.
  Buffer         - Buffer to hold retrived data.
  RequestElement - An optional field to specify which part of the buffer data
                   will be send back to Browser. If NULL, the whole buffer of
                   data will be committed to Browser.
                   <RequestElement> ::= &OFFSET=<Number>&WIDTH=<Number>*

Returns:
  EFI_SUCCESS  - Routine success.
  Other        - Updating Browser uncommitted data failed.

--*/
;

EFI_STATUS
ConvertRfc3066LanguageToIso639Language (
  CHAR8   *LanguageRfc3066,
  CHAR8   *LanguageIso639
  )
/*++

Routine Description:
  Convert language code from RFC3066 to ISO639-2.

Arguments:
  LanguageRfc3066 - RFC3066 language code.
  LanguageIso639  - ISO639-2 language code.

Returns:
  EFI_SUCCESS   - Language code converted.
  EFI_NOT_FOUND - Language code not found.

--*/
;

CHAR8 *
Rfc3066ToIso639 (
  CHAR8  *SupportedLanguages
  )
/*++

Routine Description:
  Convert language code list from RFC3066 to ISO639-2, e.g. "en-US;fr-FR" will
  be converted to "engfra".

Arguments:
  SupportedLanguages - The RFC3066 language list.

Returns:
  The ISO639-2 language list.

--*/
;

EFI_STATUS
GetCurrentLanguage (
  OUT     CHAR8               *Lang
  )
/*++

Routine Description:
  Determine what is the current language setting

Arguments:
  Lang      - Pointer of system language

Returns:
  Status code

--*/
;

VOID
GetNextLanguage (
  IN OUT CHAR8      **LangCode,
  OUT CHAR8         *Lang
  )
/*++

Routine Description:
  Get next language from language code list.

Arguments:
  LangCode - The language code.
  Lang     - Returned language.

Returns:
  None.

--*/
;

CHAR8 *
GetSupportedLanguages (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  This function returns the list of supported languages, in the format specified
  in UEFI specification Appendix M.

Arguments:
  HiiHandle  - The HII package list handle.

Returns:
  The supported languages.

--*/
;

UINT16
GetSupportedLanguageNumber (
  IN EFI_HII_HANDLE           HiiHandle
  )
/*++

Routine Description:
  This function returns the number of supported languages

Arguments:
  HiiHandle  - The HII package list handle.

Returns:
  The  number of supported languages.

--*/
;

EFI_STATUS
GetStringFromHandle (
  IN  EFI_HII_HANDLE                  HiiHandle,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
/*++

Routine Description:
  Get string specified by StringId form the HiiHandle.

Arguments:
  HiiHandle     - The HII handle of package list.
  StringId      - The String ID.
  String        - The output string.

Returns:
  EFI_NOT_FOUND         - String is not found.
  EFI_SUCCESS           - Operation is successful.
  EFI_OUT_OF_RESOURCES  - There is not enought memory in the system.
  EFI_INVALID_PARAMETER - The String is NULL.

--*/
;

EFI_STATUS
GetStringFromToken (
  IN  EFI_GUID                        *ProducerGuid,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      *String
  )
/*++

Routine Description:
  Get the string given the StringId and String package Producer's Guid.

Arguments:
  ProducerGuid  - The Guid of String package list.
  StringId      - The String ID.
  String        - The output string.

Returns:
  EFI_NOT_FOUND         - String is not found.
  EFI_SUCCESS           - Operation is successful.
  EFI_OUT_OF_RESOURCES  - There is not enought memory in the system.

--*/
;

EFI_STATUS
IfrLibNewString (
  IN  EFI_HII_HANDLE                  PackageList,
  OUT EFI_STRING_ID                   *StringId,
  IN  CONST EFI_STRING                String
  )
/*++

  Routine Description:
    This function adds the string into String Package of each language.

  Arguments:
    PackageList       - Handle of the package list where this string will be added.
    StringId          - On return, contains the new strings id, which is unique within PackageList.
    String            - Points to the new null-terminated string.

  Returns:
    EFI_SUCCESS            - The new string was added successfully.
    EFI_NOT_FOUND          - The specified PackageList could not be found in database.
    EFI_OUT_OF_RESOURCES   - Could not add the string due to lack of resources.
    EFI_INVALID_PARAMETER  - String is NULL or StringId is NULL is NULL.

--*/
;

EFI_STATUS
IfrLibGetString (
  IN  EFI_HII_HANDLE                  PackageList,
  IN  EFI_STRING_ID                   StringId,
  OUT EFI_STRING                      String,
  IN  OUT UINTN                       *StringSize
  )
/*++

  Routine Description:
    This function try to retrieve string from String package of current language.
    If fail, it try to retrieve string from String package of first language it support.

  Arguments:
    PackageList       - The package list in the HII database to search for the specified string.
    StringId          - The string's id, which is unique within PackageList.
    String            - Points to the new null-terminated string.
    StringSize        - On entry, points to the size of the buffer pointed to by String, in bytes. On return,
                        points to the length of the string, in bytes.

  Returns:
    EFI_SUCCESS            - The string was returned successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not available.
    EFI_BUFFER_TOO_SMALL   - The buffer specified by StringLength is too small to hold the string.
    EFI_INVALID_PARAMETER  - The String or StringSize was NULL.

--*/
;

EFI_STATUS
IfrLibSetString (
  IN EFI_HII_HANDLE                   PackageList,
  IN EFI_STRING_ID                    StringId,
  IN CONST EFI_STRING                 String
  )
/*++

  Routine Description:
    This function updates the string in String package of current language.

  Arguments:
    PackageList       - The package list containing the strings.
    StringId          - The string's id, which is unique within PackageList.
    String            - Points to the new null-terminated string.

  Returns:
    EFI_SUCCESS            - The string was updated successfully.
    EFI_NOT_FOUND          - The string specified by StringId is not in the database.
    EFI_INVALID_PARAMETER  - The String was NULL.
    EFI_OUT_OF_RESOURCES   - The system is out of resources to accomplish the task.

--*/
;

EFI_STATUS
IfrLibCreatePopUp (
  IN  UINTN                       NumberOfLines,
  OUT EFI_INPUT_KEY               *KeyValue,
  IN  CHAR16                      *String,
  ...
  )
/*++

Routine Description:
  Draw a dialog and return the selected key.

Arguments:
  NumberOfLines     - The number of lines for the dialog box
  KeyValue          - The EFI_KEY value returned if HotKey is TRUE..
  String            - Pointer to the first string in the list
  ...               - A series of (quantity == NumberOfLines) text strings which
                      will be used to construct the dialog box

Returns:
  EFI_SUCCESS           - Displayed dialog and received user interaction
  EFI_INVALID_PARAMETER - One of the parameters was invalid.

--*/
;

EFI_STATUS
IfrLibUpdateForm (
  IN EFI_HII_HANDLE            Handle,
  IN EFI_GUID                  *FormSetGuid, OPTIONAL
  IN EFI_FORM_ID               FormId,
  IN UINT16                    Label,
  IN BOOLEAN                   Insert,
  IN EFI_HII_UPDATE_DATA       *Data
  )
/*++

Routine Description:
  This function allows the caller to update a form that has
  previously been registered with the EFI HII database.

Arguments:
  Handle       - Hii Handle
  FormSetGuid  - The formset should be updated.
  FormId       - The form should be updated.
  Label        - Update information starting immediately after this label in the IFR
  Insert       - If TRUE and Data is not NULL, insert data after Label.
                 If FALSE, replace opcodes between two labels with Data.
  Data         - The adding data; If NULL, remove opcodes between two Label.

Returns:
  EFI_SUCCESS  - Update success.
  Other        - Update fail.

--*/
;
#endif
