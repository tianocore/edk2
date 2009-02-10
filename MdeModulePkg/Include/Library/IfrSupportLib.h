/** @file
  This library contains functions to do IFR opcode creation and utility functions 
  to help module to interact with a UEFI Form Browser.

  Copyright (c) 2007 - 2008, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _IFR_SUPPORT_LIBRARY_H_
#define _IFR_SUPPORT_LIBRARY_H_

#pragma pack(1)
typedef struct {
  EFI_STRING_ID       StringToken;
  EFI_IFR_TYPE_VALUE  Value;
  UINT8               Flags;
} IFR_OPTION;
#pragma pack()

typedef struct {
  ///
  /// Buffer size allocated for Data.
  ///
  UINT32                BufferSize;

  ///
  /// Offset in Data to append the newly created opcode binary.
  /// It will be adjusted automatically in Create***OpCode(), and should be
  /// initialized to 0 before invocation of a serial of Create***OpCode()
  ///
  UINT32                Offset;

  ///
  /// The destination buffer for created op-codes
  ///
  UINT8                 *Data;
} EFI_HII_UPDATE_DATA;


/**
  Create EFI_IFR_END_OP opcode.

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  Value                  Value for the default
  @param  Type                   Type for the default
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  QuestionId             Question ID
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  QuestionConfig         String ID for configuration
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  Flags                  Subtitle opcode flags
  @param  Scope                  Subtitle Scope bit
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  TextTwo                String ID for text two
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  FormId                 Destination Form ID
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  QuestionId             Question ID
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  OptionCount            The number of options.
  @param  OptionsList            The list of Options.
  @param  Type                   The data type.
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
  @retval EFI_BUFFER_TOO_SMALL The space reserved in Data field is too small.
  @retval EFI_INVALID_PARAMETER If OptionCount is not zero but OptionsList is NULL.

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

  If Data is NULL or Data->Data is NULL, then ASSERT.

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

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

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

  @retval EFI_SUCCESS            Opcode is created successfully.
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
  IN      UINTN               OptionCount,
  IN OUT  EFI_HII_UPDATE_DATA *Data
  )
;

/**
  Create EFI_IFR_CHECKBOX_OP opcode.

  If Data is NULL or Data->Data is NULL, then ASSERT.

  @param  QuestionId             Question ID
  @param  VarStoreId             Storage ID
  @param  VarOffset              Offset in Storage
  @param  Prompt                 String ID for Prompt
  @param  Help                   String ID for Help
  @param  QuestionFlags          Flags in Question Header
  @param  CheckBoxFlags          Flags for checkbox opcode
  @param  Data                   Destination for the created opcode binary

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

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

  @retval EFI_SUCCESS            Opcode is created successfully.
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

  If Data is NULL or Data->Data is NULL, then ASSERT.

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

  @retval EFI_SUCCESS            Opcode is created successfully.
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
    Construct <ConfigAltResp> for a buffer storage.

    @param ConfigRequest           The Config request string. If set to NULL, all the
                                   configurable elements will be extracted from BlockNameArray.
    @param Progress                On return, points to a character in the Request.
    @param ConfigAltResp           The returned <ConfigAltResp>.
    @param Guid                    GUID of the buffer storage.
    @param Name                    Name of the buffer storage.
    @param DriverHandle            The DriverHandle which is used to invoke HiiDatabase
                                   protocol interface NewPackageList().
    @param BufferStorage           Content of the buffer storage.
    @param BufferStorageSize       Length in bytes of the buffer storage.
    @param BlockNameArray          Array generated by VFR compiler.
    @param NumberAltCfg            Number of Default value array generated by VFR compiler.
                                   The sequential input parameters will be number of
                                   AltCfgId and DefaultValueArray pairs. When set to 0,
                                   there will be no <AltResp>.
    @param  ...                    Variable argument list.                   
    
    retval EFI_OUT_OF_RESOURCES  Run out of memory resource.
    retval EFI_INVALID_PARAMETER ConfigAltResp is NULL.
    retval EFI_SUCCESS           Operation successful.

**/
EFI_STATUS
ConstructConfigAltResp (
  IN  EFI_STRING                  ConfigRequest,  OPTIONAL
  OUT EFI_STRING                  *Progress,
  OUT EFI_STRING                  *ConfigAltResp,
  IN  EFI_GUID                    *Guid,
  IN  CHAR16                      *Name,
  IN  EFI_HANDLE                  *DriverHandle,
  IN  VOID                        *BufferStorage,
  IN  UINTN                       BufferStorageSize,
  IN  VOID                        *BlockNameArray, OPTIONAL
  IN  UINTN                       NumberAltCfg,
  ...
//IN  UINT16                      AltCfgId,
//IN  VOID                        *DefaultValueArray,
  )
;

/**
  Converts the unicode character of the string from uppercase to lowercase.

  @param Str     String to be converted

 
**/
VOID
EFIAPI
ToLower (
  IN OUT CHAR16    *Str
  )
;

/**
  Converts binary buffer to a Unicode string. The byte buffer is in a reversed byte order 
  compared with the byte order defined in BufToHexString().

  @param  Str                    String for output
  @param  Buffer                 Binary buffer.
  @param  BufferSize             Size of the buffer in bytes.

  @retval EFI_SUCCESS            The function completed successfully.
  @retval EFI_OUT_OF_RESOURCES   There is no enough available memory space.

**/
EFI_STATUS
EFIAPI
BufInReverseOrderToHexString (
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
  @retval RETURN_BUFFER_TOO_SMALL   The input BufferSize is too small to hold the output. BufferSize
                                   will be updated to the size required for the converstion.

**/
EFI_STATUS
EFIAPI
HexStringToBufInReverseOrder (
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
                                      containing length of Unicode string buffer when returning EFI_SUCCESS;
                                      containg length of string buffer desired when returning EFI_BUFFER_TOO_SMALL.
  @param ConfigString   Binary representation of Unicode String, <string> := (<HexCh>4)+

  @retval EFI_SUCCESS          Operation completes successfully.
  @retval EFI_BUFFER_TOO_SMALL The string buffer is too small.

**/
EFI_STATUS
EFIAPI
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

  @retval EFI_SUCCESS           Operation completes successfully.
  @retval EFI_BUFFER_TOO_SMALL  The string buffer is too small.

**/
EFI_STATUS
EFIAPI
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

  @retval EFI_SUCCESS            Operation completes successfully.
  @retval EFI_BUFFER_TOO_SMALL   The ConfigHdr string buffer is too small.

**/
EFI_STATUS
EFIAPI
ConstructConfigHdr (
  IN OUT CHAR16                *ConfigHdr,
  IN OUT UINTN                 *StrBufferLen,
  IN CONST EFI_GUID            *Guid,
  IN CHAR16                    *Name, OPTIONAL
  IN EFI_HANDLE                *DriverHandle
  )

;

/**
  Determines if the Routing data (Guid and Name) is correct in <ConfigHdr>.

  @param ConfigString  Either <ConfigRequest> or <ConfigResp>.
  @param StorageGuid   GUID of the storage.
  @param StorageName   Name of the stoarge.

  @retval TRUE         Routing information is correct in ConfigString.
  @retval FALSE        Routing information is incorrect in ConfigString.

**/
BOOLEAN
EFIAPI
IsConfigHdrMatch (
  IN EFI_STRING                ConfigString,
  IN EFI_GUID                  *StorageGuid, OPTIONAL
  IN CHAR16                    *StorageName  OPTIONAL
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
  IN UINTN                     Offset,
  IN UINTN                     Width
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

  @retval EFI_SUCCESS            Operation completes successfully.
  @retval EFI_BUFFER_TOO_SMALL   The intput buffer is too small.

**/
EFI_STATUS
EFIAPI
GetBrowserData (
  IN CONST EFI_GUID          *VariableGuid, OPTIONAL
  IN CONST CHAR16            *VariableName, OPTIONAL
  IN OUT UINTN               *BufferSize,
  IN OUT UINT8               *Buffer
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

  @retval EFI_SUCCESS            Operation completes successfully.
  @retval Other                  Updating Browser uncommitted data failed.

**/
EFI_STATUS
EFIAPI
SetBrowserData (
  IN CONST EFI_GUID          *VariableGuid, OPTIONAL
  IN CONST CHAR16            *VariableName, OPTIONAL
  IN UINTN                   BufferSize,
  IN CONST UINT8             *Buffer,
  IN CONST CHAR16            *RequestElement  OPTIONAL
  )
;

/**
  Draw a dialog and return the selected key.

  @param  NumberOfLines          The number of lines for the dialog box
  @param  KeyValue               The EFI_KEY value returned if HotKey is TRUE..
  @param  String                 Pointer to the first string in the list
  @param  ...                    A series of (quantity == NumberOfLines - 1) text
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

/**
  Draw a dialog and return the selected key using Variable Argument List.

  @param  NumberOfLines          The number of lines for the dialog box
  @param  KeyValue               The EFI_KEY value returned if HotKey is TRUE..
  @param  String                 The first String to be displayed in the Pop-Up.
  @param  Args                   VA_LIST marker for the variable argument list.
                                 A series of (quantity == NumberOfLines - 1) text
                                 strings which will be used to construct the dialog
                                 box

  @retval EFI_SUCCESS            Displayed dialog and received user interaction
  @retval EFI_INVALID_PARAMETER  One of the parameters was invalid.

**/
EFI_STATUS
EFIAPI
IfrLibCreatePopUp2 (
  IN  UINTN                       NumberOfLines,
  OUT EFI_INPUT_KEY               *KeyValue,
  IN  CHAR16                      *String,
  IN  VA_LIST                     Args
  )
;

/**
  Test if  a Unicode character is a hexadecimal digit. If true, the input
  Unicode character is converted to a byte. 

  This function tests if a Unicode character is a hexadecimal digit. If true, the input
  Unicode character is converted to a byte. For example, Unicode character
  L'A' will be converted to 0x0A. 

  If Digit is NULL, then ASSERT.

  @param  Digit       The output hexadecimal digit.

  @param  Char        The input Unicode character.

  @retval TRUE        Char is in the range of Hexadecimal number. Digit is updated
                      to the byte value of the number.
  @retval FALSE       Char is not in the range of Hexadecimal number. Digit is keep
                      intact.

**/
BOOLEAN
EFIAPI
IsHexDigit (
  OUT UINT8      *Digit,
  IN  CHAR16      Char
  )
;


/** 
  Convert binary buffer to a Unicode String in a specified sequence. 

  This function converts bytes in the memory block pointed by Buffer to a Unicode String Str. 
  Each byte will be represented by two Unicode characters. For example, byte 0xA1 will 
  be converted into two Unicode character L'A' and L'1'. In the output String, the Unicode Character 
  for the Most Significant Nibble will be put before the Unicode Character for the Least Significant
  Nibble. The output string for the buffer containing a single byte 0xA1 will be L"A1". 
  For a buffer with multiple bytes, the Unicode character produced by the first byte will be put into the 
  the last character in the output string. The one next to first byte will be put into the
  character before the last character. This rules applies to the rest of the bytes. The Unicode
  character by the last byte will be put into the first character in the output string. For example,
  the input buffer for a 64-bits unsigned integer 0x12345678abcdef1234 will be converted to
  a Unicode string equal to L"12345678abcdef1234".

  @param String                        On input, String is pointed to the buffer allocated for the convertion.
  @param StringLen                     The Length of String buffer to hold the output String. The length must include the tailing '\0' character.
                                       The StringLen required to convert a N bytes Buffer will be a least equal to or greater 
                                       than 2*N + 1.
  @param Buffer                        The pointer to a input buffer.
  @param BufferSizeInBytes             Length in bytes of the input buffer.
  

  @retval  EFI_SUCCESS                 The convertion is successful. All bytes in Buffer has been convert to the corresponding
                                       Unicode character and placed into the right place in String.
  @retval  EFI_BUFFER_TOO_SMALL        StringSizeInBytes is smaller than 2 * N + 1the number of bytes required to
                                       complete the convertion. 
**/
RETURN_STATUS
EFIAPI
BufToHexString (
  IN OUT       CHAR16               *String,
  IN OUT       UINTN                *StringLen,
  IN     CONST UINT8                *Buffer,
  IN           UINTN                BufferSizeInBytes
  )
;


/**
  Convert a Unicode string consisting of hexadecimal characters to a output byte buffer.

  This function converts a Unicode string consisting of characters in the range of Hexadecimal
  character (L'0' to L'9', L'A' to L'F' and L'a' to L'f') to a output byte buffer. The function will stop
  at the first non-hexadecimal character or the NULL character. The convertion process can be
  simply viewed as the reverse operations defined by BufToHexString. Two Unicode characters will be 
  converted into one byte. The first Unicode character represents the Most Significant Nibble and the
  second Unicode character represents the Least Significant Nibble in the output byte. 
  The first pair of Unicode characters represents the last byte in the output buffer. The second pair of Unicode 
  characters represent the  the byte preceding the last byte. This rule applies to the rest pairs of bytes. 
  The last pair represent the first byte in the output buffer. 

  For example, a Unciode String L"12345678" will be converted into a buffer wil the following bytes 
  (first byte is the byte in the lowest memory address): "0x78, 0x56, 0x34, 0x12".

  If String has N valid hexadecimal characters for conversion,  the caller must make sure Buffer is at least 
  N/2 (if N is even) or (N+1)/2 (if N if odd) bytes. 

  If either Buffer, BufferSizeInBytes or String is NULL, then ASSERT ().

  @param Buffer                      The output buffer allocated by the caller.
  @param BufferSizeInBytes           On input, the size in bytes of Buffer. On output, it is updated to 
                                     contain the size of the Buffer which is actually used for the converstion.
                                     For Unicode string with 2*N hexadecimal characters (not including the 
                                     tailing NULL character), N bytes of Buffer will be used for the output.
  @param String                      The input hexadecimal string.
  @param ConvertedStrLen             The number of hexadecimal characters used to produce content in output
                                     buffer Buffer.

  @retval  RETURN_BUFFER_TOO_SMALL   The input BufferSizeInBytes is too small to hold the output. BufferSizeInBytes
                                     will be updated to the size required for the converstion.
  @retval  RETURN_SUCCESS            The convertion is successful or the first Unicode character from String
                                     is hexadecimal. If ConvertedStrLen is not NULL, it is updated
                                     to the number of hexadecimal character used for the converstion.
**/
RETURN_STATUS
EFIAPI
HexStringToBuf (
  OUT          UINT8                    *Buffer,   
  IN OUT       UINTN                    *BufferSizeInBytes,
  IN     CONST CHAR16                   *String,
  OUT          UINTN                    *ConvertedStrLen  OPTIONAL
  )
;

#endif
