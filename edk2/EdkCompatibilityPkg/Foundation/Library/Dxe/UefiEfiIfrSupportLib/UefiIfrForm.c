/*++

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

--*/

#include "UefiIfrLibrary.h"

//
// Fake <ConfigHdr>
//
UINT16 mFakeConfigHdr[] = L"GUID=00000000000000000000000000000000&NAME=0000&PATH=0";

STATIC
EFI_STATUS
GetPackageDataFromPackageList (
  IN  EFI_HII_PACKAGE_LIST_HEADER *HiiPackageList,
  IN  UINT32                      PackageIndex,
  OUT UINT32                      *BufferLen,
  OUT EFI_HII_PACKAGE_HEADER      **Buffer
  )
{
  UINT32                        Index;
  EFI_HII_PACKAGE_HEADER        *Package;
  UINT32                        Offset;
  UINT32                        PackageListLength;
  EFI_HII_PACKAGE_HEADER        PackageHeader = {0, 0};

  ASSERT(HiiPackageList != NULL);

  if ((BufferLen == NULL) || (Buffer == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Package = NULL;
  Index   = 0;
  Offset  = sizeof (EFI_HII_PACKAGE_LIST_HEADER);
  EfiCopyMem (&PackageListLength, &HiiPackageList->PackageLength, sizeof (UINT32));
  while (Offset < PackageListLength) {
    Package = (EFI_HII_PACKAGE_HEADER *) (((UINT8 *) HiiPackageList) + Offset);
    EfiCopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
    if (Index == PackageIndex) {
      break;
    }
    Offset += PackageHeader.Length;
    Index++;
  }
  if (Offset >= PackageListLength) {
    //
    // no package found in this Package List
    //
    return EFI_NOT_FOUND;
  }

  *BufferLen = PackageHeader.Length;
  *Buffer    = Package;
  return EFI_SUCCESS;
}

STATIC
EFI_STATUS
UpdateFormPackageData (
  IN  EFI_GUID               *FormSetGuid,
  IN  EFI_FORM_ID            FormId,
  IN  EFI_HII_PACKAGE_HEADER *Package,
  IN  UINT32                 PackageLength,
  IN  UINT16                 Label,
  IN  BOOLEAN                Insert,
  IN  EFI_HII_UPDATE_DATA    *Data,
  OUT UINT8                  **TempBuffer,
  OUT UINT32                 *TempBufferSize
  )
{
  UINTN                     AddSize;
  UINT8                     *BufferPos;
  EFI_HII_PACKAGE_HEADER    PackageHeader;
  UINTN                     Offset;
  EFI_IFR_OP_HEADER         *IfrOpHdr;
  BOOLEAN                   GetFormSet;
  BOOLEAN                   GetForm;
  UINT8                     ExtendOpCode;
  UINT16                    LabelNumber;
  BOOLEAN                   Updated;
  EFI_IFR_OP_HEADER         *AddOpCode;

  if ((TempBuffer == NULL) || (TempBufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  *TempBufferSize = PackageLength;
  if (Data != NULL) {
    *TempBufferSize += Data->Offset;
  }
  *TempBuffer = EfiLibAllocateZeroPool (*TempBufferSize);
  if (*TempBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  EfiCopyMem (*TempBuffer, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  *TempBufferSize = sizeof (EFI_HII_PACKAGE_HEADER);
  BufferPos = *TempBuffer + sizeof (EFI_HII_PACKAGE_HEADER);

  EfiCopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
  IfrOpHdr   = (EFI_IFR_OP_HEADER *)((UINT8 *) Package + sizeof (EFI_HII_PACKAGE_HEADER));
  Offset     = sizeof (EFI_HII_PACKAGE_HEADER);
  GetFormSet = (BOOLEAN)((FormSetGuid == NULL) ? TRUE : FALSE);
  GetForm    = FALSE;
  Updated    = FALSE;

  while (Offset < PackageHeader.Length) {
    EfiCopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
    BufferPos += IfrOpHdr->Length;
    *TempBufferSize += IfrOpHdr->Length;

    switch (IfrOpHdr->OpCode) {
    case EFI_IFR_FORM_SET_OP :
      if (FormSetGuid != NULL) {
        if (EfiCompareMem (&((EFI_IFR_FORM_SET *) IfrOpHdr)->Guid, FormSetGuid, sizeof (EFI_GUID)) == 0) {
          GetFormSet = TRUE;
        }
      }
      break;

    case EFI_IFR_FORM_OP:
      if (EfiCompareMem (&((EFI_IFR_FORM *) IfrOpHdr)->FormId, &FormId, sizeof (EFI_FORM_ID)) == 0) {
        GetForm = TRUE;
      }
      break;

    case EFI_IFR_GUID_OP :
      if (!GetFormSet || !GetForm || Updated) {
        //
        // Go to the next Op-Code
        //
        Offset   += IfrOpHdr->Length;
        IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
        continue;
      }

      ExtendOpCode = ((EFI_IFR_GUID_LABEL *) IfrOpHdr)->ExtendOpCode;
      EfiCopyMem (&LabelNumber, &((EFI_IFR_GUID_LABEL *)IfrOpHdr)->Number, sizeof (UINT16));
      if ((ExtendOpCode != EFI_IFR_EXTEND_OP_LABEL) || (LabelNumber != Label)) {
        //
        // Go to the next Op-Code
        //
        Offset   += IfrOpHdr->Length;
        IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
        continue;
      }

      if (Insert && (Data != NULL)) {
        //
        // insert the DataCount amount of opcodes to TempBuffer if Data is NULL remove
        // DataCount amount of opcodes unless runing into a label.
        //
        AddOpCode = (EFI_IFR_OP_HEADER *)Data->Data;
        AddSize   = 0;
        while (AddSize < Data->Offset) {
          EfiCopyMem (BufferPos, AddOpCode, AddOpCode->Length);
          BufferPos += AddOpCode->Length;
          *TempBufferSize += AddOpCode->Length;

          AddSize += AddOpCode->Length;
          AddOpCode = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (AddOpCode) + AddOpCode->Length);
        }
      } else {
        //
        // Search the next Label.
        //
        while (TRUE) {
          Offset   += IfrOpHdr->Length;
          //
          // Search the next label and Fail if not label found.
          //
          if (Offset >= PackageHeader.Length) {
            goto Fail;
          }
          IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
          if (IfrOpHdr->OpCode == EFI_IFR_GUID_OP) {
            ExtendOpCode = ((EFI_IFR_GUID_LABEL *) IfrOpHdr)->ExtendOpCode;
            if (ExtendOpCode == EFI_IFR_EXTEND_OP_LABEL) {
              break;
            }
          }
        }

        if (Data != NULL) {
          AddOpCode = (EFI_IFR_OP_HEADER *)Data->Data;
          AddSize   = 0;
          while (AddSize < Data->Offset) {
            EfiCopyMem (BufferPos, AddOpCode, AddOpCode->Length);
            BufferPos += AddOpCode->Length;
            *TempBufferSize += AddOpCode->Length;

            AddSize   += AddOpCode->Length;
            AddOpCode = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (AddOpCode) + AddOpCode->Length);
          }
        }

        //
        // copy the next label
        //
        EfiCopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
        BufferPos += IfrOpHdr->Length;
        *TempBufferSize += IfrOpHdr->Length;
      }

      Updated = TRUE;
      break;
    default :
      break;
    }

    //
    // Go to the next Op-Code
    //
    Offset   += IfrOpHdr->Length;
    IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
  }

  //
  // Update the package length.
  //
  PackageHeader.Length = *TempBufferSize;
  EfiCopyMem (*TempBuffer, &PackageHeader, sizeof (EFI_HII_PACKAGE_HEADER));

Fail:
  if (!Updated) {
    gBS->FreePool (*TempBuffer);
    *TempBufferSize = 0;
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}

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
                 If FALSE, replace opcodes between two labels with Data
  Data         - The adding data; If NULL, remove opcodes between two Label.

Returns:
  EFI_SUCCESS  - Update success.
  Other        - Update fail.

--*/
{
  EFI_STATUS                   Status;
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_HII_PACKAGE_LIST_HEADER  *HiiPackageList;
  UINT32                       Index;
  EFI_HII_PACKAGE_LIST_HEADER  *UpdateBuffer;
  UINTN                        BufferSize;
  UINT8                        *UpdateBufferPos;
  EFI_HII_PACKAGE_HEADER       PackageHeader;
  EFI_HII_PACKAGE_HEADER       *Package;
  UINT32                       PackageLength;
  EFI_HII_PACKAGE_HEADER       *TempBuffer;
  UINT32                       TempBufferSize;
  BOOLEAN                      Updated;

  if (Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  LocateHiiProtocols ();
  HiiDatabase = gIfrLibHiiDatabase;

  //
  // Get the orginal package list
  //
  BufferSize = 0;
  HiiPackageList   = NULL;
  Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HiiPackageList = EfiLibAllocatePool (BufferSize);
    ASSERT (HiiPackageList != NULL);

    Status = HiiDatabase->ExportPackageLists (HiiDatabase, Handle, &BufferSize, HiiPackageList);
    if (EFI_ERROR (Status)) {
      gBS->FreePool (HiiPackageList);
      return Status;
    }
  }

  //
  // Calculate and allocate space for retrieval of IFR data
  //
  BufferSize += Data->Offset;
  UpdateBuffer = EfiLibAllocateZeroPool (BufferSize);
  if (UpdateBuffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  UpdateBufferPos = (UINT8 *) UpdateBuffer;

  //
  // copy the package list header
  //
  EfiCopyMem (UpdateBufferPos, HiiPackageList, sizeof (EFI_HII_PACKAGE_LIST_HEADER));
  UpdateBufferPos += sizeof (EFI_HII_PACKAGE_LIST_HEADER);

  Updated = FALSE;
  for (Index = 0; ; Index++) {
    Status = GetPackageDataFromPackageList (HiiPackageList, Index, &PackageLength, &Package);
    if (Status == EFI_SUCCESS) {
      EfiCopyMem (&PackageHeader, Package, sizeof (EFI_HII_PACKAGE_HEADER));
      if ((PackageHeader.Type == EFI_HII_PACKAGE_FORM) && !Updated) {
        Status = UpdateFormPackageData (FormSetGuid, FormId, Package, PackageLength, Label, Insert, Data, (UINT8 **)&TempBuffer, &TempBufferSize);
        if (!EFI_ERROR(Status)) {
          if (FormSetGuid == NULL) {
            Updated = TRUE;
          }
          EfiCopyMem (UpdateBufferPos, TempBuffer, TempBufferSize);
          UpdateBufferPos += TempBufferSize;
          gBS->FreePool (TempBuffer);
          continue;
        }
      }

      EfiCopyMem (UpdateBufferPos, Package, PackageLength);
      UpdateBufferPos += PackageLength;
    } else if (Status == EFI_NOT_FOUND) {
      break;
    } else {
      gBS->FreePool (HiiPackageList);
      return Status;
    }
  }

  //
  // Update package list length
  //
  BufferSize = UpdateBufferPos - (UINT8 *) UpdateBuffer;
  EfiCopyMem (&UpdateBuffer->PackageLength, &BufferSize, sizeof (UINT32));

  gBS->FreePool (HiiPackageList);

  return HiiDatabase->UpdatePackageList (HiiDatabase, Handle, UpdateBuffer);
}

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
{
  UINTN                         Index;
  UINTN                         Count;
  UINTN                         Start;
  UINTN                         End;
  UINTN                         Top;
  UINTN                         Bottom;
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
  EFI_SIMPLE_TEXT_OUT_PROTOCOL  *ConOut;

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

  LineBuffer = EfiLibAllocateZeroPool (DimensionsWidth * sizeof (CHAR16));
  ASSERT (LineBuffer != NULL);

  //
  // Determine the largest string in the dialog box
  // Notice we are starting with 1 since String is the first string
  //
  StringArray = EfiLibAllocateZeroPool (NumberOfLines * sizeof (CHAR16 *));
  LargestString = EfiStrLen (String);
  StringArray[0] = String;

  VA_START (Marker, String);
  for (Index = 1; Index < NumberOfLines; Index++) {
    StackString = VA_ARG (Marker, CHAR16 *);

    if (StackString == NULL) {
      return EFI_INVALID_PARAMETER;
    }

    StringArray[Index] = StackString;
    StringLen = EfiStrLen (StackString);
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
  End       = Start + LargestString + 1;

  Top       = ((DimensionsHeight - NumberOfLines - 2) / 2) + TopRow - 1;
  Bottom    = Top + NumberOfLines + 2;

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

    StringLen = EfiStrLen (StringArray[Index]);
    if (StringLen > LargestString) {
      StringLen = LargestString;
    }
    EfiCopyMem (
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
    Status = gBS->CreateEvent (EFI_EVENT_TIMER, 0, NULL, NULL, &TimerEvent);

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
  EfiCopyMem (KeyValue, &Key, sizeof (EFI_INPUT_KEY));

  ConOut->SetAttribute (ConOut, CurrentAttribute);
  ConOut->EnableCursor (ConOut, TRUE);

  return Status;
}

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
{
  VA_LIST                         Args;
  UINTN                           Index;
  UINT32                          TotalLen;
  UINT8                           *BufCfgArray;
  UINT8                           *BufferPos;
  UINT16                          Offset;
  UINT16                          Width;
  UINT8                           *Value;

  if ((Buffer == NULL) || (BufferSize == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Offset = 0;
  Width  = 0;
  Value  = NULL;

  VA_START (Args, Number);
  for (Index = 0; Index < Number; Index++) {
    BufCfgArray = (UINT8 *) VA_ARG (Args, VOID *);
    EfiCopyMem (&TotalLen, BufCfgArray, sizeof (UINT32));
    BufferPos = BufCfgArray + sizeof (UINT32);

    while ((UINT32)(BufferPos - BufCfgArray) < TotalLen) {
      EfiCopyMem (&Offset, BufferPos, sizeof (UINT16));
      BufferPos += sizeof (UINT16);
      EfiCopyMem (&Width, BufferPos, sizeof (UINT16));
      BufferPos += sizeof (UINT16);
      Value = BufferPos;
      BufferPos += Width;

      if ((UINTN)(Offset + Width) > *BufferSize) {
        return EFI_BUFFER_TOO_SMALL;
      }

      EfiCopyMem ((UINT8 *)Buffer + Offset, Value, Width);
    }
  }
  VA_END (Args);

  *BufferSize = (UINTN)Offset;

  return EFI_SUCCESS;
}

STATIC
VOID
SwapBuffer (
  IN OUT UINT8     *Buffer,
  IN UINTN         BufferSize
  )
/*++

Routine Description:
  Swap bytes in the buffer.

Arguments:
  Buffer     -  Binary buffer.
  BufferSize -  Size of the buffer in bytes.

Returns:
  None.

--*/
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

EFI_STATUS
BufferToHexString (
  IN OUT CHAR16    *Str,
  IN UINT8         *Buffer,
  IN UINTN         BufferSize
  )
/*++

Routine Description:
  Converts binary buffer to Unicode string in reversed byte order from BufToHexString().

Arguments:
  Str        -  String for output
  Buffer     -  Binary buffer.
  BufferSize -  Size of the buffer in bytes.

Returns:
  EFI_SUCCESS  -  The function completed successfully.

--*/
{
  EFI_STATUS  Status;
  UINT8       *NewBuffer;
  UINTN       StrBufferLen;

  NewBuffer = EfiLibAllocateCopyPool (BufferSize, Buffer);
  SwapBuffer (NewBuffer, BufferSize);

  StrBufferLen = (BufferSize + 1) * sizeof (CHAR16);
  Status = BufToHexString (Str, &StrBufferLen, NewBuffer, BufferSize);

  gBS->FreePool (NewBuffer);

  return Status;
}

EFI_STATUS
HexStringToBuffer (
  IN OUT UINT8         *Buffer,
  IN OUT UINTN         *BufferSize,
  IN CHAR16            *Str
  )
/*++

Routine Description:
  Converts Hex String to binary buffer in reversed byte order from HexStringToBuf().

Arguments:
    Buffer     - Pointer to buffer that receives the data.
    BufferSize - Length in bytes of the buffer to hold converted data.
                 If routine return with EFI_SUCCESS, containing length of converted data.
                 If routine return with EFI_BUFFER_TOO_SMALL, containg length of buffer desired.
    Str        - String to be converted from.

Returns:
  EFI_SUCCESS    -  The function completed successfully.

--*/
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
  StrBufferLen - On input: Length in bytes of buffer to hold the ConfigHdr string.
                 Includes tailing '\0' character.
                 On output:
                    If return EFI_SUCCESS, containing length of ConfigHdr string buffer.
                    If return EFI_BUFFER_TOO_SMALL, containg length of string buffer desired.
  Guid         - Routing information: GUID.
  Name         - Routing information: NAME.
  DriverHandle - Driver handle which contains the routing information: PATH.

Returns:
  EFI_SUCCESS          - Routine success.
  EFI_BUFFER_TOO_SMALL - The ConfigHdr string buffer is too small.

--*/
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
    NameStrLen = EfiStrLen (Name);
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

  DevicePathSize = EfiDevicePathSize (DevicePath);

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

  EfiStrCpy (StrPtr, L"GUID=");
  StrPtr += 5;
  BufferToHexString (StrPtr, (UINT8 *) Guid, sizeof (EFI_GUID));
  StrPtr += 32;

  EfiStrCpy (StrPtr, L"&NAME=");
  StrPtr += 6;
  if (Name != NULL) {
    EfiStrCpy (StrPtr, Name);
    StrPtr += NameStrLen;
  }

  EfiStrCpy (StrPtr, L"&PATH=");
  StrPtr += 6;
  BufferToHexString (StrPtr, (UINT8 *) DevicePath, DevicePathSize);

  return EFI_SUCCESS;
}

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
{
  EFI_STATUS  Status;
  UINTN       Data;
  UINTN       BufferSize;
  UINTN       ConvertedStrLen;

  while ((String = EfiStrStr (String, L"&OFFSET=")) != NULL) {
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

    if (EfiStrnCmp (String, L"&WIDTH=", 7) != 0) {
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
  HeaderLen = EfiStrLen (ConfigHdr);

  BufferLen = 0x4000;
  ConfigResp = EfiLibAllocateZeroPool (BufferLen + HeaderLen);

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
    ConfigResp = EfiLibAllocateZeroPool (BufferLen + HeaderLen);

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
  EfiCopyMem (ConfigResp, ConfigHdr, HeaderLen * sizeof (UINT16));

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
  HeaderLen = EfiStrLen (ConfigHdr);

  if (RequestElement == NULL) {
    //
    // RequestElement not specified, use "&OFFSET=0&WIDTH=<BufferSize>" as <BlockName>
    //
    BlockName[0] = L'\0';
    EfiStrCpy (BlockName, L"&OFFSET=0&WIDTH=");

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

  BufferLen = HeaderLen * sizeof (CHAR16) + EfiStrSize (Request);
  ConfigRequest = EfiLibAllocateZeroPool (BufferLen);

  EfiCopyMem (ConfigRequest, ConfigHdr, HeaderLen * sizeof (CHAR16));
  StringPtr = ConfigRequest + HeaderLen;
  EfiStrCpy (StringPtr, Request);

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
