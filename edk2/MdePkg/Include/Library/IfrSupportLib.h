/** @file
  The file contain all library functions and definitions for IFR opcode creation and 
  related Form Browser utility Operations.

  Copyright (c) 2007 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IFR_SUPPORT_LIBRARY_H
#define _IFR_SUPPORT_LIBRARY_H


#include <Protocol/HiiFont.h>
#include <Protocol/HiiImage.h>
#include <Protocol/HiiString.h>
#include <Protocol/HiiDatabase.h>
#include <Protocol/HiiConfigRouting.h>
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/FormBrowser2.h>
#include <Protocol/SimpleTextOut.h>

#include <Guid/GlobalVariable.h>

//
// The architectural variable "Lang" and "LangCodes" are deprecated in UEFI
// specification. While, UEFI specification also states that these deprecated
// variables may be provided for backwards compatibility.

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


/**
  Create EFI_IFR_END_OP opcode.

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateEndOpCode (
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
;

/**
  Create EFI_IFR_DEFAULT_OP opcode.

  @param  Value                  Value for the default
  @param  Type                   Type for the default
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER The type is not valid.

**/
EFI_STATUS
EFIAPI
CreateDefaultOpCode (
  IN     EFI_IFR_TYPE_VALUE  *Value,
  IN     UINT8               Type,
  IN OUT EFI_HII_UPDATE_DATA *Data
  )
;

/**
  Create EFI_IFR_ACTION_OP opcode.

  @param  QuestionId             Question ID
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  QuestionConfig         String ID for configuration
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateActionOpCode (
  IN     EFI_QUESTION_ID      QuestionId,
  IN     EFI_STRING_ID        Prompt,
  IN     EFI_STRING_ID        Help,
  IN     UINT8                QuestionFlags,
  IN     EFI_STRING_ID        QuestionConfig,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
;

/**
  Create EFI_IFR_SUBTITLE_OP opcode.

  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  Flags                  Subtitle opcode flags
  @param  Scope                  Subtitle Scope bit
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  
**/
EFI_STATUS
EFIAPI
CreateSubTitleOpCode (
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               Flags,
  IN      UINT8               Scope,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
;

/**
  Create EFI_IFR_TEXT_OP opcode.

  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  TextTwo                String ID for text two
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateTextOpCode (
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      EFI_STRING_ID       TextTwo,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  )
;

/**
  Create EFI_IFR_REF_OP opcode.

  @param  FormId                 Destination Form ID
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  QuestionId             Question ID
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateGotoOpCode (
  IN      EFI_FORM_ID         FormId,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      EFI_QUESTION_ID     QuestionId,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  )
;

/**
  Create EFI_IFR_ONE_OF_OPTION_OP opcode.

  @param  OptionCount            The number of options.
  @param  OptionsList            The list of Options.
  @param  Type                   The data type.
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.

**/
EFI_STATUS
EFIAPI
CreateOneOfOptionOpCode (
  IN     UINTN                OptionCount,
  IN     IFR_OPTION           *OptionsList,
  IN     UINT8                Type,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
;

/**
  Create EFI_IFR_ONE_OF_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  OneOfFlags             Flags for oneof opcode
  @param  OptionsList            List of options
  @param  OptionCount            Number of options in option list
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
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
;

/**
  Create EFI_IFR_ORDERED_LIST_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  OrderedListFlags       Flags for ordered list opcode
  @param  DataType               Type for option value
  @param  MaxContainers          Maximum count for options in this ordered list
  @param  OptionsList            List of options
  @param  OptionCount            Number of options in option list
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
CreateOrderedListOpCode (
  IN      EFI_QUESTION_ID     QuestionId,
  IN      EFI_VARSTORE_ID     VarStoreId,
  IN      UINT16              VarOffset,
  IN      EFI_STRING_ID       Prompt,
  IN      EFI_STRING_ID       Help,
  IN      UINT8               QuestionFlags,
  IN      UINT8               OrderedListFlags,
  IN      UINT8               DataType,
  IN      UINT8               MaxContainers,
  IN      IFR_OPTION          *OptionsList,
  IN     UINTN                OptionCount,
  IN OUT EFI_HII_UPDATE_DATA  *Data
  )
;

/**
  Create EFI_IFR_CHECKBOX_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  CheckBoxFlags          Flags for checkbox opcode
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
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
;

/**
  Create EFI_IFR_NUMERIC_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  NumericFlags           Flags for numeric opcode
  @param  Minimum                Numeric minimum value
  @param  Maximum                Numeric maximum value
  @param  Step                   Numeric step for edit
  @param  Default                Numeric default value
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
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
;

/**
  Create EFI_IFR_STRING_OP opcode.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  StringFlags            Flags for string opcode
  @param  MinSize                String minimum length
  @param  MaxSize                String maximum length
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode create success
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If QuestionFlags is not valid.

**/
EFI_STATUS
EFIAPI
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
;

/**
  Converts the unicode character of the string from uppercase to lowercase.

  @param Str     String to be converted

  @retval VOID
  
**/
VOID
ToLower (
  IN OUT CHAR16    *Str
  )
;

/**
  Converts binary buffer to Unicode string in reversed byte order to BufToHexString().

  @param  Str                    String for output
  @param  Buffer                 Binary buffer.
  @param  BufferSize             Size of the buffer in bytes.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
EFIAPI
BufferToHexString (
  IN OUT CHAR16    *Str,
  IN UINT8         *Buffer,
  IN UINTN         BufferSize
  )
;

/**
  Converts Hex String to binary buffer in reversed byte order to HexStringToBuf().

  @param  Buffer                 Pointer to buffer that receives the data.
  @param  BufferSize             Length in bytes of the buffer to hold converted
                                 data. If routine return with EFI_SUCCESS,
                                 containing length of converted data. If routine
                                 return with EFI_BUFFER_TOO_SMALL, containg length
                                 of buffer desired.
  @param  Str                    String to be converted from.

  @retval EFI_SUCCESS            The function completed successfully.

**/
EFI_STATUS
EFIAPI
HexStringToBuffer (
  IN OUT UINT8         *Buffer,
  IN OUT UINTN         *BufferSize,
  IN CHAR16            *Str
  )
;

/**
  Convert binary representation Config string (e.g. "0041004200430044") to the
  original string (e.g. "ABCD"). Config string appears in <ConfigHdr> (i.e.
  "&NAME=<string>"), or Name/Value pair in <ConfigBody> (i.e. "label=<string>").

  @param UnicodeString  Original Unicode string.
  @param StrBufferLen   On input: Length in bytes of buffer to hold the Unicode string.
                                    Includes tailing '\0' character.
                                    On output:
                                      If return EFI_SUCCESS, containing length of Unicode string buffer.
                                      If return EFI_BUFFER_TOO_SMALL, containg length of string buffer desired.
  @param ConfigString   Binary representation of Unicode String, <string> := (<HexCh>4)+

  @retval EFI_SUCCESS          Routine success.
  @retval EFI_BUFFER_TOO_SMALL The string buffer is too small.

**/EFI_STATUS
ConfigStringToUnicode (
  IN OUT CHAR16                *UnicodeString,
  IN OUT UINTN                 *StrBufferLen,
  IN CHAR16                    *ConfigString
  )
;

/**
  Convert Unicode string to binary representation Config string, e.g.
  "ABCD" => "0041004200430044". Config string appears in <ConfigHdr> (i.e.
  "&NAME=<string>"), or Name/Value pair in <ConfigBody> (i.e. "label=<string>").

  @param ConfigString   Binary representation of Unicode String, <string> := (<HexCh>4)+
  @param  StrBufferLen  On input: Length in bytes of buffer to hold the Unicode string.
                                    Includes tailing '\0' character.
                                    On output:
                                      If return EFI_SUCCESS, containing length of Unicode string buffer.
                                      If return EFI_BUFFER_TOO_SMALL, containg length of string buffer desired.
  @param  UnicodeString  Original Unicode string.

  @retval EFI_SUCCESS           Routine success.
  @retval EFI_BUFFER_TOO_SMALL  The string buffer is too small.

**/EFI_STATUS
UnicodeToConfigString (
  IN OUT CHAR16                *ConfigString,
  IN OUT UINTN                 *StrBufferLen,
  IN CHAR16                    *UnicodeString
  )
;

/**
  Construct <ConfigHdr> using routing information GUID/NAME/PATH.

  @param  ConfigHdr              Pointer to the ConfigHdr string.
  @param  StrBufferLen           On input: Length in bytes of buffer to hold the
                                 ConfigHdr string. Includes tailing '\0' character.
                                 On output: If return EFI_SUCCESS, containing
                                 length of ConfigHdr string buffer. If return
                                 EFI_BUFFER_TOO_SMALL, containg length of string
                                 buffer desired.
  @param  Guid                   Routing information: GUID.
  @param  Name                   Routing information: NAME.
  @param  DriverHandle           Driver handle which contains the routing
                                 information: PATH.

  @retval EFI_SUCCESS            Routine success.
  @retval EFI_BUFFER_TOO_SMALL   The ConfigHdr string buffer is too small.

**/
EFI_STATUS
EFIAPI
ConstructConfigHdr (
  IN OUT CHAR16                *ConfigHdr,
  IN OUT UINTN                 *StrBufferLen,
  IN EFI_GUID                  *Guid,
  IN CHAR16                    *Name, OPTIONAL
  IN EFI_HANDLE                *DriverHandle
  )

;

/**
  Search BlockName "&OFFSET=Offset&WIDTH=Width" in a string.

  @param  String                 The string to be searched in.
  @param  Offset                 Offset in BlockName.
  @param  Width                  Width in BlockName.

  @retval TRUE                   Block name found.
  @retval FALSE                  Block name not found.

**/
BOOLEAN
EFIAPI
FindBlockName (
  IN OUT CHAR16                *String,
  UINTN                        Offset,
  UINTN                        Width
  )
;

/**
  This routine is invoked by ConfigAccess.Callback() to retrived uncommitted data from Form Browser.

  @param  VariableGuid           An optional field to indicate the target variable
                                 GUID name to use.
  @param  VariableName           An optional field to indicate the target
                                 human-readable variable name.
  @param  BufferSize             On input: Length in bytes of buffer to hold
                                 retrived data. On output: If return
                                 EFI_BUFFER_TOO_SMALL, containg length of buffer
                                 desired.
  @param  Buffer                 Buffer to hold retrived data.

  @retval EFI_SUCCESS            Routine success.
  @retval EFI_BUFFER_TOO_SMALL   The intput buffer is too small.

**/
EFI_STATUS
EFIAPI
GetBrowserData (
  EFI_GUID                   *VariableGuid, OPTIONAL
  CHAR16                     *VariableName, OPTIONAL
  UINTN                      *BufferSize,
  UINT8                      *Buffer
  )
;

/**
  This routine is invoked by ConfigAccess.Callback() to update uncommitted data of Form Browser.

  @param  VariableGuid           An optional field to indicate the target variable
                                 GUID name to use.
  @param  VariableName           An optional field to indicate the target
                                 human-readable variable name.
  @param  BufferSize             Length in bytes of buffer to hold retrived data.
  @param  Buffer                 Buffer to hold retrived data.
  @param  RequestElement         An optional field to specify which part of the
                                 buffer data will be send back to Browser. If NULL,
                                 the whole buffer of data will be committed to
                                 Browser. <RequestElement> ::=
                                 &OFFSET=<Number>&WIDTH=<Number>*

  @retval EFI_SUCCESS            Routine success.
  @retval Other                  Updating Browser uncommitted data failed.

**/
EFI_STATUS
EFIAPI
SetBrowserData (
  EFI_GUID                   *VariableGuid, OPTIONAL
  CHAR16                     *VariableName, OPTIONAL
  UINTN                      BufferSize,
  UINT8                      *Buffer,
  CHAR16                     *RequestElement  OPTIONAL
  )
;

/**
  Draw a dialog and return the selected key.

  @param  NumberOfLines          The number of lines for the dialog box
  @param  KeyValue               The EFI_KEY value returned if HotKey is TRUE..
  @param  String                 Pointer to the first string in the list
  @param  ...                    A series of (quantity == NumberOfLines) text
                                 strings which will be used to construct the dialog
                                 box

  @retval EFI_SUCCESS            Displayed dialog and received user interaction
  @retval EFI_INVALID_PARAMETER  One of the parameters was invalid.

**/

EFI_STATUS
EFIAPI
IfrLibCreatePopUp (
  IN  UINTN                       NumberOfLines,
  OUT EFI_INPUT_KEY               *KeyValue,
  IN  CHAR16                      *String,
  ...
  )
;

#endif
