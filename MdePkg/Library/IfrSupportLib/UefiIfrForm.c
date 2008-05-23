/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UefiIfrForm.c

Abstract:

  Common Library Routines to assist handle HII elements.


**/

#include "UefiIfrLibraryInternal.h"

//
// Fake <ConfigHdr>
//
UINT16 mFakeConfigHdr[] = L"GUID=00000000000000000000000000000000&NAME=0000&PATH=0";

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
{
  UINTN                         Index;
  UINTN                         Count;
  UINTN                         Start;
  UINTN                         Top;
  CHAR16                        *StringPtr;
  UINTN                         LeftColumn;
  UINTN                         RightColumn;
  UINTN                         TopRow;
  UINTN                         BottomRow;
  UINTN                         DimensionsWidth;
  UINTN                         DimensionsHeight;
  VA_LIST                       Marker;
  EFI_INPUT_KEY                 Key;
  UINTN                         LargestString;
  CHAR16                        *StackString;
  EFI_STATUS                    Status;
  UINTN                         StringLen;
  CHAR16                        *LineBuffer;
  CHAR16                        **StringArray;
  EFI_EVENT                     TimerEvent;
  EFI_EVENT                     WaitList[2];
  UINTN                         CurrentAttribute;
  EFI_SIMPLE_TEXT_OUTPUT_PROTOCOL  *ConOut;

  if ((KeyValue == NULL) || (String == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  TopRow      = 0;
  BottomRow   = 0;
  LeftColumn  = 0;
  RightColumn = 0;

  ConOut = gST->ConOut;
  ConOut->QueryMode (ConOut, ConOut->Mode->Mode, &RightColumn, &BottomRow);

  DimensionsWidth  = RightColumn - LeftColumn;
  DimensionsHeight = BottomRow - TopRow;

  CurrentAttribute = ConOut->Mode->Attribute;

  LineBuffer = AllocateZeroPool (DimensionsWidth * sizeof (CHAR16));
  ASSERT (LineBuffer != NULL);

  //
  // Determine the largest string in the dialog box
  // Notice we are starting with 1 since String is the first string
  //
  StringArray = AllocateZeroPool (NumberOfLines * sizeof (CHAR16 *));
  LargestString = StrLen (String);
  StringArray[0] = String;

  VA_START (Marker, String);
  for (Index = 1; Index < NumberOfLines; Index++) {
    StackString = VA_ARG (Marker, CHAR16 *);

    if (StackString == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    StringArray[Index] = StackString;
    StringLen = StrLen (StackString);
    if (StringLen > LargestString) {
      LargestString = StringLen;
    }
  }

  if ((LargestString + 2) > DimensionsWidth) {
    LargestString = DimensionsWidth - 2;
  }

  //
  // Subtract the PopUp width from total Columns, allow for one space extra on
  // each end plus a border.
  //
  Start     = (DimensionsWidth - LargestString - 2) / 2 + LeftColumn + 1;

  Top       = ((DimensionsHeight - NumberOfLines - 2) / 2) + TopRow - 1;

  //
  // Disable cursor
  //
  ConOut->EnableCursor (ConOut, FALSE);
  ConOut->SetAttribute (ConOut, EFI_LIGHTGRAY | EFI_BACKGROUND_BLUE);

  StringPtr = &LineBuffer[0];
  *StringPtr++ = BOXDRAW_DOWN_RIGHT;
  for (Index = 0; Index < LargestString; Index++) {
    *StringPtr++ = BOXDRAW_HORIZONTAL;
  }
  *StringPtr++ = BOXDRAW_DOWN_LEFT;
  *StringPtr = L'\0';

  ConOut->SetCursorPosition (ConOut, Start, Top);
  ConOut->OutputString (ConOut, LineBuffer);

  for (Index = 0; Index < NumberOfLines; Index++) {
    StringPtr = &LineBuffer[0];
    *StringPtr++ = BOXDRAW_VERTICAL;

    for (Count = 0; Count < LargestString; Count++) {
      StringPtr[Count] = L' ';
    }

    StringLen = StrLen (StringArray[Index]);
    if (StringLen > LargestString) {
      StringLen = LargestString;
    }
    CopyMem (
      StringPtr + ((LargestString - StringLen) / 2),
      StringArray[Index],
      StringLen * sizeof (CHAR16)
      );
    StringPtr += LargestString;

    *StringPtr++ = BOXDRAW_VERTICAL;
    *StringPtr = L'\0';

    ConOut->SetCursorPosition (ConOut, Start, Top + 1 + Index);
    ConOut->OutputString (ConOut, LineBuffer);
  }

  StringPtr = &LineBuffer[0];
  *StringPtr++ = BOXDRAW_UP_RIGHT;
  for (Index = 0; Index < LargestString; Index++) {
    *StringPtr++ = BOXDRAW_HORIZONTAL;
  }
  *StringPtr++ = BOXDRAW_UP_LEFT;
  *StringPtr = L'\0';

  ConOut->SetCursorPosition (ConOut, Start, Top + NumberOfLines + 1);
  ConOut->OutputString (ConOut, LineBuffer);

  do {
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);

    //
    // Set a timer event of 1 second expiration
    //
    gBS->SetTimer (
          TimerEvent,
          TimerRelative,
          10000000
          );

    //
    // Wait for the keystroke event or the timer
    //
    WaitList[0] = gST->ConIn->WaitForKey;
    WaitList[1] = TimerEvent;
    Status      = gBS->WaitForEvent (2, WaitList, &Index);

    //
    // Check for the timer expiration
    //
    if (!EFI_ERROR (Status) && Index == 1) {
      Status = EFI_TIMEOUT;
    }

    gBS->CloseEvent (TimerEvent);
  } while (Status == EFI_TIMEOUT);

  Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);
  CopyMem (KeyValue, &Key, sizeof (EFI_INPUT_KEY));

  ConOut->SetAttribute (ConOut, CurrentAttribute);
  ConOut->EnableCursor (ConOut, TRUE);

  return Status;
}


/**
  Swap bytes in the buffer.

  @param  Buffer                 Binary buffer.
  @param  BufferSize             Size of the buffer in bytes.

  @return None.

**/
STATIC
VOID
SwapBuffer (
  IN OUT UINT8     *Buffer,
  IN UINTN         BufferSize
  )
{
  UINTN  Index;
  UINT8  Temp;
  UINTN  SwapCount;

  SwapCount = (BufferSize - 1) / 2;
  for (Index = 0; Index < SwapCount; Index++) {
    Temp = Buffer[Index];
    Buffer[Index] = Buffer[BufferSize - 1 - Index];
    Buffer[BufferSize - 1 - Index] = Temp;
  }
}


/**
  Converts binary buffer to Unicode string in reversed byte order from BufToHexString().

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
{
  EFI_STATUS  Status;
  UINT8       *NewBuffer;
  UINTN       StrBufferLen;

  NewBuffer = AllocateCopyPool (BufferSize, Buffer);
  SwapBuffer (NewBuffer, BufferSize);

  StrBufferLen = (BufferSize + 1) * sizeof (CHAR16);
  Status = BufToHexString (Str, &StrBufferLen, NewBuffer, BufferSize);

  gBS->FreePool (NewBuffer);

  return Status;
}


/**
  Converts Hex String to binary buffer in reversed byte order from HexStringToBuf().

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
{
  EFI_STATUS  Status;
  UINTN       ConvertedStrLen;

  ConvertedStrLen = 0;
  Status = HexStringToBuf (Buffer, BufferSize, Str, &ConvertedStrLen);
  if (!EFI_ERROR (Status)) {
    SwapBuffer (Buffer, ConvertedStrLen);
  }

  return Status;
}


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
{
  EFI_STATUS                Status;
  UINTN                     NameStrLen;
  UINTN                     DevicePathSize;
  UINTN                     BufferSize;
  CHAR16                    *StrPtr;
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;

  if (Name == NULL) {
    //
    // There will be no "NAME" in <ConfigHdr> for  Name/Value storage
    //
    NameStrLen = 0;
  } else {
    //
    // For buffer storage
    //
    NameStrLen = StrLen (Name);
  }

  //
  // Retrieve DevicePath Protocol associated with this HiiPackageList
  //
  Status = gBS->HandleProtocol (
                  DriverHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID **) &DevicePath
                  );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  DevicePathSize = GetDevicePathSize (DevicePath);

  //
  // GUID=<HexCh>32&NAME=<Alpha>NameStrLen&PATH=<HexChar>DevicePathStrLen <NULL>
  // | 5  |   32   |  6  |   NameStrLen   |  6  |    DevicePathStrLen   |
  //
  BufferSize = (5 + 32 + 6 + NameStrLen + 6 + DevicePathSize * 2 + 1) * sizeof (CHAR16);
  if (*StrBufferLen < BufferSize) {
    *StrBufferLen = BufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StrBufferLen = BufferSize;

  StrPtr = ConfigHdr;

  StrCpy (StrPtr, L"GUID=");
  StrPtr += 5;
  BufferToHexString (StrPtr, (UINT8 *) Guid, sizeof (EFI_GUID));
  StrPtr += 32;

  StrCpy (StrPtr, L"&NAME=");
  StrPtr += 6;
  if (Name != NULL) {
    StrCpy (StrPtr, Name);
    StrPtr += NameStrLen;
  }

  StrCpy (StrPtr, L"&PATH=");
  StrPtr += 6;
  BufferToHexString (StrPtr, (UINT8 *) DevicePath, DevicePathSize);

  return EFI_SUCCESS;
}


/**
  Search BlockName "&OFFSET=Offset&WIDTH=Width" in a string.

  @param  String                 The string to be searched in.
  @param  Offset                 Offset in BlockName.
  @param  Width                  Width in BlockName.

  @retval TRUE                   Block name found.
  @retval FALSE                  Block name not found.

**/
BOOLEAN
FindBlockName (
  IN OUT CHAR16                *String,
  UINTN                        Offset,
  UINTN                        Width
  )
{
  EFI_STATUS  Status;
  UINTN       Data;
  UINTN       BufferSize;
  UINTN       ConvertedStrLen;

  while ((String = StrStr (String, L"&OFFSET=")) != NULL) {
    //
    // Skip '&OFFSET='
    //
    String = String + 8;

    Data = 0;
    BufferSize = sizeof (UINTN);
    Status = HexStringToBuf ((UINT8 *) &Data, &BufferSize, String, &ConvertedStrLen);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
    String = String + ConvertedStrLen;

    if (Data != Offset) {
      continue;
    }

    if (StrnCmp (String, L"&WIDTH=", 7) != 0) {
      return FALSE;
    }
    String = String + 7;

    Data = 0;
    BufferSize = sizeof (UINTN);
    Status = HexStringToBuf ((UINT8 *) &Data, &BufferSize, String, &ConvertedStrLen);
    if (EFI_ERROR (Status)) {
      return FALSE;
    }
    if (Data == Width) {
      return TRUE;
    }

    String = String + ConvertedStrLen;
  }

  return FALSE;
}


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
{
  EFI_STATUS                      Status;
  CHAR16                          *ConfigHdr;
  CHAR16                          *ConfigResp;
  CHAR16                          *StringPtr;
  UINTN                           HeaderLen;
  UINTN                           BufferLen;
  CHAR16                          *Progress;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  //
  // Locate protocols for use
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Retrive formset storage data from Form Browser
  //
  ConfigHdr = mFakeConfigHdr;
  HeaderLen = StrLen (ConfigHdr);

  BufferLen = 0x4000;
  ConfigResp = AllocateZeroPool (BufferLen + HeaderLen);

  StringPtr = ConfigResp + HeaderLen;
  *StringPtr = L'&';
  StringPtr++;

  Status = FormBrowser2->BrowserCallback (
                           FormBrowser2,
                           &BufferLen,
                           StringPtr,
                           TRUE,
                           VariableGuid,
                           VariableName
                           );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    gBS->FreePool (ConfigResp);
    ConfigResp = AllocateZeroPool (BufferLen + HeaderLen);

    StringPtr = ConfigResp + HeaderLen;
    *StringPtr = L'&';
    StringPtr++;

    Status = FormBrowser2->BrowserCallback (
                             FormBrowser2,
                             &BufferLen,
                             StringPtr,
                             TRUE,
                             VariableGuid,
                             VariableName
                             );
  }
  if (EFI_ERROR (Status)) {
    gBS->FreePool (ConfigResp);
    return Status;
  }
  CopyMem (ConfigResp, ConfigHdr, HeaderLen * sizeof (UINT16));

  //
  // Convert <ConfigResp> to buffer data
  //
  Status = HiiConfigRouting->ConfigToBlock (
                               HiiConfigRouting,
                               ConfigResp,
                               Buffer,
                               BufferSize,
                               &Progress
                               );
  gBS->FreePool (ConfigResp);

  return Status;
}


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
{
  EFI_STATUS                      Status;
  CHAR16                          *ConfigHdr;
  CHAR16                          *ConfigResp;
  CHAR16                          *StringPtr;
  UINTN                           HeaderLen;
  UINTN                           BufferLen;
  CHAR16                          *Progress;
  EFI_FORM_BROWSER2_PROTOCOL      *FormBrowser2;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;
  CHAR16                          BlockName[33];
  CHAR16                          *ConfigRequest;
  CHAR16                          *Request;

  //
  // Locate protocols for use
  //
  Status = gBS->LocateProtocol (&gEfiFormBrowser2ProtocolGuid, NULL, (VOID **) &FormBrowser2);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Prepare <ConfigRequest>
  //
  ConfigHdr = mFakeConfigHdr;
  HeaderLen = StrLen (ConfigHdr);

  if (RequestElement == NULL) {
    //
    // RequestElement not specified, use "&OFFSET=0&WIDTH=<BufferSize>" as <BlockName>
    //
    BlockName[0] = L'\0';
    StrCpy (BlockName, L"&OFFSET=0&WIDTH=");

    //
    // String lenghth of L"&OFFSET=0&WIDTH=" is 16
    //
    StringPtr = BlockName + 16;
    BufferLen = sizeof (BlockName) - (16 * sizeof (CHAR16));
    BufToHexString (StringPtr, &BufferLen, (UINT8 *) &BufferSize, sizeof (UINTN));

    Request = BlockName;
  } else {
    Request = RequestElement;
  }

  BufferLen = HeaderLen * sizeof (CHAR16) + StrSize (Request);
  ConfigRequest = AllocateZeroPool (BufferLen);

  CopyMem (ConfigRequest, ConfigHdr, HeaderLen * sizeof (CHAR16));
  StringPtr = ConfigRequest + HeaderLen;
  StrCpy (StringPtr, Request);

  //
  // Convert buffer to <ConfigResp>
  //
  Status = HiiConfigRouting->BlockToConfig (
                                HiiConfigRouting,
                                ConfigRequest,
                                Buffer,
                                BufferSize,
                                &ConfigResp,
                                &Progress
                                );
  if (EFI_ERROR (Status)) {
    gBS->FreePool (ConfigResp);
    return Status;
  }

  //
  // Skip <ConfigHdr> and '&'
  //
  StringPtr = ConfigResp + HeaderLen + 1;

  //
  // Change uncommitted data in Browser
  //
  Status = FormBrowser2->BrowserCallback (
                           FormBrowser2,
                           &BufferSize,
                           StringPtr,
                           FALSE,
                           NULL,
                           NULL
                           );
  gBS->FreePool (ConfigResp);
  return Status;
}
