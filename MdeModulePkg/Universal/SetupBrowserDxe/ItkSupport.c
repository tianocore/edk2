/** @file
Utility functions for UI presentation.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <ItkSupport.h>

ITK_DRIVER_POST_PROTOCOL          *mItkDriverPostProtocol;
BOOLEAN                           mGetCustomizedDefaultOverrides = FALSE;
VOID                              *mMultiConfigRespBuffer = NULL;
UINTN                             mMultiConfigRespBufferSize = 0;

/**
  Reset Question to its default value from ITK.

  @param  FormSet                The form set.
  @param  Form                   The form.
  @param  Question               The question.
  @param  DefaultId              The Class of the default.

  @retval EFI_SUCCESS            Question is reset to default value.

**/
EFI_STATUS
GetDefaultValueFromITK (
  IN FORM_BROWSER_FORMSET             *FormSet,
  IN FORM_BROWSER_STATEMENT           *Question,
  IN UINT16                           DefaultId
  )
{
  VOID                *TempRespBuffer = NULL;
  BROWSER_STORAGE     *Storage;
  EFI_STATUS          Status;
  EFI_STRING          GuidStr;
  EFI_STRING          NameStr;
  EFI_STRING          OffsetStr;
  EFI_STRING          WidthStr;
  EFI_STRING          TmpPtr;
  CHAR16              *Value;
  CHAR16              *StringPtr;
  UINTN               LengthStr;
  UINT8               *Dst;
  CHAR16              TemStr[5];
  UINTN               Index;
  UINT8               DigitUint8;
  //
  // Get the customer settings from ITK
  //
  if (!mGetCustomizedDefaultOverrides) {
    mGetCustomizedDefaultOverrides = TRUE;
    //
    // Locate ITK 5.0 post protocol
    //
    Status = gBS->LocateProtocol (
                    &gItk50PostProtocolGuid,
                    NULL,
                    (VOID **) &mItkDriverPostProtocol
                    );

    if (EFI_ERROR (Status)) {
      return EFI_NOT_FOUND;
    }

    Status = mItkDriverPostProtocol->GetCustomizedDefaultOverrides (
                                       (UINT16*) &mMultiConfigRespBufferSize,
                                       mMultiConfigRespBuffer
                                       );

    if (Status == EFI_BUFFER_TOO_SMALL) {
      // Allocate Buffer
      Status = gBS->AllocatePool (
                      EfiBootServicesData,
                      mMultiConfigRespBufferSize,
                      (VOID **) &mMultiConfigRespBuffer
                      );

      if (EFI_ERROR (Status)) {
        return EFI_NOT_FOUND;
      }

      Status = mItkDriverPostProtocol->GetCustomizedDefaultOverrides (
                                         (UINT16*) &mMultiConfigRespBufferSize,
                                         mMultiConfigRespBuffer
                                         );
    } else {
      return EFI_NOT_FOUND;
    }
  }

  if (!mMultiConfigRespBuffer) {
    return EFI_NOT_FOUND;
  }

  // Allocate buffer
  TempRespBuffer = AllocateCopyPool (mMultiConfigRespBufferSize, mMultiConfigRespBuffer);
  if (!TempRespBuffer) {
    return EFI_NOT_FOUND;
  }

  Storage       = Question->Storage;
  if ((Storage == NULL) ||
    (Storage->Type == EFI_HII_VARSTORE_NAME_VALUE) ||
    (Storage->Type == EFI_HII_VARSTORE_EFI_VARIABLE))  {
    FreePool (TempRespBuffer);
    return EFI_NOT_FOUND;
  }

  //
  // Question Value is provided by Buffer Storage or NameValue Storage
  //
  if (Question->BufferValue != NULL) {
    //
    // This Question is password or orderedlist
    //
    Dst = Question->BufferValue;
  } else {
    //
    // Other type of Questions
    //
    Dst = (UINT8 *) &Question->HiiValue.Value;
  }

  //
  // Generate the sub string for later matching.
  //
  GenerateSubStr (L"GUID=", sizeof (EFI_GUID), (VOID *) &Storage->Guid, 1, &GuidStr);
  GenerateSubStr (L"NAME=", StrLen (Storage->Name) * sizeof (CHAR16), (VOID *) Storage->Name, 2, &NameStr);
  GenerateSubStr (L"OFFSET=", sizeof (UINT16), (VOID *) &Question->VarStoreInfo.VarOffset, 3, &OffsetStr);
  GenerateSubStr (L"WIDTH=", sizeof (UINT16), (VOID *) &Question->StorageWidth, 3, &WidthStr);

  StringPtr = TempRespBuffer;
  Status = EFI_NOT_FOUND;

  while (*StringPtr != 0) {
    //
    // Try to match the GUID
    //
    TmpPtr = StrStr (StringPtr, GuidStr);
    if (TmpPtr == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }

    //
    // Try to match the NAME
    //
    StringPtr = TmpPtr + StrLen (GuidStr);
    if (StrnCmp (StringPtr, NameStr, StrLen (NameStr)) != 0) {
      continue;
    }

    //
    // Skip <PathHdr>
    //
    StringPtr = StringPtr + StrLen (NameStr);
    if (StrnCmp (StringPtr, L"PATH=", StrLen (L"Path=")) != 0) {
      continue;
    }

    while (*StringPtr != L'\0' && *StringPtr != L'&') {
      StringPtr++;
    }

    //
    // Skip &
    //
    StringPtr++;

    //
    // Try to match the OFFSET
    //
    if (StrnCmp (StringPtr, OffsetStr, StrLen (OffsetStr)) != 0) {
      continue;
    }

    //
    // Try to match the WIDTH
    //
    StringPtr = StringPtr + StrLen (OffsetStr);
    if (StrnCmp (StringPtr, WidthStr, StrLen (WidthStr)) != 0) {
      continue;
    }

    //
    // Get the VALUE
    //
    StringPtr = StringPtr + StrLen (WidthStr);
    if (StrnCmp (StringPtr, L"VALUE=", StrLen (L"VALUE=")) != 0) {
      continue;
    } else {
      //
      // Skip "&VALUE"
      //
      StringPtr = StringPtr + 6;
      Value = StringPtr;
      while (*StringPtr != L'\0' && *StringPtr != L'&') {
        StringPtr++;
      }
      *StringPtr = L'\0';

      LengthStr = StrLen (Value);
      ZeroMem (TemStr, sizeof (TemStr));
      for (Index = 0; Index < LengthStr; Index ++) {
        TemStr[0] = Value[LengthStr - Index - 1];
        DigitUint8 = (UINT8) StrHexToUint64 (TemStr);
        if ((Index & 1) == 0) {
          Dst [Index/2] = DigitUint8;
        } else {
          Dst [Index/2] = (UINT8) ((DigitUint8 << 4) + Dst [Index/2]);
        }
      }
      Status = EFI_SUCCESS;
      break;
    }
  }

Done:
  FreePool (TempRespBuffer);
  FreePool (GuidStr);
  FreePool (OffsetStr);
  FreePool (WidthStr);

  return Status;
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

  StrCpyS (Str, Length, String);

  StringHeader = Str + StrLen (String);
  TemString    = (CHAR16 *) StringHeader;

  switch (Flag) {
  case 1:
    //
    // Convert Buffer to Hex String in reverse order
    //
    TemBuffer = ((UINT8 *) Buffer);
    for (Index = 0; Index < BufferLen; Index ++, TemBuffer ++) {
      UnicodeValueToStringS (
        TemString,
        sizeof (CHAR16) * (Length - StrnLenS (Str, Length)),
        PREFIX_ZERO | RADIX_HEX,
        *TemBuffer,
        2
        );
      TemString += StrnLenS (TemString, Length - StrnLenS (Str, Length));
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
      UnicodeValueToStringS (
        TemString,
        sizeof (CHAR16) * (Length - StrnLenS (Str, Length)),
        PREFIX_ZERO | RADIX_HEX,
        *TemName,
        4
        );
      TemString += StrnLenS (TemString, Length - StrnLenS (Str, Length));
    }
    break;
  case 3:
    //
    // Convert Buffer to Hex String
    //
    TemBuffer = ((UINT8 *) Buffer) + BufferLen - 1;
    for (Index = 0; Index < BufferLen; Index ++, TemBuffer --) {
      UnicodeValueToStringS (
        TemString,
        sizeof (CHAR16) * (Length - StrnLenS (Str, Length)),
        PREFIX_ZERO | RADIX_HEX,
        *TemBuffer,
        2
        );
      TemString += StrnLenS (TemString, Length - StrnLenS (Str, Length));
    }
    break;
  default:
    break;
  }

  //
  // Convert the uppercase to lowercase since <HexAf> is defined in lowercase format.
  //
  StrCatS (Str, Length, L"&");
  HiiToLower (Str);

  *SubStr = Str;
}

/**
  Converts the unicode character of the string from uppercase to lowercase.
  This is a internal function.

  @param ConfigString  String to be converted

**/
// VOID
// EFIAPI
// HiiToLower (
//   IN EFI_STRING  ConfigString
//   )
// {
//   EFI_STRING  String;
//   BOOLEAN     Lower;

//   ASSERT (ConfigString != NULL);

//   //
//   // Convert all hex digits in range [A-F] in the configuration header to [a-f]
//   //
//   for (String = ConfigString, Lower = FALSE; *String != L'\0'; String++) {
//     if (*String == L'=') {
//       Lower = TRUE;
//     } else if (*String == L'&') {
//       Lower = FALSE;
//     } else if (Lower && *String >= L'A' && *String <= L'F') {
//       *String = (CHAR16) (*String - L'A' + L'a');
//     }
//   }

//   return;
// }
