/** @file
Implementation of interfaces function for EFI_HII_CONFIG_ROUTING_PROTOCOL.

Copyright (c) 2007 - 2008, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "HiiDatabase.h"
extern HII_DATABASE_PRIVATE_DATA mPrivate;

/**
  Calculate the number of Unicode characters of the incoming Configuration string,
  not including NULL terminator.

  This is a internal function.

  @param  String                 String in <MultiConfigRequest> or
                                 <MultiConfigResp> format.

  @return The number of Unicode characters.

**/
UINTN
CalculateConfigStringLen (
  IN EFI_STRING                    String
  )
{
  EFI_STRING  TmpPtr;

  //
  // "GUID=" should be the first element of incoming string.
  //
  ASSERT (String != NULL);
  ASSERT (StrnCmp (String, L"GUID=", StrLen (L"GUID=")) == 0);

  //
  // The beginning of next <ConfigRequest>/<ConfigResp> should be "&GUID=".
  // Will meet '\0' if there is only one <ConfigRequest>/<ConfigResp>.
  // 
  TmpPtr = StrStr (String, L"&GUID=");
  if (TmpPtr == NULL) {
    return StrLen (String);
  }

  return (TmpPtr - String);
}


/**
  Convert the hex UNICODE %02x encoding of a UEFI device path to binary
  from <PathHdr> of <ConfigHdr>.

  This is a internal function.

  @param  String                 UEFI configuration string
  @param  DevicePath             binary of a UEFI device path.

  @retval EFI_INVALID_PARAMETER  Any incoming parameter is invalid.
  @retval EFI_OUT_OF_RESOURCES   Lake of resources to store neccesary structures.
  @retval EFI_SUCCESS            The device path is retrieved and translated to
                                 binary format.

**/
EFI_STATUS
GetDevicePath (
  IN  EFI_STRING                   String,
  OUT UINT8                        **DevicePath
  )
{
  UINTN      Length;
  EFI_STRING PathHdr;
  EFI_STRING DevicePathString;
  UINT8      *DevicePathBuffer;
  CHAR16     TemStr[2];
  UINTN      Index;
  UINT8      DigitUint8;

  if (String == NULL || DevicePath == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Find the 'PATH=' of <PathHdr> and skip it.
  //
  for (; (*String != 0 && StrnCmp (String, L"PATH=", StrLen (L"PATH=")) != 0); String++);
  if (*String == 0) {
    return EFI_INVALID_PARAMETER;
  }

  String += StrLen (L"PATH=");
  PathHdr = String;

  //
  // The content between 'PATH=' of <ConfigHdr> and '&' of next element
  // or '\0' (end of configuration string) is the UNICODE %02x bytes encoding
  // of UEFI device path.
  //
  for (Length = 0; *String != 0 && *String != L'&'; String++, Length++);
  DevicePathString = (EFI_STRING) AllocateZeroPool ((Length + 1) * sizeof (CHAR16));
  if (DevicePathString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  StrnCpy (DevicePathString, PathHdr, Length);
  *(DevicePathString + Length) = 0;

  //
  // The data in <PathHdr> is encoded as hex UNICODE %02x bytes in the same order
  // as the device path resides in RAM memory.
  // Translate the data into binary.
  //
  DevicePathBuffer = (UINT8 *) AllocateZeroPool ((Length + 1) / 2);
  if (DevicePathBuffer == NULL) {
    FreePool (DevicePathString);
    return EFI_OUT_OF_RESOURCES;
  }

  ZeroMem (TemStr, sizeof (TemStr));
  for (Index = 0; DevicePathString[Index] != L'\0'; Index ++) {
    TemStr[0] = DevicePathString[Index];
    DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
    if ((Index & 1) == 0) {
      DevicePathBuffer [Index/2] = DigitUint8;
    } else {
      DevicePathBuffer [Index/2] = (UINT8) ((DevicePathBuffer [Index/2] << 4) + DigitUint8);
    }
  }

  FreePool (DevicePathString);
  
  *DevicePath = DevicePathBuffer;

  return EFI_SUCCESS;

}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param Str     String to be converted

**/
VOID
EFIAPI
HiiToLower (
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
    } else if (Lower && *String >= L'A' && *String <= L'F') {
      *String = (CHAR16) (*String - L'A' + L'a');
    }
  }

  return;
}

/**
  Generate a sub string then output it.

  This is a internal function.

  @param  String                 A constant string which is the prefix of the to be
                                 generated string, e.g. GUID=

  @param  BufferLen              The length of the Buffer in bytes.

  @param  Buffer                 Points to a buffer which will be converted to be the 
                                 content of the generated string.

  @param  Flag                   If 1, the buffer contains data for the value of GUID or PATH stored in 
                                 UINT8 *; if 2, the buffer contains unicode string for the value of NAME;
                                 if 3, the buffer contains other data.

  @param  SubStr                 Points to the output string. It's caller's
                                 responsibility to free this buffer.


**/
VOID
GenerateSubStr (
  IN CONST EFI_STRING              String,
  IN  UINTN                        BufferLen,
  IN  VOID                         *Buffer,
  IN  UINT8                        Flag,
  OUT EFI_STRING                   *SubStr
  )
{
  UINTN       Length;
  EFI_STRING  Str;
  EFI_STRING  StringHeader;
  CHAR16      *TemString;
  CHAR16      *TemName;
  UINT8       *TemBuffer;
  UINTN       Index;

  ASSERT (String != NULL && SubStr != NULL);

  if (Buffer == NULL) {
    *SubStr = AllocateCopyPool (StrSize (String), String);
    ASSERT (*SubStr != NULL);
    return ;
  }
  
  //
  // Header + Data + '&' + '\0'
  //
  Length = StrLen (String) + BufferLen * 2 + 1 + 1;
  Str    = AllocateZeroPool (Length * sizeof (CHAR16));
  ASSERT (Str != NULL);

  StrCpy (Str, String);
  Length = (BufferLen * 2 + 1) * sizeof (CHAR16);

  StringHeader = Str + StrLen (String);
  TemString    = (CHAR16 *) StringHeader;

  switch (Flag) {
  case 1:
    //
    // Convert Buffer to Hex String in reverse order
    //
    TemBuffer = ((UINT8 *) Buffer);
    for (Index = 0; Index < BufferLen; Index ++, TemBuffer ++) {
      TemString += UnicodeValueToString (TemString, PREFIX_ZERO | RADIX_HEX, *TemBuffer, 2);
    }
    break;
  case 2:
    //
    // Check buffer is enough
    //
    TemName = (CHAR16 *) Buffer;
    ASSERT ((BufferLen * 2 + 1) >= (StrLen (TemName) * 4 + 1));
    //
    // Convert Unicode String to Config String, e.g. "ABCD" => "0041004200430044"
    //
    for (; *TemName != L'\0'; TemName++) {
      TemString += UnicodeValueToString (TemString, PREFIX_ZERO | RADIX_HEX, *TemName, 4);
    }
    break;
  case 3:
    //
    // Convert Buffer to Hex String
    //
    TemBuffer = ((UINT8 *) Buffer) + BufferLen - 1;
    for (Index = 0; Index < BufferLen; Index ++, TemBuffer --) {
      TemString += UnicodeValueToString (TemString, PREFIX_ZERO | RADIX_HEX, *TemBuffer, 2);
    }
    break;
  default:
    break;
  }

  //
  // Convert the uppercase to lowercase since <HexAf> is defined in lowercase format.
  //
  StrCat (Str, L"&");  
  HiiToLower (Str);

  *SubStr = Str;
}


/**
  Retrieve the <ConfigBody> from String then output it.

  This is a internal function.

  @param  String                 A sub string of a configuration string in
                                 <MultiConfigAltResp> format.
  @param  ConfigBody             Points to the output string. It's caller's
                                 responsibility to free this buffer.

  @retval EFI_INVALID_PARAMETER  There is no form package in current hii database.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to finish this operation.
  @retval EFI_SUCCESS            All existing storage is exported.

**/
EFI_STATUS
OutputConfigBody (
  IN  EFI_STRING                   String,
  OUT EFI_STRING                   *ConfigBody
  )
{
  EFI_STRING  TmpPtr;
  EFI_STRING  Result;
  UINTN       Length;

  if (String == NULL || ConfigBody == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // The setting information should start OFFSET, not ALTCFG.
  //
  if (StrnCmp (String, L"&ALTCFG=", StrLen (L"&ALTCFG=")) == 0) {
    return EFI_INVALID_PARAMETER;
  }

  TmpPtr = StrStr (String, L"GUID=");
  if (TmpPtr == NULL) {
    //
    // It is the last <ConfigResp> of the incoming configuration string.
    //
    Result = AllocateCopyPool (StrSize (String), String);
    if (Result == NULL) {
      return EFI_OUT_OF_RESOURCES;
    } else {
      *ConfigBody = Result;
      return EFI_SUCCESS;
    }
  }

  Length = TmpPtr - String;
  Result = AllocateCopyPool (Length * sizeof (CHAR16), String);
  if (Result == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  *(Result + Length - 1) = 0;
  *ConfigBody = Result;
  return EFI_SUCCESS;
}

/**
  Append a string to a multi-string format.

  This is a internal function.

  @param  MultiString            String in <MultiConfigRequest>,
                                 <MultiConfigAltResp>, or <MultiConfigResp>. On
                                 input, the buffer length of  this string is
                                 MAX_STRING_LENGTH. On output, the  buffer length
                                 might be updated.
  @param  AppendString           NULL-terminated Unicode string.

  @retval EFI_INVALID_PARAMETER  Any incoming parameter is invalid.
  @retval EFI_SUCCESS            AppendString is append to the end of MultiString

**/
EFI_STATUS
AppendToMultiString (
  IN OUT EFI_STRING                *MultiString,
  IN EFI_STRING                    AppendString
  )
{
  UINTN AppendStringSize;
  UINTN MultiStringSize;

  if (MultiString == NULL || *MultiString == NULL || AppendString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  AppendStringSize = StrSize (AppendString);
  MultiStringSize  = StrSize (*MultiString);

  //
  // Enlarge the buffer each time when length exceeds MAX_STRING_LENGTH.
  //
  if (MultiStringSize + AppendStringSize > MAX_STRING_LENGTH ||
      MultiStringSize > MAX_STRING_LENGTH) {
    *MultiString = (EFI_STRING) ReallocatePool (
                                  MultiStringSize,
                                  MultiStringSize + AppendStringSize,
                                  (VOID *) (*MultiString)
                                  );
    ASSERT (*MultiString != NULL);
  }
  //
  // Append the incoming string
  //
  StrCat (*MultiString, AppendString);

  return EFI_SUCCESS;
}


/**
  Get the value of <Number> in <BlockConfig> format, i.e. the value of OFFSET
  or WIDTH or VALUE.
  <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE'=<Number>

  This is a internal function.

  @param  StringPtr              String in <BlockConfig> format and points to the
                                 first character of <Number>.
  @param  Number                 The output value. Caller takes the responsibility
                                 to free memory.
  @param  Len                    Length of the <Number>, in characters.

  @retval EFI_OUT_OF_RESOURCES   Insufficient resources to store neccessary
                                 structures.
  @retval EFI_SUCCESS            Value of <Number> is outputted in Number
                                 successfully.

**/
EFI_STATUS
GetValueOfNumber (
  IN EFI_STRING                    StringPtr,
  OUT UINT8                        **Number,
  OUT UINTN                        *Len
  )
{
  EFI_STRING               TmpPtr;
  UINTN                    Length;
  EFI_STRING               Str;
  UINT8                    *Buf;
  EFI_STATUS               Status;
  UINT8                    DigitUint8;
  UINTN                    Index;
  CHAR16                   TemStr[2];

  ASSERT (StringPtr != NULL && Number != NULL && Len != NULL);
  ASSERT (*StringPtr != L'\0');

  Buf = NULL;

  TmpPtr = StringPtr;
  while (*StringPtr != L'\0' && *StringPtr != L'&') {
    StringPtr++;
  }
  *Len   = StringPtr - TmpPtr;
  Length = *Len + 1;

  Str = (EFI_STRING) AllocateZeroPool (Length * sizeof (CHAR16));
  if (Str == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  CopyMem (Str, TmpPtr, *Len * sizeof (CHAR16));
  *(Str + *Len) = L'\0';

  Length = (Length + 1) / 2;
  Buf = (UINT8 *) AllocateZeroPool (Length);
  if (Buf == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Exit;
  }
  
  Length = *Len;
  ZeroMem (TemStr, sizeof (TemStr));
  for (Index = 0; Index < Length; Index ++) {
    TemStr[0] = Str[Length - Index - 1];
    DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
    if ((Index & 1) == 0) {
      Buf [Index/2] = DigitUint8;
    } else {
      Buf [Index/2] = (UINT8) ((DigitUint8 << 4) + Buf [Index/2]);
    }
  }

  *Number = Buf;
  Status  = EFI_SUCCESS;

Exit:
  if (Str != NULL) {
    FreePool (Str);
  }

  return Status;
}

/**
  This function merges DefaultAltCfgResp string into AltCfgResp string for
  the missing AltCfgId in AltCfgResq.

  @param  AltCfgResp             Pointer to a null-terminated Unicode string in
                                 <ConfigAltResp> format. The default value string 
                                 will be merged into it. 
  @param  DefaultAltCfgResp      Pointer to a null-terminated Unicode string in
                                 <MultiConfigAltResp> format. The default value 
                                 string may contain more than one ConfigAltResp
                                 string for the different varstore buffer.

  @retval EFI_SUCCESS            The merged string returns.
  @retval EFI_INVALID_PARAMETER  *AltCfgResp is to NULL.
**/
EFI_STATUS
EFIAPI
MergeDefaultString (
  IN OUT EFI_STRING  *AltCfgResp,
  IN     EFI_STRING  DefaultAltCfgResp
  )
{
  EFI_STRING   StringPtrDefault;
  EFI_STRING   StringPtrEnd;
  CHAR16       TempChar;
  EFI_STRING   StringPtr;
  EFI_STRING   AltConfigHdr;
  UINTN        HeaderLength;
  UINTN        SizeAltCfgResp;
  
  if (*AltCfgResp == NULL) {
    return EFI_INVALID_PARAMETER;
  }
  
  //
  // Get the requestr ConfigHdr
  //
  SizeAltCfgResp  = 0;
  StringPtr       = *AltCfgResp;
  
  //
  // Find <ConfigHdr> GUID=...&NAME=...&PATH=...
  //
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }
  while (*StringPtr != L'\0' && StrnCmp (StringPtr, L"&NAME=", StrLen (L"&NAME=")) != 0) {
    StringPtr++;
  }
  while (*StringPtr != L'\0' && StrnCmp (StringPtr, L"&PATH=", StrLen (L"&PATH=")) != 0) {
    StringPtr++;
  }
  if (*StringPtr == L'\0') {
    return EFI_INVALID_PARAMETER;
  }
  StringPtr += StrLen (L"&PATH=");
  while (*StringPtr != L'\0' && *StringPtr != L'&') {
    StringPtr ++;
  }
  HeaderLength = StringPtr - *AltCfgResp;

  //
  // Construct AltConfigHdr string  "&<ConfigHdr>&ALTCFG=XXXX\0"
  //                                  |1| StrLen (ConfigHdr) | 8 | 4 | 1 |
  //
  AltConfigHdr = AllocateZeroPool ((1 + HeaderLength + 8 + 4 + 1) * sizeof (CHAR16));
  if (AltConfigHdr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  StrCpy (AltConfigHdr, L"&");
  StrnCat (AltConfigHdr, *AltCfgResp, HeaderLength);
  StrCat (AltConfigHdr, L"&ALTCFG=");
  HeaderLength = StrLen (AltConfigHdr);
  
  StringPtrDefault = StrStr (DefaultAltCfgResp, AltConfigHdr);
  while (StringPtrDefault != NULL) {
    //
    // Get AltCfg Name
    //
    StrnCat (AltConfigHdr, StringPtrDefault + HeaderLength, 4);
    StringPtr = StrStr (*AltCfgResp, AltConfigHdr); 
    
    //
    // Append the found default value string to the input AltCfgResp
    // 
    if (StringPtr == NULL) {
      StringPtrEnd   = StrStr (StringPtrDefault + 1, L"&GUID");
      SizeAltCfgResp = StrSize (*AltCfgResp);
      if (StringPtrEnd == NULL) {
        //
        // No more default string is found.
        //
        *AltCfgResp    = (EFI_STRING) ReallocatePool (
                                     SizeAltCfgResp,
                                     SizeAltCfgResp + StrSize (StringPtrDefault),
                                     (VOID *) (*AltCfgResp)
                                     );
        StrCat (*AltCfgResp, StringPtrDefault);
        break;
      } else {
        TempChar = *StringPtrEnd;
        *StringPtrEnd = L'\0';
        *AltCfgResp = (EFI_STRING) ReallocatePool (
                                     SizeAltCfgResp,
                                     SizeAltCfgResp + StrSize (StringPtrDefault),
                                     (VOID *) (*AltCfgResp)
                                     );
        StrCat (*AltCfgResp, StringPtrDefault);
        *StringPtrEnd = TempChar;
      }
    }
    
    //
    // Find next AltCfg String
    //    
    *(AltConfigHdr + HeaderLength) = L'\0';
    StringPtrDefault = StrStr (StringPtrDefault + 1, AltConfigHdr);    
  }

  return EFI_SUCCESS;  
}

/**
  This function finds the matched DefaultName for the input DefaultId

  @param  DefaultIdArray    Array stores the map table between DefaultId and DefaultName.
  @param  VarDefaultId      Default Id
  @param  VarDefaultName    Default Name string ID for the input default ID.
  
  @retval EFI_SUCCESS       The mapped default name string ID is found.
  @retval EFI_NOT_FOUND     The mapped default name string ID is not found.
**/
EFI_STATUS
FindDefaultName (
  IN  IFR_DEFAULT_DATA *DefaultIdArray, 
  IN  UINT16           VarDefaultId, 
  OUT EFI_STRING_ID    *VarDefaultName
  )
{
  LIST_ENTRY        *Link;
  IFR_DEFAULT_DATA  *DefaultData;

  for (Link = DefaultIdArray->Entry.ForwardLink; Link != &DefaultIdArray->Entry; Link = Link->ForwardLink) {
    DefaultData = BASE_CR (Link, IFR_DEFAULT_DATA, Entry);
    if (DefaultData->DefaultId == VarDefaultId) {
      *VarDefaultName = DefaultData->DefaultName;
      return EFI_SUCCESS;
    }
  }
  
  return EFI_NOT_FOUND;
}

/**
  This function inserts new DefaultValueData into the BlockData DefaultValue array.

  @param  BlockData         The BlockData is updated to add new default value.
  @param  DefaultValueData  The DefaultValue is added.

**/
VOID
InsertDefaultValue (
  IN IFR_BLOCK_DATA         *BlockData,
  IN IFR_DEFAULT_DATA       *DefaultValueData
  )
{
  LIST_ENTRY             *Link;
  IFR_DEFAULT_DATA       *DefaultValueArray;

  for (Link = BlockData->DefaultValueEntry.ForwardLink; Link != &BlockData->DefaultValueEntry; Link = Link->ForwardLink) {
    DefaultValueArray = BASE_CR (Link, IFR_DEFAULT_DATA, Entry);
    if (DefaultValueArray->DefaultId == DefaultValueData->DefaultId) {
      //
      // Update the default value array in BlockData.
      //
      DefaultValueArray->Value = DefaultValueData->Value;
      FreePool (DefaultValueData);
      return;
    } else if (DefaultValueArray->DefaultId > DefaultValueData->DefaultId) {
      //
      // Insert new default value data in the front of this default value array.
      //
      InsertTailList (Link, &DefaultValueData->Entry);
      return;
    }
  }

  //
  // Insert new default value data in tail.
  //
  InsertTailList (Link, &DefaultValueData->Entry);
  return;
}

/**
  This function inserts new BlockData into the block link

  @param  BlockLink   The list entry points to block array.
  @param  BlockData   The point to BlockData is added.
  
**/
VOID
InsertBlockData (
  IN LIST_ENTRY        *BlockLink,
  IN IFR_BLOCK_DATA    **BlockData
  )
{
  LIST_ENTRY      *Link;
  IFR_BLOCK_DATA  *BlockArray;
  IFR_BLOCK_DATA  *BlockSingleData;

  BlockSingleData = *BlockData;
  
  //
  // Insert block data in its Offset and Width order.
  //
  for (Link = BlockLink->ForwardLink; Link != BlockLink; Link = Link->ForwardLink) {
    BlockArray = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
    if (BlockArray->Offset == BlockSingleData->Offset) {
      if (BlockArray->Width > BlockSingleData->Width) {
        //
        // Insert this block data in the front of block array
        //
        InsertTailList (Link, &BlockSingleData->Entry);
        return;
      }

      if (BlockArray->Width == BlockSingleData->Width) {
        //
        // The same block array has been added.
        //
        FreePool (BlockSingleData);
        *BlockData = BlockArray;
        return;
      }
    } else if (BlockArray->Offset > BlockSingleData->Offset) {
      //
      // Insert new block data in the front of block array 
      //
      InsertTailList (Link, &BlockSingleData->Entry);
      return;
    }
  }
  
  //
  // Add new block data into the tail.
  //
  InsertTailList (Link, &BlockSingleData->Entry);
  return;  
}

/**
  This function checks VarOffset and VarWidth is in the block range.

  @param  BlockArray         The block array is to be checked. 
  @param  VarOffset          Offset of var to the structure
  @param  VarWidth           Width of var.
  
  @retval TRUE   This Var is in the block range.
  @retval FALSE  This Var is not in the block range.
**/
BOOLEAN
BlockArrayCheck (
  IN IFR_BLOCK_DATA  *RequestBlockArray,
  IN UINT16          VarOffset,
  IN UINT16          VarWidth
  )
{
  LIST_ENTRY          *Link;
  IFR_BLOCK_DATA      *BlockData;
  
  //
  // No Request Block array, all vars are got.
  //
  if (RequestBlockArray == NULL) {
    return TRUE;
  }
  
  //
  // Check the input var is in the request block range.
  //
  for (Link = RequestBlockArray->Entry.ForwardLink; Link != &RequestBlockArray->Entry; Link = Link->ForwardLink) {
    BlockData = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
    if ((VarOffset >= BlockData->Offset) && ((VarOffset + VarWidth) <= (BlockData->Offset + BlockData->Width))) {
      return TRUE;
    }
  }

  return FALSE;
}

/**
  This function parses Form Package to get the block array and the default
  value array according to the request ConfigHdr.

  @param  Package               Pointer to the form package data.
  @param  PackageLength         Length of the pacakge.
  @param  ConfigHdr             Request string ConfigHdr. If it is NULL,
                                the first found varstore will be as ConfigHdr.
  @param  RequestBlockArray     The block array is retrieved from the request string.
  @param  VarStorageData        VarStorage structure contains the got block and default value.
  @param  PIfrDefaultIdArray    Point to the got default id and default name array.

  @retval EFI_SUCCESS           The block array and the default value array are got.
  @retval EFI_INVALID_PARAMETER The varstore defintion in the differnt form pacakges
                                are conflicted. 
  @retval EFI_OUT_OF_RESOURCES  No enough memory.
**/
EFI_STATUS
EFIAPI
ParseIfrData (
  IN     UINT8               *Package,
  IN     UINT32              PackageLenth,
  IN     EFI_STRING          ConfigHdr,
  IN     IFR_BLOCK_DATA      *RequestBlockArray,
  IN OUT IFR_VARSTORAGE_DATA *VarStorageData,
  OUT    IFR_DEFAULT_DATA    **PIfrDefaultIdArray
  )
{
  EFI_STATUS               Status;
  UINTN                    IfrOffset;
  EFI_IFR_VARSTORE         *IfrVarStore;
  EFI_IFR_OP_HEADER        *IfrOpHdr;
  EFI_IFR_ONE_OF           *IfrOneOf;
  EFI_IFR_ONE_OF_OPTION    *IfrOneOfOption;
  EFI_IFR_DEFAULT          *IfrDefault;
  EFI_IFR_ORDERED_LIST     *IfrOrderedList;
  EFI_IFR_CHECKBOX         *IfrCheckBox;
  EFI_IFR_PASSWORD         *IfrPassword;
  EFI_IFR_STRING           *IfrString;
  IFR_DEFAULT_DATA         *DefaultIdArray;
  IFR_DEFAULT_DATA         *DefaultData;
  IFR_BLOCK_DATA           *BlockData;
  CHAR16                   *VarStoreName;
  UINT16                   VarOffset;
  UINT16                   VarWidth;
  EFI_STRING_ID            VarDefaultName;
  UINT16                   VarDefaultId;
  EFI_STRING               GuidStr;
  EFI_STRING               NameStr;
  EFI_STRING               TempStr;
  UINTN                    LengthString;

  //
  // Initialize DefaultIdArray to store the map between DeaultId and DefaultName
  //
  LengthString     = 0;
  Status           = EFI_SUCCESS;
  GuidStr          = NULL;
  NameStr          = NULL;
  TempStr          = NULL;
  BlockData        = NULL;
  DefaultData      = NULL;
  DefaultIdArray   = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
  if (DefaultIdArray == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  InitializeListHead (&DefaultIdArray->Entry);

  //
  // Go through the form package to parse OpCode one by one.
  //
  IfrOffset   = sizeof (EFI_HII_PACKAGE_HEADER);
  while (IfrOffset < PackageLenth) {
    IfrOpHdr  = (EFI_IFR_OP_HEADER *) (Package + IfrOffset);

    switch (IfrOpHdr->OpCode) {
    case EFI_IFR_VARSTORE_OP:
      //
      // VarStore is found. Don't need to search any more.
      //
      if (VarStorageData->Size != 0) {
        break;
      }

      //
      // Get the requied varstore information
      // Add varstore by Guid and Name in ConfigHdr
      // Make sure Offset is in varstore size and varstoreid
      //
      IfrVarStore = (EFI_IFR_VARSTORE *) IfrOpHdr;
      VarStoreName = AllocateZeroPool (AsciiStrSize ((CHAR8 *)IfrVarStore->Name) * sizeof (CHAR16));
      if (VarStoreName == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      AsciiStrToUnicodeStr ((CHAR8 *) IfrVarStore->Name, VarStoreName);

      GenerateSubStr (L"GUID=", sizeof (EFI_GUID), (VOID *) &IfrVarStore->Guid, 1, &GuidStr);
      GenerateSubStr (L"NAME=", StrLen (VarStoreName) * sizeof (CHAR16), (VOID *) VarStoreName, 2, &NameStr);
      LengthString = StrLen (GuidStr);
      LengthString = LengthString + StrLen (NameStr) + 1;
      TempStr = AllocateZeroPool (LengthString * sizeof (CHAR16));
    if (TempStr == NULL) {
        FreePool (GuidStr);
        FreePool (NameStr);
        FreePool (VarStoreName);
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      StrCpy (TempStr, GuidStr);
      StrCat (TempStr, NameStr);
      if (ConfigHdr == NULL || StrnCmp (ConfigHdr, TempStr, StrLen (TempStr)) == 0) {
        //
        // Find the matched VarStore
        //
        CopyGuid (&VarStorageData->Guid, (EFI_GUID *) (VOID *) &IfrVarStore->Guid);
        VarStorageData->VarStoreId = IfrVarStore->VarStoreId;
        VarStorageData->Size       = IfrVarStore->Size;
        VarStorageData->Name       = VarStoreName;
      } else {
        //
        // No found, free the allocated memory 
        //
        FreePool (VarStoreName);
      }
      //
      // Free alllocated temp string.
      //
      FreePool (GuidStr);
      FreePool (NameStr);
      FreePool (TempStr);
      break;

    case EFI_IFR_DEFAULTSTORE_OP:
      //
      // Add new the map between default id and default name.
      //
      DefaultData = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
      if (DefaultData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      DefaultData->DefaultId   = ((EFI_IFR_DEFAULTSTORE *) IfrOpHdr)->DefaultId;
      DefaultData->DefaultName = ((EFI_IFR_DEFAULTSTORE *) IfrOpHdr)->DefaultName;
      InsertTailList (&DefaultIdArray->Entry, &DefaultData->Entry);
      DefaultData = NULL;
      break;

    case EFI_IFR_FORM_OP:
      //
      // No matched varstore is found and directly return.
      //
      if (VarStorageData->Size == 0) {
        Status = EFI_SUCCESS;
        goto Done;
      }
      break;

    case EFI_IFR_ONE_OF_OP:
    case EFI_IFR_NUMERIC_OP:
      //
      // Numeric and OneOf has the same opcode structure.
      //

      //
      // Check whether this question is for the requested varstore.
      //
      IfrOneOf = (EFI_IFR_ONE_OF *) IfrOpHdr;
      if (IfrOneOf->Question.VarStoreId != VarStorageData->VarStoreId) {
        break;
      }
      
      //
      // Get Offset/Width by Question header and OneOf Flags
      //
      VarOffset = IfrOneOf->Question.VarStoreInfo.VarOffset;
      VarWidth  = (UINT16) (1 << (IfrOneOf->Flags & EFI_IFR_NUMERIC_SIZE));
      //
      // Check whether this question is in requested block array.
      //
      if (!BlockArrayCheck (RequestBlockArray, VarOffset, VarWidth)) {
        //
        // This question is not in the requested string. Skip it.
        //
        break;
      }

      //
      // Check this var question is in the var storage 
      //
      if ((VarOffset + VarWidth) > VarStorageData->Size) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Set Block Data
      //
      BlockData = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
      if (BlockData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      BlockData->Offset     = VarOffset;
      BlockData->Width      = VarWidth;
      BlockData->QuestionId = IfrOneOf->Question.QuestionId;
      BlockData->OpCode     = IfrOpHdr->OpCode;
      BlockData->Scope      = IfrOpHdr->Scope;
      InitializeListHead (&BlockData->DefaultValueEntry);
      //
      // Add Block Data into VarStorageData BlockEntry
      //
      InsertBlockData (&VarStorageData->BlockEntry, &BlockData);
      break;

    case EFI_IFR_ORDERED_LIST_OP:
      //
      // offset by question header
      // width by EFI_IFR_ORDERED_LIST MaxContainers * OneofOption Type
      // no default value and default id, how to define its default value?
      //
      
      //
      // Check whether this question is for the requested varstore.
      //
      IfrOrderedList = (EFI_IFR_ORDERED_LIST *) IfrOpHdr;
      if (IfrOrderedList->Question.VarStoreId != VarStorageData->VarStoreId) {
        break;
      }
      
      //
      // Get Offset/Width by Question header and OneOf Flags
      //
      VarOffset = IfrOrderedList->Question.VarStoreInfo.VarOffset;
      VarWidth  = IfrOrderedList->MaxContainers;

      //
      // Check whether this question is in requested block array.
      //
      if (!BlockArrayCheck (RequestBlockArray, VarOffset, VarWidth)) {
        //
        // This question is not in the requested string. Skip it.
        //
        break;
      }

      //
      // Check this var question is in the var storage 
      //
      if ((VarOffset + VarWidth) > VarStorageData->Size) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Set Block Data
      //
      BlockData = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
      if (BlockData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      BlockData->Offset     = VarOffset;
      BlockData->Width      = VarWidth;
      BlockData->QuestionId = IfrOrderedList->Question.QuestionId;
      BlockData->OpCode     = IfrOpHdr->OpCode;
      BlockData->Scope      = IfrOpHdr->Scope;
      InitializeListHead (&BlockData->DefaultValueEntry);
      
      //
      // Add Block Data into VarStorageData BlockEntry
      //
      InsertBlockData (&VarStorageData->BlockEntry, &BlockData);
      break;

    case EFI_IFR_CHECKBOX_OP:
      //
      // EFI_IFR_DEFAULT_OP
      // offset by question header
      // width is 1 sizeof (BOOLEAN)
      // default id by CheckBox Flags if CheckBox flags (Default or Mau) is set, the default value is 1 to be set.
      // value by DefaultOption
      // default id by DeaultOption DefaultId can override CheckBox Flags and Default value.
      // 

      //
      // Check whether this question is for the requested varstore.
      //
      IfrCheckBox = (EFI_IFR_CHECKBOX *) IfrOpHdr;
      if (IfrCheckBox->Question.VarStoreId != VarStorageData->VarStoreId) {
        break;
      }
      
      //
      // Get Offset/Width by Question header and OneOf Flags
      //
      VarOffset = IfrCheckBox->Question.VarStoreInfo.VarOffset;
      VarWidth  = sizeof (BOOLEAN);

      //
      // Check whether this question is in requested block array.
      //
      if (!BlockArrayCheck (RequestBlockArray, VarOffset, VarWidth)) {
        //
        // This question is not in the requested string. Skip it.
        //
        break;
      }

      //
      // Check this var question is in the var storage 
      //
      if ((VarOffset + VarWidth) > VarStorageData->Size) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Set Block Data
      //
      BlockData = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
      if (BlockData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      BlockData->Offset     = VarOffset;
      BlockData->Width      = VarWidth;
      BlockData->QuestionId = IfrCheckBox->Question.QuestionId;
      BlockData->OpCode     = IfrOpHdr->OpCode;
      BlockData->Scope      = IfrOpHdr->Scope;
      InitializeListHead (&BlockData->DefaultValueEntry);
      //
      // Add Block Data into VarStorageData BlockEntry
      //
      InsertBlockData (&VarStorageData->BlockEntry, &BlockData);
      
      //
      // Add default value by CheckBox Flags 
      //
      if ((IfrCheckBox->Flags & EFI_IFR_CHECKBOX_DEFAULT) == EFI_IFR_CHECKBOX_DEFAULT) {
        //
        // Set standard ID to Manufacture ID and Get DefaultName String ID
        //
        VarDefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
        Status       = FindDefaultName (DefaultIdArray, VarDefaultId, &VarDefaultName);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        //
        // Prepare new DefaultValue
        //
        DefaultData = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
        if (DefaultData == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
        DefaultData->DefaultId   = VarDefaultId;
        DefaultData->DefaultName = VarDefaultName;
        DefaultData->Value       = 1;
        //
        // Add DefaultValue into current BlockData
        //
        InsertDefaultValue (BlockData, DefaultData);
      }

      if ((IfrCheckBox->Flags & EFI_IFR_CHECKBOX_DEFAULT_MFG) == EFI_IFR_CHECKBOX_DEFAULT_MFG) {
        //
        // Set standard ID to Manufacture ID and Get DefaultName String ID
        //
        VarDefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
        Status       = FindDefaultName (DefaultIdArray, VarDefaultId, &VarDefaultName);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        //
        // Prepare new DefaultValue
        //
        DefaultData = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
        if (DefaultData == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
        DefaultData->DefaultId   = VarDefaultId;
        DefaultData->DefaultName = VarDefaultName;
        DefaultData->Value       = 1;
        //
        // Add DefaultValue into current BlockData
        //
        InsertDefaultValue (BlockData, DefaultData);
      }
      break;

    case EFI_IFR_STRING_OP:
      //
      // offset by question header
      // width MaxSize * sizeof (CHAR16)
      // no default value, only block array
      //

      //
      // Check whether this question is for the requested varstore.
      //
      IfrString = (EFI_IFR_STRING *) IfrOpHdr;
      if (IfrString->Question.VarStoreId != VarStorageData->VarStoreId) {
        break;
      }
      
      //
      // Get Offset/Width by Question header and OneOf Flags
      //
      VarOffset = IfrString->Question.VarStoreInfo.VarOffset;
      VarWidth  = (UINT16) (IfrString->MaxSize * sizeof (UINT16));

      //
      // Check whether this question is in requested block array.
      //
      if (!BlockArrayCheck (RequestBlockArray, VarOffset, VarWidth)) {
        //
        // This question is not in the requested string. Skip it.
        //
        break;
      }

      //
      // Check this var question is in the var storage 
      //
      if ((VarOffset + VarWidth) > VarStorageData->Size) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Set Block Data
      //
      BlockData = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
      if (BlockData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      BlockData->Offset     = VarOffset;
      BlockData->Width      = VarWidth;
      BlockData->QuestionId = IfrString->Question.QuestionId;
      BlockData->OpCode     = IfrOpHdr->OpCode;
      InitializeListHead (&BlockData->DefaultValueEntry);
      
      //
      // Add Block Data into VarStorageData BlockEntry
      //
      InsertBlockData (&VarStorageData->BlockEntry, &BlockData);
      
      //
      // No default value for string.
      //
      BlockData = NULL;
      break;

    case EFI_IFR_PASSWORD_OP:
      //
      // offset by question header
      // width MaxSize * sizeof (CHAR16)
      // no default value, only block array
      //

      //
      // Check whether this question is for the requested varstore.
      //
      IfrPassword = (EFI_IFR_PASSWORD *) IfrOpHdr;
      if (IfrPassword->Question.VarStoreId != VarStorageData->VarStoreId) {
        break;
      }
      
      //
      // Get Offset/Width by Question header and OneOf Flags
      //
      VarOffset = IfrPassword->Question.VarStoreInfo.VarOffset;
      VarWidth  = (UINT16) (IfrPassword->MaxSize * sizeof (UINT16));

      //
      // Check whether this question is in requested block array.
      //
      if (!BlockArrayCheck (RequestBlockArray, VarOffset, VarWidth)) {
        //
        // This question is not in the requested string. Skip it.
        //
        break;
      }

      //
      // Check this var question is in the var storage 
      //
      if ((VarOffset + VarWidth) > VarStorageData->Size) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Set Block Data
      //
      BlockData = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
      if (BlockData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      BlockData->Offset     = VarOffset;
      BlockData->Width      = VarWidth;
      BlockData->QuestionId = IfrPassword->Question.QuestionId;
      BlockData->OpCode     = IfrOpHdr->OpCode;
      InitializeListHead (&BlockData->DefaultValueEntry);
      
      //
      // Add Block Data into VarStorageData BlockEntry
      //
      InsertBlockData (&VarStorageData->BlockEntry, &BlockData);
      
      //
      // No default value for string.
      //
      BlockData = NULL;
      break;

    case EFI_IFR_ONE_OF_OPTION_OP:
      //
      // No matched block data is ignored.
      //
      if (BlockData == NULL || BlockData->Scope == 0) {
        break;
      }
      
      IfrOneOfOption = (EFI_IFR_ONE_OF_OPTION *) IfrOpHdr;
      if (BlockData->OpCode == EFI_IFR_ORDERED_LIST_OP) {
        //
        // Get ordered list option data type.
        //
        if (IfrOneOfOption->Type == EFI_IFR_TYPE_NUM_SIZE_8 || IfrOneOfOption->Type == EFI_IFR_TYPE_BOOLEAN) {
          VarWidth = 1;
        } else if (IfrOneOfOption->Type == EFI_IFR_TYPE_NUM_SIZE_16) {
          VarWidth = 2;
        } else if (IfrOneOfOption->Type == EFI_IFR_TYPE_NUM_SIZE_32) {
          VarWidth = 4;
        } else if (IfrOneOfOption->Type == EFI_IFR_TYPE_NUM_SIZE_64) {
          VarWidth = 8;
        } else {
          //
          // Invalid ordered list option data type.
          //
          Status = EFI_INVALID_PARAMETER;
          goto Done;
        }
        //
        // Calculate Ordered list QuestionId width.
        //
        BlockData->Width = (UINT16) (BlockData->Width * VarWidth);
        BlockData = NULL;
        break;
      }

      if ((IfrOneOfOption->Flags & EFI_IFR_OPTION_DEFAULT) == EFI_IFR_OPTION_DEFAULT) {
        //
        // Set standard ID to Manufacture ID and Get DefaultName String ID
        //
        VarDefaultId = EFI_HII_DEFAULT_CLASS_STANDARD;
        Status       = FindDefaultName (DefaultIdArray, VarDefaultId, &VarDefaultName);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        //
        // Prepare new DefaultValue
        //
        DefaultData = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
        if (DefaultData == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
        DefaultData->DefaultId   = VarDefaultId;
        DefaultData->DefaultName = VarDefaultName;
        DefaultData->Value       = IfrOneOfOption->Value.u64;
        //
        // Add DefaultValue into current BlockData
        //
        InsertDefaultValue (BlockData, DefaultData);
      }

      if ((IfrOneOfOption->Flags & EFI_IFR_OPTION_DEFAULT_MFG) == EFI_IFR_OPTION_DEFAULT_MFG) {
        //
        // Set default ID to Manufacture ID and Get DefaultName String ID
        //
        VarDefaultId = EFI_HII_DEFAULT_CLASS_MANUFACTURING;
        Status       = FindDefaultName (DefaultIdArray, VarDefaultId, &VarDefaultName);
        if (EFI_ERROR (Status)) {
          goto Done;
        }
        //
        // Prepare new DefaultValue
        //
        DefaultData = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
        if (DefaultData == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
        DefaultData->DefaultId   = VarDefaultId;
        DefaultData->DefaultName = VarDefaultName;
        DefaultData->Value       = IfrOneOfOption->Value.u64;
        //
        // Add DefaultValue into current BlockData
        //
        InsertDefaultValue (BlockData, DefaultData);
      }
      break;

    case EFI_IFR_DEFAULT_OP:
      //
      // Update Current BlockData to the default value.
      //
      if (BlockData == NULL || BlockData->Scope == 0) {
        //
        // No matched block data is ignored.        
        //
        break;
      }

      if (BlockData->OpCode == EFI_IFR_ORDERED_LIST_OP) {
        //
        // OrderedList Opcode is no default value.
        //
        break;
      }
      //
      // Get the DefaultId and DefaultName String ID
      //
      IfrDefault     = (EFI_IFR_DEFAULT *) IfrOpHdr;
      VarDefaultId   = IfrDefault->DefaultId;
      Status       = FindDefaultName (DefaultIdArray, VarDefaultId, &VarDefaultName);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // Prepare new DefaultValue
      //
      DefaultData = (IFR_DEFAULT_DATA *) AllocateZeroPool (sizeof (IFR_DEFAULT_DATA));
      if (DefaultData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      DefaultData->DefaultId   = VarDefaultId;
      DefaultData->DefaultName = VarDefaultName;
      DefaultData->Value       = IfrDefault->Value.u64;
      //
      // Add DefaultValue into current BlockData
      //
      InsertDefaultValue (BlockData, DefaultData);
      break;
    case EFI_IFR_END_OP:
      //
      // End Opcode is for Var.
      //
      if (BlockData != NULL && BlockData->Scope > 0) {
        BlockData->Scope--;
      }
      break;
    default:
      if (BlockData != NULL && BlockData->Scope > 0) {
        BlockData->Scope = (UINT8) (BlockData->Scope + IfrOpHdr->Scope);
      }
      break;
    }

    IfrOffset += IfrOpHdr->Length;
  }

Done:
  //
  // Set the defualt ID array.
  //
  *PIfrDefaultIdArray = DefaultIdArray;

  return Status;  
}

/**
  This function gets the full request string and full default value string by 
  parsing IFR data in HII form packages. 
  
  When Request points to NULL string, the request string and default value string 
  for each varstore in form package will return. 

  @param  HiiHandle              Hii Handle which Hii Packages are registered.
  @param  DevicePath             Device Path which Hii Config Access Protocol is registered.
  @param  Request                Pointer to a null-terminated Unicode string in
                                 <ConfigRequest> format. When it doesn't contain
                                 any RequestElement, it will be updated to return 
                                 the full RequestElement retrieved from IFR data.
                                 If it points to NULL, the request string for the first
                                 varstore in form package will be merged into a
                                 <MultiConfigRequest> format string and return. 
  @param  AltCfgResp             Pointer to a null-terminated Unicode string in
                                 <ConfigAltResp> format. When the pointer is to NULL,
                                 the full default value string retrieved from IFR data
                                 will return. When the pinter is to a string, the
                                 full default value string retrieved from IFR data
                                 will be merged into the input string and return.
                                 When Request points to NULL, the default value string 
                                 for each varstore in form package will be merged into 
                                 a <MultiConfigAltResp> format string and return.
  @retval EFI_SUCCESS            The Results string is set to the full request string.
                                 And AltCfgResp contains all default value string.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory for the return string.
  @retval EFI_NOT_FOUND          The varstore (Guid and Name) in Request string 
                                 can't be found in Form package.
  @retval EFI_NOT_FOUND          HiiPackage can't be got on the input HiiHandle.
  @retval EFI_INVALID_PARAMETER  *Request points to NULL.

**/
EFI_STATUS
EFIAPI
GetFullStringFromHiiFormPackages (
  IN     EFI_HII_HANDLE             HiiHandle,
  IN     EFI_DEVICE_PATH_PROTOCOL   *DevicePath,
  IN OUT EFI_STRING                 *Request,
  IN OUT EFI_STRING                 *AltCfgResp
  )
{
  EFI_STATUS                   Status;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT32                       PackageListLength;  
  UINTN                        BufferSize;
  IFR_BLOCK_DATA               *RequestBlockArray;
  IFR_BLOCK_DATA               *BlockData;
  IFR_BLOCK_DATA               *NextBlockData;
  IFR_DEFAULT_DATA             *DefaultValueData;
  IFR_DEFAULT_DATA             *DefaultId;
  IFR_DEFAULT_DATA             *DefaultIdArray;
  EFI_HII_PACKAGE_HEADER       PacakgeHeader;
  UINT32                       PackageOffset;
  IFR_VARSTORAGE_DATA          *VarStorageData;
  EFI_STRING                   DefaultAltCfgResp;
  EFI_STRING                   FullConfigRequest;
  EFI_STRING                   ConfigHdr;
  EFI_STRING                   GuidStr;
  EFI_STRING                   NameStr;
  EFI_STRING                   PathStr;
  EFI_STRING                   StringPtr;
  UINTN                        Length;
  UINT8                        *TmpBuffer;
  UINT16                       Offset;
  UINT16                       Width;
  LIST_ENTRY                   *Link;
  LIST_ENTRY                   *LinkData;
  LIST_ENTRY                   *LinkDefault;

  //
  // Initialize the local variables.
  //
  RequestBlockArray = NULL;
  VarStorageData    = NULL;
  DefaultAltCfgResp = NULL;
  FullConfigRequest = NULL;
  ConfigHdr         = NULL;
  DefaultIdArray    = NULL;
  GuidStr           = NULL;
  NameStr           = NULL;
  PathStr           = NULL;

  //
  // 1. Get HiiPackage by HiiHandle
  //
  BufferSize      = 0;
  HiiPackageList  = NULL;
  Status = HiiExportPackageLists (&mPrivate.HiiDatabase, HiiHandle, &BufferSize, HiiPackageList);

  //
  // The return status should always be EFI_BUFFER_TOO_SMALL as input buffer's size is 0.
  //
  if (Status != EFI_BUFFER_TOO_SMALL) {
    return Status;
  }

  HiiPackageList = AllocatePool (BufferSize);
  if (HiiPackageList == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Get PackageList on HiiHandle
  //
  Status = HiiExportPackageLists (&mPrivate.HiiDatabase, HiiHandle, &BufferSize, HiiPackageList);
  if (EFI_ERROR (Status)) {
    goto Done;
  }

  //
  // 2. Parse FormPackage to get BlockArray and DefaultId Array for the request BlockArray.
  //    1) Request is NULL.
  //    2) Request is not NULL. And it doesn't contain any BlockArray.
  //    3) Request is not NULL. And it containts BlockArray.
  //

  //
  // Initialize VarStorageData to store the var store Block and Default value information.
  //
  VarStorageData = (IFR_VARSTORAGE_DATA *) AllocateZeroPool (sizeof (IFR_VARSTORAGE_DATA));
  if (VarStorageData == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  InitializeListHead (&VarStorageData->Entry);
  InitializeListHead (&VarStorageData->BlockEntry);

  //
  // Gte the request block array by Request String 
  //
  StringPtr = NULL;
  if (*Request != NULL) {
    StringPtr = StrStr (*Request, L"&OFFSET=");
  }
  if (StringPtr != NULL) {
    //
    // Init RequestBlockArray
    //
    RequestBlockArray = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
    if (RequestBlockArray == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
    InitializeListHead (&RequestBlockArray->Entry);

    //
    // Get the request Block array from the request string
    // Offset and Width
    //

    //
    // Parse each <RequestElement> if exists
    // Only <BlockName> format is supported by this help function.
    // <BlockName> ::= &'OFFSET='<Number>&'WIDTH='<Number>
    //
    while (*StringPtr != 0 && StrnCmp (StringPtr, L"&OFFSET=", StrLen (L"&OFFSET=")) == 0) {
      //
      // Skip the OFFSET string
      //  
      StringPtr += StrLen (L"&OFFSET=");
      //
      // Get Offset
      //
      Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      Offset = 0;
      CopyMem (
        &Offset,
        TmpBuffer,
        (((Length + 1) / 2) < sizeof (UINT16)) ? ((Length + 1) / 2) : sizeof (UINT16)
        );
      FreePool (TmpBuffer);
  
      StringPtr += Length;
      if (StrnCmp (StringPtr, L"&WIDTH=", StrLen (L"&WIDTH=")) != 0) {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      StringPtr += StrLen (L"&WIDTH=");
  
      //
      // Get Width
      //
      Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      Width = 0;
      CopyMem (
        &Width,
        TmpBuffer,
        (((Length + 1) / 2) < sizeof (UINT16)) ? ((Length + 1) / 2) : sizeof (UINT16)
        );
      FreePool (TmpBuffer);

      StringPtr += Length;
      if (*StringPtr != 0 && *StringPtr != L'&') {
        Status = EFI_INVALID_PARAMETER;
        goto Done;
      }
      
      //
      // Set Block Data
      //
      BlockData = (IFR_BLOCK_DATA *) AllocateZeroPool (sizeof (IFR_BLOCK_DATA));
      if (BlockData == NULL) {
        Status = EFI_OUT_OF_RESOURCES;
        goto Done;
      }
      BlockData->Offset = Offset;
      BlockData->Width  = Width;
      InsertBlockData (&RequestBlockArray->Entry, &BlockData);

      //
      // If '\0', parsing is finished. 
      //
      if (*StringPtr == 0) {
        break;
      }
    }
    
    //
    // Merge the requested block data.
    //
    Link = RequestBlockArray->Entry.ForwardLink;
    while ((Link != &RequestBlockArray->Entry) && (Link->ForwardLink != &RequestBlockArray->Entry)) {
      BlockData = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
      NextBlockData = BASE_CR (Link->ForwardLink, IFR_BLOCK_DATA, Entry);
      if ((NextBlockData->Offset >= BlockData->Offset) && (NextBlockData->Offset <= (BlockData->Offset + BlockData->Width))) {
        if ((NextBlockData->Offset + NextBlockData->Width) > (BlockData->Offset + BlockData->Width)) {
          BlockData->Width = (UINT16) (NextBlockData->Offset + NextBlockData->Width - BlockData->Offset);
        }
        RemoveEntryList (Link->ForwardLink);
        FreePool (NextBlockData);
        continue;
      }
      Link = Link->ForwardLink;      
    }
  }
  
  //
  // Get the form package
  //
  PackageOffset     = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  PackageListLength = ReadUnaligned32 (&HiiPackageList->PackageLength);
  while (PackageOffset < PackageListLength) {
    CopyMem (&PacakgeHeader, (UINT8 *) HiiPackageList + PackageOffset, sizeof (PacakgeHeader));

    if (PacakgeHeader.Type == EFI_HII_PACKAGE_FORMS) {
      //
      // Reset VarStorageData
      //
      VarStorageData->Size = 0;
      VarStorageData->VarStoreId = 0;
      if (VarStorageData->Name != NULL) {
        FreePool (VarStorageData->Name);
        VarStorageData->Name = NULL;
      }

      //
      // Parse the opcode in form package 
      //
      Status = ParseIfrData ((UINT8 *) HiiPackageList + PackageOffset, PacakgeHeader.Length, *Request, RequestBlockArray, VarStorageData, &DefaultIdArray);
      if (EFI_ERROR (Status)) {
        goto Done;
      }

      //
      // Only one form is in a pacakge list.
      //
      break;
    }

    PackageOffset += PacakgeHeader.Length;
  }
  
  //
  // No requested varstore in IFR data and directly return
  //
  if (VarStorageData->Size == 0) {
    goto Done;
  }

  //
  // 3. Construct Request Element (Block Name) for 2.1 and 2.2 case.
  //

  //
  // Construct <ConfigHdr> : "GUID=...&NAME=...&PATH=..." by VarStorageData Guid, Name and DriverHandle
  //
  GenerateSubStr (L"GUID=", sizeof (EFI_GUID), (VOID *) &VarStorageData->Guid, 1, &GuidStr);
  GenerateSubStr (L"NAME=", StrLen (VarStorageData->Name) * sizeof (CHAR16), (VOID *) VarStorageData->Name, 2, &NameStr);
  GenerateSubStr (
    L"PATH=",
    GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) DevicePath),
    (VOID *) DevicePath,
    1,
    &PathStr
    );
  Length = StrLen (GuidStr);
  Length = Length + StrLen (NameStr);
  Length = Length + StrLen (PathStr) + 1;
  ConfigHdr = AllocateZeroPool (Length * sizeof (CHAR16));
  if (ConfigHdr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;    
  }
  StrCpy (ConfigHdr, GuidStr);
  StrCat (ConfigHdr, NameStr);
  StrCat (ConfigHdr, PathStr);

  //
  // Remove the last character L'&'
  //
  *(ConfigHdr + StrLen (ConfigHdr) - 1) = L'\0';

  if (RequestBlockArray == NULL) {
    //
    // Append VarStorageData BlockEntry into *Request string
    // Now support only one varstore in a form package.
    //

    //
    // Go through all VarStorageData Entry and get BlockEntry for each one for the multiple varstore in a single form package
    // Then construct them all to return MultiRequest string : ConfigHdr BlockConfig
    //

    //
    // Compute the length of the entire request starting with <ConfigHdr> and a 
    // Null-terminator
    //
    Length = StrLen (ConfigHdr) + 1;

    for (Link = VarStorageData->BlockEntry.ForwardLink; Link != &VarStorageData->BlockEntry; Link = Link->ForwardLink) {
      //
      // Add <BlockName> length for each Offset/Width pair
      //
      // <BlockName> ::= &OFFSET=1234&WIDTH=1234
      //                 |  8   | 4 |   7  | 4 |
      //
      Length = Length + (8 + 4 + 7 + 4);
    }
    
    //
    // Allocate buffer for the entire <ConfigRequest>
    //
    FullConfigRequest = AllocateZeroPool (Length * sizeof (CHAR16));
    if (FullConfigRequest == NULL) {
      goto Done;
    }
    StringPtr = FullConfigRequest;
  
    //
    // Start with <ConfigHdr>
    //
    StrCpy (StringPtr, ConfigHdr);
    StringPtr += StrLen (StringPtr);

    //
    // Loop through all the Offset/Width pairs and append them to ConfigRequest
    //
    for (Link = VarStorageData->BlockEntry.ForwardLink; Link != &VarStorageData->BlockEntry; Link = Link->ForwardLink) {
      BlockData = BASE_CR (Link, IFR_BLOCK_DATA, Entry);
      //
      // Append &OFFSET=XXXX&WIDTH=YYYY\0
      //
      UnicodeSPrint (
        StringPtr, 
        (8 + 4 + 7 + 4 + 1) * sizeof (CHAR16), 
        L"&OFFSET=%04X&WIDTH=%04X", 
        BlockData->Offset, 
        BlockData->Width
      );
      StringPtr += StrLen (StringPtr);
    }
    //
    // Set to the got full request string.
    //
    HiiToLower (FullConfigRequest);
    if (*Request != NULL) {
      FreePool (*Request);
    }
    *Request = FullConfigRequest;
  }
  
  //
  // 4. Construct Default Value string in AltResp according to request element.
  // Go through all VarStorageData Entry and get the DefaultId array for each one
  // Then construct them all to : ConfigHdr AltConfigHdr ConfigBody AltConfigHdr ConfigBody
  //

  //
  // Add length for <ConfigHdr> + '\0'
  //
  Length = StrLen (ConfigHdr) + 1;
  
  for (Link = DefaultIdArray->Entry.ForwardLink; Link != &DefaultIdArray->Entry; Link = Link->ForwardLink) {
    DefaultId = BASE_CR (Link, IFR_DEFAULT_DATA, Entry);
    //
    // Add length for "&<ConfigHdr>&ALTCFG=XXXX"
    //                |1| StrLen (ConfigHdr) | 8 | 4 |
    //
    Length += (1 + StrLen (ConfigHdr) + 8 + 4);
    
    for (LinkData = VarStorageData->BlockEntry.ForwardLink; LinkData != &VarStorageData->BlockEntry; LinkData = LinkData->ForwardLink) {
      BlockData = BASE_CR (LinkData, IFR_BLOCK_DATA, Entry);
      for (LinkDefault = BlockData->DefaultValueEntry.ForwardLink; LinkDefault != &BlockData->DefaultValueEntry; LinkDefault = LinkDefault->ForwardLink) {
        DefaultValueData = BASE_CR (LinkDefault, IFR_DEFAULT_DATA, Entry);
        if (DefaultValueData->DefaultId == DefaultId->DefaultId) {
          //
          // Add length for "&OFFSET=XXXX&WIDTH=YYYY&VALUE=zzzzzzzzzzzz"
          //                |    8  | 4 |   7  | 4 |   7  | Width * 2 |
          //
          Length += (8 + 4 + 7 + 4 + 7 + BlockData->Width * 2);       
        }
      }
    }
  }

  //
  // Allocate buffer for the entire <DefaultAltCfgResp>
  //
  DefaultAltCfgResp = AllocateZeroPool (Length * sizeof (CHAR16));
  if (DefaultAltCfgResp == NULL) {
    goto Done;
  }
  StringPtr = DefaultAltCfgResp;

  //
  // Start with <ConfigHdr>
  //
  StrCpy (StringPtr, ConfigHdr);
  StringPtr += StrLen (StringPtr);

  for (Link = DefaultIdArray->Entry.ForwardLink; Link != &DefaultIdArray->Entry; Link = Link->ForwardLink) {
    DefaultId = BASE_CR (Link, IFR_DEFAULT_DATA, Entry);
    //
    // Add <AltConfigHdr> of the form "&<ConfigHdr>&ALTCFG=XXXX\0"
    //                                |1| StrLen (ConfigHdr) | 8 | 4 |
    //
    UnicodeSPrint (
      StringPtr, 
      (1 + StrLen (ConfigHdr) + 8 + 4 + 1) * sizeof (CHAR16), 
      L"&%s&ALTCFG=%04X", 
      ConfigHdr, 
      DefaultId->DefaultName
      );
    StringPtr += StrLen (StringPtr);
    
    for (LinkData = VarStorageData->BlockEntry.ForwardLink; LinkData != &VarStorageData->BlockEntry; LinkData = LinkData->ForwardLink) {
      BlockData = BASE_CR (LinkData, IFR_BLOCK_DATA, Entry);
      for (LinkDefault = BlockData->DefaultValueEntry.ForwardLink; LinkDefault != &BlockData->DefaultValueEntry; LinkDefault = LinkDefault->ForwardLink) {
        DefaultValueData = BASE_CR (LinkDefault, IFR_DEFAULT_DATA, Entry);
        if (DefaultValueData->DefaultId == DefaultId->DefaultId) {
          //
          // Add <BlockConfig>
          // <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE'=<Number>
          //
          UnicodeSPrint (
            StringPtr, 
            (8 + 4 + 7 + 4 + 7 + 1) * sizeof (CHAR16),
            L"&OFFSET=%04X&WIDTH=%04X&VALUE=", 
            BlockData->Offset, 
            BlockData->Width
            );
          StringPtr += StrLen (StringPtr);

          //
          // Convert Value to a hex string in "%x" format
          // NOTE: This is in the opposite byte that GUID and PATH use
          //
          Width     = BlockData->Width;
          TmpBuffer = (UINT8 *) &(DefaultValueData->Value);
          for (; Width > 0; Width--) {
            StringPtr += UnicodeValueToString (StringPtr, PREFIX_ZERO | RADIX_HEX, TmpBuffer[Width - 1], 2);
          }
        }
      }
    }
  }
  HiiToLower (DefaultAltCfgResp);

  //
  // 5. Merge string into the input AltCfgResp if the iput *AltCfgResp is not NULL.
  //
  if (*AltCfgResp != NULL) {
    Status = MergeDefaultString (AltCfgResp, DefaultAltCfgResp);
    FreePool (DefaultAltCfgResp);
  } else {
   *AltCfgResp = DefaultAltCfgResp;
  }

Done:
  if (RequestBlockArray != NULL) {
    //
    // Free Link Array RequestBlockArray
    //
    while (!IsListEmpty (&RequestBlockArray->Entry)) {
      BlockData = BASE_CR (RequestBlockArray->Entry.ForwardLink, IFR_BLOCK_DATA, Entry);
      RemoveEntryList (&BlockData->Entry);
      FreePool (BlockData);
    }

    FreePool (RequestBlockArray);
  }
  
  if (VarStorageData != NULL) {
    //
    // Free link array VarStorageData
    //
    while (!IsListEmpty (&VarStorageData->BlockEntry)) {
      BlockData = BASE_CR (VarStorageData->BlockEntry.ForwardLink, IFR_BLOCK_DATA, Entry);
      RemoveEntryList (&BlockData->Entry);
      //
      // Free default value link array
      //
      while (!IsListEmpty (&BlockData->DefaultValueEntry)) {
        DefaultValueData = BASE_CR (BlockData->DefaultValueEntry.ForwardLink, IFR_DEFAULT_DATA, Entry);
        RemoveEntryList (&DefaultValueData->Entry);
        FreePool (DefaultValueData);
      }
      FreePool (BlockData);
    }
    FreePool (VarStorageData);
  }

  if (DefaultIdArray != NULL) {
    //
    // Free DefaultId Array
    //
    while (!IsListEmpty (&DefaultIdArray->Entry)) {
      DefaultId = BASE_CR (DefaultIdArray->Entry.ForwardLink, IFR_DEFAULT_DATA, Entry);
      RemoveEntryList (&DefaultId->Entry);
      FreePool (DefaultId);
    }
    FreePool (DefaultIdArray);
  }
  
  //
  // Free the allocated string 
  //
  if (GuidStr != NULL) {
    FreePool (GuidStr);
  }
  if (NameStr != NULL) {
    FreePool (NameStr);
  }
  if (PathStr != NULL) {
    FreePool (PathStr);
  }
  if (ConfigHdr != NULL) {
    FreePool (ConfigHdr);
  }

  //
  // Free Pacakge data
  //
  if (HiiPackageList != NULL) {
    FreePool (HiiPackageList);
  }

  return Status;
}

/**
  This function allows a caller to extract the current configuration
  for one or more named elements from one or more drivers.

  @param  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param  Request                A null-terminated Unicode string in
                                 <MultiConfigRequest> format.
  @param  Progress               On return, points to a character in the Request
                                 string. Points to the string's null terminator if
                                 request was successful. Points to the most recent
                                 & before the first failing name / value pair (or
                                 the beginning of the string if the failure is in
                                 the first name / value pair) if the request was
                                 not successful.
  @param  Results                Null-terminated Unicode string in
                                 <MultiConfigAltResp> format which has all values
                                 filled in for the names in the Request string.
                                 String to be allocated by the called function.

  @retval EFI_SUCCESS            The Results string is filled with the values
                                 corresponding to all requested names.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the parts of the
                                 results that must be stored awaiting possible
                                 future        protocols.
  @retval EFI_NOT_FOUND          Routing data doesn't match any known driver.
                                   Progress set to the "G" in "GUID" of the routing
                                  header that doesn't match. Note: There is no
                                    requirement that all routing data be validated
                                 before any configuration extraction.
  @retval EFI_INVALID_PARAMETER  For example, passing in a NULL for the Request
                                 parameter would result in this type of error. The
                                 Progress parameter is set to NULL.
  @retval EFI_INVALID_PARAMETER  Illegal syntax. Progress set to most recent &
                                 before the error or the beginning of the string.
  @retval EFI_INVALID_PARAMETER  Unknown name. Progress points to the & before the
                                 name in question.

**/
EFI_STATUS
EFIAPI
HiiConfigRoutingExtractConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       Request,
  OUT EFI_STRING                             *Progress,
  OUT EFI_STRING                             *Results
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_STRING                          StringPtr;
  EFI_STRING                          ConfigRequest;
  UINTN                               Length;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  EFI_STATUS                          Status;
  LIST_ENTRY                          *Link;
  HII_DATABASE_RECORD                 *Database;
  UINT8                               *DevicePathPkg;
  UINT8                               *CurrentDevicePath;
  EFI_HANDLE                          DriverHandle;
  EFI_HII_HANDLE                      HiiHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL      *ConfigAccess;
  EFI_STRING                          AccessProgress;
  EFI_STRING                          AccessResults;
  EFI_STRING                          DefaultResults;
  BOOLEAN                             FirstElement;
  UINTN                               DevicePathLength;

  if (This == NULL || Progress == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Request == NULL) {
    *Progress = NULL;
    return EFI_INVALID_PARAMETER;
  }

  Private   = CONFIG_ROUTING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  StringPtr = Request;
  *Progress = StringPtr;
  DefaultResults = NULL;
  ConfigRequest  = NULL;
  Status         = EFI_SUCCESS;
  AccessResults  = NULL;
  DevicePath     = NULL;

  //
  // The first element of <MultiConfigRequest> should be
  // <GuidHdr>, which is in 'GUID='<Guid> syntax.
  //
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  FirstElement = TRUE;

  //
  // Allocate a fix length of memory to store Results. Reallocate memory for
  // Results if this fix length is insufficient.
  //
  *Results = (EFI_STRING) AllocateZeroPool (MAX_STRING_LENGTH);
  if (*Results == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  while (*StringPtr != 0 && StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) == 0) {
    //
    // If parsing error, set Progress to the beginning of the <MultiConfigRequest>
    // or most recent & before the error.
    //
    if (StringPtr == Request) {
      *Progress = StringPtr;
    } else {
      *Progress = StringPtr - 1;
    }

    //
    // Process each <ConfigRequest> of <MultiConfigRequest>
    //
    Length = CalculateConfigStringLen (StringPtr);
    ConfigRequest = AllocateCopyPool ((Length + 1) * sizeof (CHAR16), StringPtr);
    if (ConfigRequest == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
    *(ConfigRequest + Length) = 0;

    //
    // Get the UEFI device path
    //
    Status = GetDevicePath (ConfigRequest, (UINT8 **) &DevicePath);
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Find driver which matches the routing data.
    //
    DriverHandle     = NULL;
    HiiHandle        = NULL;
    DevicePathLength = GetDevicePathSize (DevicePath);
    for (Link = Private->DatabaseList.ForwardLink;
         Link != &Private->DatabaseList;
         Link = Link->ForwardLink
        ) {
      Database = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
   
      if ((DevicePathPkg = Database->PackageList->DevicePathPkg) != NULL) {
        CurrentDevicePath = DevicePathPkg + sizeof (EFI_HII_PACKAGE_HEADER);
        if ((DevicePathLength == GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) CurrentDevicePath)) && 
            (CompareMem (
              DevicePath,
              CurrentDevicePath,
              DevicePathLength
              ) == 0)) {
          DriverHandle = Database->DriverHandle;
          HiiHandle    = Database->Handle;
          break;
        }
      }
    }
    
    //
    // Try to find driver handle by device path.
    //
    if (DriverHandle == NULL) {
      TempDevicePath = DevicePath;
      Status = gBS->LocateDevicePath (
                      &gEfiDevicePathProtocolGuid,
                      &TempDevicePath,
                      &DriverHandle
                      );
      if (EFI_ERROR (Status) || (DriverHandle == NULL)) {
        //
        // Routing data does not match any known driver.
        // Set Progress to the 'G' in "GUID" of the routing header.
        //
        *Progress = StringPtr;
        Status = EFI_NOT_FOUND;
        goto Done;
      }
    }
    
    //
    // Check whether ConfigRequest contains request string OFFSET/WIDTH
    //
    if ((HiiHandle != NULL) && (StrStr (ConfigRequest, L"&OFFSET=") == NULL)) {
      //
      // Get the full request string from IFR when HiiPackage is registered to HiiHandle 
      //
      Status = GetFullStringFromHiiFormPackages (HiiHandle, DevicePath, &ConfigRequest, &DefaultResults);
      if (EFI_ERROR (Status)) {
        goto Done;
      }
      //
      // Not any request block is found.
      //
      if (StrStr (ConfigRequest, L"&OFFSET=") == NULL) {
        AccessResults = AllocateCopyPool (StrSize (ConfigRequest), ConfigRequest);
        goto NextConfigString;
      }
    }

    //
    // Call corresponding ConfigAccess protocol to extract settings
    //
    Status = gBS->HandleProtocol (
                    DriverHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    (VOID **) &ConfigAccess
                    );
    ASSERT_EFI_ERROR (Status);

    Status = ConfigAccess->ExtractConfig (
                             ConfigAccess,
                             ConfigRequest,
                             &AccessProgress,
                             &AccessResults
                             );
    if (EFI_ERROR (Status)) {
      //
      // AccessProgress indicates the parsing progress on <ConfigRequest>.
      // Map it to the progress on <MultiConfigRequest> then return it.
      //
      *Progress = StrStr (StringPtr, AccessProgress);
      goto Done;
    }

    //
    // Attach this <ConfigAltResp> to a <MultiConfigAltResp>. There is a '&'
    // which seperates the first <ConfigAltResp> and the following ones.
    //
    ASSERT (*AccessProgress == 0);

    //
    // Update AccessResults by getting default setting from IFR when HiiPackage is registered to HiiHandle 
    //
    if (HiiHandle != NULL) {
      if (DefaultResults == NULL) {
        Status = GetFullStringFromHiiFormPackages (HiiHandle, DevicePath, &ConfigRequest, &AccessResults);
      } else {
        Status = MergeDefaultString (&AccessResults, DefaultResults);
      }
    }
    FreePool (DevicePath);
    DevicePath = NULL;
    
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    //
    // Free the allocated memory.
    //
    if (DefaultResults != NULL) {
      FreePool (DefaultResults);
      DefaultResults = NULL;
    }

NextConfigString:   
    if (!FirstElement) {
      Status = AppendToMultiString (Results, L"&");
      ASSERT_EFI_ERROR (Status);
    }
    
    Status = AppendToMultiString (Results, AccessResults);
    ASSERT_EFI_ERROR (Status);

    FirstElement = FALSE;

    FreePool (AccessResults);
    AccessResults = NULL;
    FreePool (ConfigRequest);
    ConfigRequest = NULL;

    //
    // Go to next <ConfigRequest> (skip '&').
    //
    StringPtr += Length;
    if (*StringPtr == 0) {
      *Progress = StringPtr;
      break;
    }

    StringPtr++;
  }

Done:
  if (EFI_ERROR (Status)) {
    FreePool (*Results);
  *Results = NULL;
  }
  
  if (ConfigRequest != NULL) {
    FreePool (ConfigRequest);
  }
  
  if (AccessResults != NULL) {
    FreePool (AccessResults);
  }
  
  if (DefaultResults != NULL) {
    FreePool (DefaultResults);
  }
  
  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }  

  return Status;
}


/**
  This function allows the caller to request the current configuration for the
  entirety of the current HII database and returns the data in a
  null-terminated Unicode string.

  @param  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param  Results                Null-terminated Unicode string in
                                 <MultiConfigAltResp> format which has all values
                                 filled in for the names in the Request string.
                                 String to be allocated by the  called function.
                                 De-allocation is up to the caller.

  @retval EFI_SUCCESS            The Results string is filled with the values
                                 corresponding to all requested names.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the parts of the
                                 results that must be stored awaiting possible
                                 future        protocols.
  @retval EFI_INVALID_PARAMETER  For example, passing in a NULL for the Results
                                 parameter would result in this type of error.

**/
EFI_STATUS
EFIAPI
HiiConfigRoutingExportConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  OUT EFI_STRING                             *Results
  )
{
  EFI_STATUS                          Status;
  EFI_HII_CONFIG_ACCESS_PROTOCOL      *ConfigAccess;
  EFI_STRING                          AccessResults;
  EFI_STRING                          Progress;
  EFI_STRING                          ConfigRequest;
  UINTN                               Index;
  EFI_HANDLE                          *ConfigAccessHandles;
  UINTN                               NumberConfigAccessHandles;
  BOOLEAN                             FirstElement;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_HII_HANDLE                      HiiHandle;
  EFI_STRING                          DefaultResults;
  HII_DATABASE_PRIVATE_DATA           *Private;
  LIST_ENTRY                          *Link;
  HII_DATABASE_RECORD                 *Database;
  UINT8                               *DevicePathPkg;
  UINT8                               *CurrentDevicePath;
  UINTN                               DevicePathLength;

  if (This == NULL || Results == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Private = CONFIG_ROUTING_DATABASE_PRIVATE_DATA_FROM_THIS (This);

  //
  // Allocate a fix length of memory to store Results. Reallocate memory for
  // Results if this fix length is insufficient.
  //
  *Results = (EFI_STRING) AllocateZeroPool (MAX_STRING_LENGTH);
  if (*Results == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  NumberConfigAccessHandles = 0;
  Status = gBS->LocateHandleBuffer (
             ByProtocol,
             &gEfiHiiConfigAccessProtocolGuid,
             NULL,
             &NumberConfigAccessHandles,
             &ConfigAccessHandles
             );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  FirstElement = TRUE;

  for (Index = 0; Index < NumberConfigAccessHandles; Index++) {
    Status = gBS->HandleProtocol (
                    ConfigAccessHandles[Index],
                    &gEfiHiiConfigAccessProtocolGuid,
                    (VOID **) &ConfigAccess
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Get DevicePath and HiiHandle for this ConfigAccess driver handle
    //
    Progress         = NULL;
    HiiHandle        = NULL;
    ConfigRequest    = NULL;
    DefaultResults   = NULL;
    DevicePath       = DevicePathFromHandle (ConfigAccessHandles[Index]);
    DevicePathLength = GetDevicePathSize (DevicePath);
    if (DevicePath != NULL) {
      for (Link = Private->DatabaseList.ForwardLink;
           Link != &Private->DatabaseList;
           Link = Link->ForwardLink
          ) {
        Database = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);
        if ((DevicePathPkg = Database->PackageList->DevicePathPkg) != NULL) {
          CurrentDevicePath = DevicePathPkg + sizeof (EFI_HII_PACKAGE_HEADER);
          if ((DevicePathLength == GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) CurrentDevicePath)) &&
              (CompareMem (
                DevicePath,
                CurrentDevicePath,
                DevicePathLength
                ) == 0)) {
            HiiHandle = Database->Handle;
            break;
          }
        }
      }
    }

    //
    // Update AccessResults by getting default setting from IFR when HiiPackage is registered to HiiHandle 
    //
    if (HiiHandle != NULL && DevicePath != NULL) {
      Status = GetFullStringFromHiiFormPackages (HiiHandle, DevicePath, &ConfigRequest, &DefaultResults);
    }
    //
    // Can't parse IFR data to get the request string and default string.
    //
    if (EFI_ERROR (Status)) {
      ConfigRequest  = NULL;
      DefaultResults = NULL;
    }
       
    Status = ConfigAccess->ExtractConfig (
                             ConfigAccess,
                             ConfigRequest,
                             &Progress,
                             &AccessResults
                             );
    if (!EFI_ERROR (Status)) {
      //
      // Merge the default sting from IFR code into the got setting from driver.
      //
      if (DefaultResults != NULL) {
        MergeDefaultString (&AccessResults, DefaultResults);
        FreePool (DefaultResults);
      }
      
      //
      // Attach this <ConfigAltResp> to a <MultiConfigAltResp>. There is a '&'
      // which seperates the first <ConfigAltResp> and the following ones.      
      //
      if (!FirstElement) {
        Status = AppendToMultiString (Results, L"&");
        ASSERT_EFI_ERROR (Status);
      }
      
      Status = AppendToMultiString (Results, AccessResults);
      ASSERT_EFI_ERROR (Status);

      FirstElement = FALSE;
      
      FreePool (AccessResults);
      AccessResults = NULL;
    }
  }
  FreePool (ConfigAccessHandles);

  return EFI_SUCCESS;  
}


/**
  This function processes the results of processing forms and routes it to the
  appropriate handlers or storage.

  @param  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param  Configuration          A null-terminated Unicode string in
                                 <MulltiConfigResp> format.
  @param  Progress               A pointer to a string filled in with the offset of
                                 the most recent & before the first failing name /
                                 value pair (or the beginning of the string if the
                                 failure is in the first name / value pair) or the
                                 terminating NULL if all was successful.

  @retval EFI_SUCCESS            The results have been distributed or are awaiting
                                 distribution.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to store the parts of the
                                 results that must be stored awaiting possible
                                 future        protocols.
  @retval EFI_INVALID_PARAMETER  Passing in a NULL for the Configuration parameter
                                 would result in this type of error.
  @retval EFI_NOT_FOUND          Target for the specified routing data was not
                                 found.

**/
EFI_STATUS
EFIAPI
HiiConfigRoutingRouteConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       Configuration,
  OUT EFI_STRING                             *Progress
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_STRING                          StringPtr;
  EFI_STRING                          ConfigResp;
  UINTN                               Length;
  EFI_STATUS                          Status;
  EFI_DEVICE_PATH_PROTOCOL            *DevicePath;
  EFI_DEVICE_PATH_PROTOCOL            *TempDevicePath;
  LIST_ENTRY                          *Link;
  HII_DATABASE_RECORD                 *Database;
  UINT8                               *DevicePathPkg;
  UINT8                               *CurrentDevicePath;
  EFI_HANDLE                          DriverHandle;
  EFI_HII_CONFIG_ACCESS_PROTOCOL      *ConfigAccess;
  EFI_STRING                          AccessProgress;
  UINTN                               DevicePathLength;

  if (This == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Configuration == NULL) {
    *Progress = NULL;
    return EFI_INVALID_PARAMETER;
  }

  Private   = CONFIG_ROUTING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  StringPtr = Configuration;
  *Progress = StringPtr;

  //
  // The first element of <MultiConfigResp> should be
  // <GuidHdr>, which is in 'GUID='<Guid> syntax.
  //
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  while (*StringPtr != 0 && StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) == 0) {
    //
    // If parsing error, set Progress to the beginning of the <MultiConfigResp>
    // or most recent & before the error.
    //
    if (StringPtr == Configuration) {
      *Progress = StringPtr;
    } else {
      *Progress = StringPtr - 1;
    }

    //
    // Process each <ConfigResp> of <MultiConfigResp>
    //
    Length = CalculateConfigStringLen (StringPtr);
    ConfigResp = AllocateCopyPool ((Length + 1) * sizeof (CHAR16), StringPtr);
    if (ConfigResp == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    //
    // Append '\0' to the end of ConfigRequest
    //
    *(ConfigResp + Length) = 0;

    //
    // Get the UEFI device path
    //
    Status = GetDevicePath (ConfigResp, (UINT8 **) &DevicePath);
    if (EFI_ERROR (Status)) {
      FreePool (ConfigResp);
      return Status;
    }

    //
    // Find driver which matches the routing data.
    //
    DriverHandle     = NULL;
    DevicePathLength = GetDevicePathSize (DevicePath);
    for (Link = Private->DatabaseList.ForwardLink;
         Link != &Private->DatabaseList;
         Link = Link->ForwardLink
        ) {
      Database = CR (Link, HII_DATABASE_RECORD, DatabaseEntry, HII_DATABASE_RECORD_SIGNATURE);

      if ((DevicePathPkg = Database->PackageList->DevicePathPkg) != NULL) {
        CurrentDevicePath = DevicePathPkg + sizeof (EFI_HII_PACKAGE_HEADER);
        if ((DevicePathLength == GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) CurrentDevicePath)) &&
            (CompareMem (
              DevicePath,
              CurrentDevicePath,
              DevicePathLength
              ) == 0)) {
          DriverHandle = Database->DriverHandle;
          break;
        }
      }
    }

    //
    // Try to find driver handle by device path.
    //
    if (DriverHandle == NULL) {
      TempDevicePath = DevicePath;
      Status = gBS->LocateDevicePath (
                      &gEfiDevicePathProtocolGuid,
                      &TempDevicePath,
                      &DriverHandle
                      );
      if (EFI_ERROR (Status) || (DriverHandle == NULL)) {
        //
        // Routing data does not match any known driver.
        // Set Progress to the 'G' in "GUID" of the routing header.
        //
        FreePool (DevicePath);
        *Progress = StringPtr;
        FreePool (ConfigResp);
        return EFI_NOT_FOUND;
      }
    }

    FreePool (DevicePath);

    //
    // Call corresponding ConfigAccess protocol to route settings
    //
    Status = gBS->HandleProtocol (
                    DriverHandle,
                    &gEfiHiiConfigAccessProtocolGuid,
                    (VOID **)  &ConfigAccess
                    );
    ASSERT_EFI_ERROR (Status);

    Status = ConfigAccess->RouteConfig (
                             ConfigAccess,
                             ConfigResp,
                             &AccessProgress
                             );

    if (EFI_ERROR (Status)) {
      //
      // AccessProgress indicates the parsing progress on <ConfigResp>.
      // Map it to the progress on <MultiConfigResp> then return it.
      //
      *Progress = StrStr (StringPtr, AccessProgress);

      FreePool (ConfigResp);
      return Status;
    }

    FreePool (ConfigResp);
    ConfigResp = NULL;

    //
    // Go to next <ConfigResp> (skip '&').
    //
    StringPtr += Length;
    if (*StringPtr == 0) {
      *Progress = StringPtr;
      break;
    }

    StringPtr++;

  }

  return EFI_SUCCESS;
}


/**
  This helper function is to be called by drivers to map configuration data
  stored in byte array ("block") formats such as UEFI Variables into current
  configuration strings.

  @param  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param  ConfigRequest          A null-terminated Unicode string in
                                 <ConfigRequest> format.
  @param  Block                  Array of bytes defining the block's configuration.
  @param  BlockSize              Length in bytes of Block.
  @param  Config                 Filled-in configuration string. String allocated
                                 by  the function. Returned only if call is
                                 successful. It is <ConfigResp> string format.
  @param  Progress               A pointer to a string filled in with the offset of
                                  the most recent & before the first failing
                                 name/value pair (or the beginning of the string if
                                 the failure is in the first name / value pair) or
                                 the terminating NULL if all was successful.

  @retval EFI_SUCCESS            The request succeeded. Progress points to the null
                                 terminator at the end of the ConfigRequest
                                 string.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to allocate Config.     Progress
                                 points to the first character of ConfigRequest.
  @retval EFI_INVALID_PARAMETER  Passing in a NULL for the ConfigRequest or
                                 Block parameter would result in this type of
                                 error. Progress points to the first character of
                                 ConfigRequest.
  @retval EFI_DEVICE_ERROR       Block not large enough. Progress undefined.
  @retval EFI_INVALID_PARAMETER  Encountered non <BlockName> formatted string.
                                     Block is left updated and Progress points at
                                 the "&" preceding the first non-<BlockName>.

**/
EFI_STATUS
EFIAPI
HiiBlockToConfig (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL  *This,
  IN  CONST EFI_STRING                       ConfigRequest,
  IN  CONST UINT8                            *Block,
  IN  CONST UINTN                            BlockSize,
  OUT EFI_STRING                             *Config,
  OUT EFI_STRING                             *Progress
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_STRING                          StringPtr;
  UINTN                               Length;
  EFI_STATUS                          Status;
  EFI_STRING                          TmpPtr;
  UINT8                               *TmpBuffer;
  UINTN                               Offset;
  UINTN                               Width;
  UINT8                               *Value;
  EFI_STRING                          ValueStr;
  EFI_STRING                          ConfigElement;
  UINTN                               Index;
  UINT8                               *TemBuffer;
  CHAR16                              *TemString;

  if (This == NULL || Progress == NULL || Config == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (Block == NULL || ConfigRequest == NULL) {
    *Progress = ConfigRequest;
    return EFI_INVALID_PARAMETER;
  }


  Private = CONFIG_ROUTING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (Private != NULL);

  StringPtr     = ConfigRequest;
  ValueStr      = NULL;
  Value         = NULL;
  ConfigElement = NULL;

  //
  // Allocate a fix length of memory to store Results. Reallocate memory for
  // Results if this fix length is insufficient.
  //
  *Config = (EFI_STRING) AllocateZeroPool (MAX_STRING_LENGTH);
  if (*Config == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Jump <ConfigHdr>
  //
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    *Progress = StringPtr;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  while (*StringPtr != 0 && StrnCmp (StringPtr, L"PATH=", StrLen (L"PATH=")) != 0) {
    StringPtr++;
  }
  if (*StringPtr == 0) {
    *Progress = StringPtr - 1;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  while (*StringPtr != L'&' && *StringPtr != 0) {
    StringPtr++;
  }
  if (*StringPtr == 0) {
    *Progress = StringPtr - 1;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  //
  // Skip '&'
  //
  StringPtr++;

  //
  // Copy <ConfigHdr> and an additional '&' to <ConfigResp>
  //
  Length = StringPtr - ConfigRequest;
  CopyMem (*Config, ConfigRequest, Length * sizeof (CHAR16));

  //
  // Parse each <RequestElement> if exists
  // Only <BlockName> format is supported by this help function.
  // <BlockName> ::= 'OFFSET='<Number>&'WIDTH='<Number>
  //
  while (*StringPtr != 0 && StrnCmp (StringPtr, L"OFFSET=", StrLen (L"OFFSET=")) == 0) {
    //
    // Back up the header of one <BlockName>
    //
    TmpPtr = StringPtr;

    StringPtr += StrLen (L"OFFSET=");
    //
    // Get Offset
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (Status == EFI_OUT_OF_RESOURCES) {
      *Progress = ConfigRequest;
      goto Exit;
    }
    Offset = 0;
    CopyMem (
      &Offset,
      TmpBuffer,
      (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
      );
    FreePool (TmpBuffer);

    StringPtr += Length;
    if (StrnCmp (StringPtr, L"&WIDTH=", StrLen (L"&WIDTH=")) != 0) {
      *Progress = StringPtr - Length - StrLen (L"OFFSET=") - 1;
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    StringPtr += StrLen (L"&WIDTH=");

    //
    // Get Width
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (Status == EFI_OUT_OF_RESOURCES) {
      *Progress = ConfigRequest;
      goto Exit;
    }
    Width = 0;
    CopyMem (
      &Width,
      TmpBuffer,
      (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
      );
    FreePool (TmpBuffer);

    StringPtr += Length;
    if (*StringPtr != 0 && *StringPtr != L'&') {
      *Progress = StringPtr - Length - StrLen (L"&WIDTH=");
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    //
    // Calculate Value and convert it to hex string.
    //
    if (Offset + Width > BlockSize) {
      *Progress = StringPtr;
      Status = EFI_DEVICE_ERROR;
      goto Exit;
    }

    Value = (UINT8 *) AllocateZeroPool (Width);
    if (Value == NULL) {
      *Progress = ConfigRequest;
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }

    CopyMem (Value, (UINT8 *) Block + Offset, Width);

    Length = Width * 2 + 1;
    ValueStr = (EFI_STRING) AllocateZeroPool (Length  * sizeof (CHAR16));
    if (ValueStr == NULL) {
      *Progress = ConfigRequest;
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    
    TemString = ValueStr;
    TemBuffer = Value + Width - 1;
    for (Index = 0; Index < Width; Index ++, TemBuffer --) {
      TemString += UnicodeValueToString (TemString, PREFIX_ZERO | RADIX_HEX, *TemBuffer, 2);
    }

    FreePool (Value);
    Value = NULL;

    //
    // Build a ConfigElement
    //
    Length += StringPtr - TmpPtr + 1 + StrLen (L"VALUE=");
    ConfigElement = (EFI_STRING) AllocateZeroPool (Length * sizeof (CHAR16));
    if (ConfigElement == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Exit;
    }
    CopyMem (ConfigElement, TmpPtr, (StringPtr - TmpPtr + 1) * sizeof (CHAR16));
    if (*StringPtr == 0) {
      *(ConfigElement + (StringPtr - TmpPtr)) = L'&';
    }
    *(ConfigElement + (StringPtr - TmpPtr) + 1) = 0;
    StrCat (ConfigElement, L"VALUE=");
    StrCat (ConfigElement, ValueStr);

    AppendToMultiString (Config, ConfigElement);

    FreePool (ConfigElement);
    FreePool (ValueStr);
    ConfigElement = NULL;
    ValueStr = NULL;

    //
    // If '\0', parsing is finished. Otherwise skip '&' to continue
    //
    if (*StringPtr == 0) {
      break;
    }
    AppendToMultiString (Config, L"&");
    StringPtr++;

  }

  if (*StringPtr != 0) {
    *Progress = StringPtr - 1;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  
  HiiToLower (*Config);
  *Progress = StringPtr;
  return EFI_SUCCESS;

Exit:
  if (*Config != NULL) {
  FreePool (*Config);
  *Config = NULL;
  }
  if (ValueStr != NULL) {
    FreePool (ValueStr);
  }
  if (Value != NULL) {
    FreePool (Value);
  }
  if (ConfigElement != NULL) {
    FreePool (ConfigElement);
  }

  return Status;

}


/**
  This helper function is to be called by drivers to map configuration strings
  to configurations stored in byte array ("block") formats such as UEFI Variables.

  @param  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param  ConfigResp             A null-terminated Unicode string in <ConfigResp>
                                 format. It can be ConfigAltResp format string.
  @param  Block                  A possibly null array of bytes representing the
                                 current  block. Only bytes referenced in the
                                 ConfigResp string  in the block are modified. If
                                 this parameter is null or if the *BlockSize
                                 parameter is (on input) shorter than required by
                                 the Configuration string, only the BlockSize
                                 parameter is updated and an appropriate status
                                 (see below)  is returned.
  @param  BlockSize              The length of the Block in units of UINT8.  On
                                 input, this is the size of the Block. On output,
                                 if successful, contains the index of the  last
                                 modified byte in the Block.
  @param  Progress               On return, points to an element of the ConfigResp
                                 string filled in with the offset of the most
                                 recent '&' before the first failing name / value
                                 pair (or  the beginning of the string if the
                                 failure is in the  first name / value pair) or the
                                 terminating NULL if all was successful.

  @retval EFI_SUCCESS            The request succeeded. Progress points to the null
                                 terminator at the end of the ConfigResp string.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to allocate Config.     Progress
                                 points to the first character of ConfigResp.
  @retval EFI_INVALID_PARAMETER  Passing in a NULL for the ConfigResp or
                                 Block parameter would result in this type of
                                 error. Progress points to the first character of
                                         ConfigResp.
  @retval EFI_INVALID_PARAMETER  Encountered non <BlockName> formatted name /
                                 value pair. Block is left updated and
                                 Progress points at the '&' preceding the first
                                 non-<BlockName>.

**/
EFI_STATUS
EFIAPI
HiiConfigToBlock (
  IN     CONST EFI_HII_CONFIG_ROUTING_PROTOCOL *This,
  IN     CONST EFI_STRING                      ConfigResp,
  IN OUT UINT8                                 *Block,
  IN OUT UINTN                                 *BlockSize,
  OUT    EFI_STRING                            *Progress
  )
{
  HII_DATABASE_PRIVATE_DATA           *Private;
  EFI_STRING                          StringPtr;
  UINTN                               Length;
  EFI_STATUS                          Status;
  UINT8                               *TmpBuffer;
  UINTN                               Offset;
  UINTN                               Width;
  UINT8                               *Value;
  UINTN                               BufferSize;

  if (This == NULL || BlockSize == NULL || Progress == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  if (ConfigResp == NULL || Block == NULL) {
    *Progress = ConfigResp;
    return EFI_INVALID_PARAMETER;
  }

  Private = CONFIG_ROUTING_DATABASE_PRIVATE_DATA_FROM_THIS (This);
  ASSERT (Private != NULL);

  StringPtr  = ConfigResp;
  BufferSize = *BlockSize;
  Value      = NULL;

  //
  // Jump <ConfigHdr>
  //
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    *Progress = StringPtr;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  while (*StringPtr != 0 && StrnCmp (StringPtr, L"PATH=", StrLen (L"PATH=")) != 0) {
    StringPtr++;
  }
  if (*StringPtr == 0) {
    *Progress = StringPtr;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  while (*StringPtr != L'&' && *StringPtr != 0) {
    StringPtr++;
  }
  if (*StringPtr == 0) {
    *Progress = StringPtr;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }
  //
  // Skip '&'
  //
  StringPtr++;

  //
  // Parse each <ConfigElement> if exists
  // Only <BlockConfig> format is supported by this help function.
  // <BlockConfig> ::= 'OFFSET='<Number>&'WIDTH='<Number>&'VALUE='<Number>
  //
  while (*StringPtr != 0 && StrnCmp (StringPtr, L"OFFSET=", StrLen (L"OFFSET=")) == 0) {
    StringPtr += StrLen (L"OFFSET=");
    //
    // Get Offset
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (EFI_ERROR (Status)) {
      *Progress = ConfigResp;
      goto Exit;
    }
    Offset = 0;
    CopyMem (
      &Offset,
      TmpBuffer,
      (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
      );
    FreePool (TmpBuffer);

    StringPtr += Length;
    if (StrnCmp (StringPtr, L"&WIDTH=", StrLen (L"&WIDTH=")) != 0) {
      *Progress = StringPtr - Length - StrLen (L"OFFSET=") - 1;
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    StringPtr += StrLen (L"&WIDTH=");

    //
    // Get Width
    //
    Status = GetValueOfNumber (StringPtr, &TmpBuffer, &Length);
    if (Status == EFI_OUT_OF_RESOURCES) {
      *Progress = ConfigResp;
      goto Exit;
    }
    Width = 0;
    CopyMem (
      &Width,
      TmpBuffer,
      (((Length + 1) / 2) < sizeof (UINTN)) ? ((Length + 1) / 2) : sizeof (UINTN)
      );
    FreePool (TmpBuffer);

    StringPtr += Length;
    if (StrnCmp (StringPtr, L"&VALUE=", StrLen (L"&VALUE=")) != 0) {
      *Progress = StringPtr - Length - StrLen (L"&WIDTH=");
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }
    StringPtr += StrLen (L"&VALUE=");

    //
    // Get Value
    //
    Status = GetValueOfNumber (StringPtr, &Value, &Length);
    if (EFI_ERROR (Status)) {
      *Progress = ConfigResp;
      goto Exit;
    }

    StringPtr += Length;
    if (*StringPtr != 0 && *StringPtr != L'&') {
      *Progress = StringPtr - Length - 7;
      Status = EFI_INVALID_PARAMETER;
      goto Exit;
    }

    //
    // Update the Block with configuration info
    //

    if (Offset + Width > BufferSize) {
      return EFI_DEVICE_ERROR;
    }

    CopyMem (Block + Offset, Value, Width);
    *BlockSize = Offset + Width - 1;

    FreePool (Value);
    Value = NULL;

    //
    // If '\0', parsing is finished. Otherwise skip '&' to continue
    //
    if (*StringPtr == 0) {
      break;
    }

    StringPtr++;
  }
  
  //
  // The input string is ConfigAltResp format.
  //
  if ((*StringPtr != 0) && (StrnCmp (StringPtr, L"&GUID=", StrLen (L"&GUID=")) != 0)) {
    *Progress = StringPtr - 1;
    Status = EFI_INVALID_PARAMETER;
    goto Exit;
  }

  *Progress = StringPtr;
  return EFI_SUCCESS;

Exit:

  if (Value != NULL) {
    FreePool (Value);
  }
  return Status;
}


/**
  This helper function is to be called by drivers to extract portions of
  a larger configuration string.

  @param  This                   A pointer to the EFI_HII_CONFIG_ROUTING_PROTOCOL
                                 instance.
  @param  Configuration          A null-terminated Unicode string in
                                 <MultiConfigAltResp> format. It is <ConfigAltResp> format.
  @param  Guid                   A pointer to the GUID value to search for in the
                                 routing portion of the ConfigResp string when
                                 retrieving  the requested data. If Guid is NULL,
                                 then all GUID  values will be searched for.
  @param  Name                   A pointer to the NAME value to search for in the
                                 routing portion of the ConfigResp string when
                                 retrieving  the requested data. If Name is NULL,
                                 then all Name  values will be searched for.
  @param  DevicePath             A pointer to the PATH value to search for in the
                                 routing portion of the ConfigResp string when
                                 retrieving  the requested data. If DevicePath is
                                 NULL, then all  DevicePath values will be searched
                                 for.
  @param  AltCfgId               A pointer to the ALTCFG value to search for in the
                                  routing portion of the ConfigResp string when
                                 retrieving  the requested data.  If this parameter
                                 is NULL,  then the current setting will be
                                 retrieved.
  @param  AltCfgResp             A pointer to a buffer which will be allocated by
                                 the  function which contains the retrieved string
                                 as requested.   This buffer is only allocated if
                                 the call was successful. It is <ConfigResp> format.

  @retval EFI_SUCCESS            The request succeeded. The requested data was
                                 extracted  and placed in the newly allocated
                                 AltCfgResp buffer.
  @retval EFI_OUT_OF_RESOURCES   Not enough memory to allocate AltCfgResp.
  @retval EFI_INVALID_PARAMETER  Any parameter is invalid.
  @retval EFI_NOT_FOUND          Target for the specified routing data was not
                                 found.

**/
EFI_STATUS
EFIAPI
HiiGetAltCfg (
  IN  CONST EFI_HII_CONFIG_ROUTING_PROTOCOL    *This,
  IN  CONST EFI_STRING                         Configuration,
  IN  CONST EFI_GUID                           *Guid,
  IN  CONST EFI_STRING                         Name,
  IN  CONST EFI_DEVICE_PATH_PROTOCOL           *DevicePath,
  IN  CONST UINT16                             *AltCfgId,
  OUT EFI_STRING                               *AltCfgResp
  )
{
  EFI_STATUS                          Status;
  EFI_STRING                          StringPtr;
  EFI_STRING                          HdrStart;
  EFI_STRING                          HdrEnd;
  EFI_STRING                          TmpPtr;
  UINTN                               Length;
  EFI_STRING                          GuidStr;
  EFI_STRING                          NameStr;
  EFI_STRING                          PathStr;
  EFI_STRING                          AltIdStr;
  EFI_STRING                          Result;
  BOOLEAN                             GuidFlag;
  BOOLEAN                             NameFlag;
  BOOLEAN                             PathFlag;

  HdrStart = NULL;
  HdrEnd   = NULL;
  GuidStr  = NULL;
  NameStr  = NULL;
  PathStr  = NULL;
  AltIdStr = NULL;
  Result   = NULL;
  GuidFlag = FALSE;
  NameFlag = FALSE;
  PathFlag = FALSE;

  if (This == NULL || Configuration == NULL || AltCfgResp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  StringPtr = Configuration;
  if (StrnCmp (StringPtr, L"GUID=", StrLen (L"GUID=")) != 0) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Generate the sub string for later matching.
  //
  GenerateSubStr (L"GUID=", sizeof (EFI_GUID), (VOID *) Guid, 1, &GuidStr);
  GenerateSubStr (
    L"PATH=",
    GetDevicePathSize ((EFI_DEVICE_PATH_PROTOCOL *) DevicePath),
    (VOID *) DevicePath,
    1,
    &PathStr
    );
  if (AltCfgId != NULL) {
    GenerateSubStr (L"ALTCFG=", sizeof (UINT16), (VOID *) AltCfgId, 3, &AltIdStr);  
  }
  if (Name != NULL) {
    GenerateSubStr (L"NAME=", StrLen (Name) * sizeof (CHAR16), (VOID *) Name, 2, &NameStr);    
  } else {
    GenerateSubStr (L"NAME=", 0, NULL, 2, &NameStr);
  }

  while (*StringPtr != 0) {
    //
    // Try to match the GUID
    //
    if (!GuidFlag) {
      TmpPtr = StrStr (StringPtr, GuidStr);
      if (TmpPtr == NULL) {
        Status = EFI_NOT_FOUND;
        goto Exit;
      }
      HdrStart = TmpPtr;

      //
      // Jump to <NameHdr>
      //
      if (Guid != NULL) {
        StringPtr = TmpPtr + StrLen (GuidStr);
      } else {
        StringPtr = StrStr (TmpPtr, L"NAME=");
        if (StringPtr == NULL) {
          Status = EFI_NOT_FOUND;
          goto Exit;
        }
      }
      GuidFlag = TRUE;
    }

    //
    // Try to match the NAME
    //
    if (GuidFlag && !NameFlag) {
      if (StrnCmp (StringPtr, NameStr, StrLen (NameStr)) != 0) {
        GuidFlag = FALSE;
      } else {
        //
        // Jump to <PathHdr>
        //
        if (Name != NULL) {
          StringPtr += StrLen (NameStr);
        } else {
          StringPtr = StrStr (StringPtr, L"PATH=");
          if (StringPtr == NULL) {
            Status = EFI_NOT_FOUND;
            goto Exit;
          }
        }
        NameFlag = TRUE;
      }
    }

    //
    // Try to match the DevicePath
    //
    if (GuidFlag && NameFlag && !PathFlag) {
      if (StrnCmp (StringPtr, PathStr, StrLen (PathStr)) != 0) {
        GuidFlag = FALSE;
        NameFlag = FALSE;
      } else {
        //
        // Jump to '&' before <DescHdr> or <ConfigBody>
        //
        if (DevicePath != NULL) {
          StringPtr += StrLen (PathStr);
        } else {
          StringPtr = StrStr (StringPtr, L"&");
          if (StringPtr == NULL) {
            Status = EFI_NOT_FOUND;
            goto Exit;
          }
          StringPtr ++;
        }
        PathFlag = TRUE;
        HdrEnd   = StringPtr;
      }
    }

    //
    // Try to match the AltCfgId
    //
    if (GuidFlag && NameFlag && PathFlag) {
      if (AltCfgId == NULL) {
        //
        // Return Current Setting when AltCfgId is NULL.
        //
        Status = OutputConfigBody (StringPtr, &Result);
        goto Exit;
      }
      //
      // Search the <ConfigAltResp> to get the <AltResp> with AltCfgId.
      //
      if (StrnCmp (StringPtr, AltIdStr, StrLen (AltIdStr)) != 0) {
        GuidFlag = FALSE;
        NameFlag = FALSE;
        PathFlag = FALSE;
      } else {
        //
        // Skip AltIdStr and &
        //
        StringPtr = StringPtr + StrLen (AltIdStr);
        Status    = OutputConfigBody (StringPtr, &Result);
        goto Exit;
      }
    }
  }

  Status = EFI_NOT_FOUND;

Exit:
  *AltCfgResp = NULL;
  if (!EFI_ERROR (Status) && (Result != NULL)) {
    //
    // Copy the <ConfigHdr> and <ConfigBody>
    //
    Length = HdrEnd - HdrStart + StrLen (Result) + 1;
    *AltCfgResp = AllocateZeroPool (Length * sizeof (CHAR16));
    if (*AltCfgResp == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      StrnCpy (*AltCfgResp, HdrStart, HdrEnd - HdrStart);
      StrCat (*AltCfgResp, Result);
      Status = EFI_SUCCESS;
    }
  }

  if (GuidStr != NULL) {
    FreePool (GuidStr);
  }
  if (NameStr != NULL) {
    FreePool (NameStr);
  }
  if (PathStr != NULL) {
    FreePool (PathStr);
  }
  if (AltIdStr != NULL) {
    FreePool (AltIdStr);
  }
  if (Result != NULL) {
    FreePool (Result);
  }

  return Status;

}


