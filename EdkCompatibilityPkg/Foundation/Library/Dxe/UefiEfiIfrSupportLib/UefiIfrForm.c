/*++

Copyright (c) 2007 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
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
  UINT8                     *BufferPos;
  EFI_HII_PACKAGE_HEADER    PackageHeader;
  UINT32                    Offset;
  EFI_IFR_OP_HEADER         *IfrOpHdr;
  BOOLEAN                   GetFormSet;
  BOOLEAN                   GetForm;
  UINT8                     ExtendOpCode;
  UINT16                    LabelNumber;
  BOOLEAN                   Updated;

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

  while (!Updated && Offset < PackageHeader.Length) {
    EfiCopyMem (BufferPos, IfrOpHdr, IfrOpHdr->Length);
    BufferPos += IfrOpHdr->Length;
    *TempBufferSize += IfrOpHdr->Length;

    switch (IfrOpHdr->OpCode) {
    case EFI_IFR_FORM_SET_OP :
      if (FormSetGuid != NULL) {
        if (EfiCompareMem (&((EFI_IFR_FORM_SET *) IfrOpHdr)->Guid, FormSetGuid, sizeof (EFI_GUID)) == 0) {
          GetFormSet = TRUE;
        } else {
          GetFormSet = FALSE;
        }
      }
      break;

    case EFI_IFR_FORM_OP:
      if (EfiCompareMem (&((EFI_IFR_FORM *) IfrOpHdr)->FormId, &FormId, sizeof (EFI_FORM_ID)) == 0) {
        GetForm = TRUE;
      } else {
        GetForm = FALSE;
      }
      break;

    case EFI_IFR_GUID_OP :
      if (!GetFormSet || !GetForm) {
        //
        // Go to the next Op-Code
        //
        break;
      }

      if (!EfiCompareGuid (&((EFI_IFR_GUID *) IfrOpHdr)->Guid, &mIfrVendorGuid)) {
        //
        // GUID mismatch, skip this op-code
        //
        break;
      }

      ExtendOpCode = ((EFI_IFR_GUID_LABEL *) IfrOpHdr)->ExtendOpCode;
      EfiCopyMem (&LabelNumber, &((EFI_IFR_GUID_LABEL *)IfrOpHdr)->Number, sizeof (UINT16));
      if ((ExtendOpCode != EFI_IFR_EXTEND_OP_LABEL) || (LabelNumber != Label)) {
        //
        // Go to the next Op-Code
        //
        break;
      }

      if (Insert) {
        //
        // Insert data after current Label, skip myself
        //
        Offset   += IfrOpHdr->Length;
        IfrOpHdr = (EFI_IFR_OP_HEADER *) ((CHAR8 *) (IfrOpHdr) + IfrOpHdr->Length);
      } else {
        //
        // Replace data between two paired Label, try to find the next Label.
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
            if (EfiCompareGuid (&((EFI_IFR_GUID *) IfrOpHdr)->Guid, &mIfrVendorGuid) && ExtendOpCode == EFI_IFR_EXTEND_OP_LABEL) {
              break;
            }
          }
        }
      }

      //
      // Fill in the update data
      //
      if (Data != NULL) {
        EfiCopyMem (BufferPos, Data->Data, Data->Offset);
        BufferPos += Data->Offset;
        *TempBufferSize += Data->Offset;
      }

      //
      // Copy the reset data
      //
      EfiCopyMem (BufferPos, IfrOpHdr, PackageHeader.Length - Offset);
      *TempBufferSize += PackageHeader.Length - Offset;

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
IfrLibInitUpdateData (
  IN OUT EFI_HII_UPDATE_DATA   *UpdateData,
  IN UINT32                    BufferSize
  )
/*++

Routine Description:
  This function initialize the data structure for dynamic opcode.

Arguments:
  UpdateData     - The adding data;
  BufferSize     - Length of the buffer to fill dynamic opcodes.

Returns:
  EFI_SUCCESS           - Update data is initialized.
  EFI_INVALID_PARAMETER - UpdateData is NULL.
  EFI_OUT_OF_RESOURCES  - No enough memory to allocate.

--*/
{
  if (UpdateData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  UpdateData->BufferSize = BufferSize;
  UpdateData->Offset = 0;
  UpdateData->Data = EfiLibAllocatePool (BufferSize);

  return (UpdateData->Data != NULL) ? EFI_SUCCESS : EFI_OUT_OF_RESOURCES;
}

EFI_STATUS
IfrLibFreeUpdateData (
  IN EFI_HII_UPDATE_DATA       *UpdateData
  )
/*++

Routine Description:
  This function free the resource of update data.

Arguments:
  UpdateData     - The adding data;

Returns:
  EFI_SUCCESS           - Resource in UpdateData is released.
  EFI_INVALID_PARAMETER - UpdateData is NULL.

--*/
{
  EFI_STATUS  Status;

  if (UpdateData == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = gBS->FreePool (UpdateData->Data);
  UpdateData->Data = NULL;

  return Status;
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
      if ((PackageHeader.Type == EFI_HII_PACKAGE_FORMS) && !Updated) {
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
      VA_END (Marker);
      return EFI_INVALID_PARAMETER;
    }

    StringArray[Index] = StackString;
    StringLen = EfiStrLen (StackString);
    if (StringLen > LargestString) {
      LargestString = StringLen;
    }
  }
  VA_END (Marker);

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
        VA_END (Args);
        return EFI_BUFFER_TOO_SMALL;
      }

      EfiCopyMem ((UINT8 *)Buffer + Offset, Value, Width);
    }
  }
  VA_END (Args);

  *BufferSize = (UINTN)Offset;

  return EFI_SUCCESS;
}

EFI_STATUS
ExtractBlockName (
  IN UINT8                        *Buffer,
  OUT CHAR16                      **BlockName
  )
/*++

  Routine Description:

    Extract block name from the array generated by VFR compiler. The name of
  this array is "Vfr + <StorageName> + BlockName", e.g. "VfrMyIfrNVDataBlockName".
  Format of this array is:
     Array length | 4-bytes
       Offset     | 2-bytes
       Width      | 2-bytes
       Offset     | 2-bytes
       Width      | 2-bytes
       ... ...

  Arguments:
    Buffer                - Array generated by VFR compiler.
    BlockName             - The returned <BlockName>

  Returns:
    EFI_OUT_OF_RESOURCES  - Run out of memory resource.
    EFI_INVALID_PARAMETER - Buffer is NULL or BlockName is NULL.
    EFI_SUCCESS           - Operation successful.

--*/
{
  UINTN       Index;
  UINT32      Length;
  UINT32      BlockNameNumber;
  UINTN       HexStringBufferLen;
  CHAR16      *StringPtr;

  if ((Buffer == NULL) || (BlockName == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate number of Offset/Width pair
  //
  EfiCopyMem (&Length, Buffer, sizeof (UINT32));
  BlockNameNumber = (Length - sizeof (UINT32)) / (sizeof (UINT16) * 2);

  //
  // <BlockName> ::= &OFFSET=1234&WIDTH=1234
  //                 |   8  | 4 |  7   | 4 |
  //
  StringPtr = EfiLibAllocateZeroPool ((BlockNameNumber * (8 + 4 + 7 + 4) + 1) * sizeof (CHAR16));
  *BlockName = StringPtr;
  if (StringPtr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Buffer += sizeof (UINT32);
  for (Index = 0; Index < BlockNameNumber; Index++) {
    EfiStrCpy (StringPtr, L"&OFFSET=");
    StringPtr += 8;

    HexStringBufferLen = 5;
    BufToHexString (StringPtr, &HexStringBufferLen, Buffer, sizeof (UINT16));
    Buffer += sizeof (UINT16);
    StringPtr += 4;

    EfiStrCpy (StringPtr, L"&WIDTH=");
    StringPtr += 7;

    HexStringBufferLen = 5;
    BufToHexString (StringPtr, &HexStringBufferLen, Buffer, sizeof (UINT16));
    Buffer += sizeof (UINT16);
    StringPtr += 4;
  }

  return EFI_SUCCESS;
}

EFI_STATUS
ExtractBlockConfig (
  IN UINT8                        *Buffer,
  OUT CHAR16                      **BlockConfig
  )
/*++

  Routine Description:

    Extract block config from the array generated by VFR compiler. The name of
  this array is "Vfr + <StorageName> + Default<HexCh>4", e.g. "VfrMyIfrNVDataDefault0000".

  Arguments:
    Buffer                - Array generated by VFR compiler.
    BlockConfig           - The returned <BlockConfig>

  Returns:
    EFI_OUT_OF_RESOURCES  - Run out of memory resource.
    EFI_INVALID_PARAMETER - Buffer is NULL or BlockConfig is NULL.
    EFI_SUCCESS           - Operation successful.

--*/
{
  UINT32      Length;
  UINT16      Width;
  UINTN       HexStringBufferLen;
  CHAR16      *StringPtr;
  UINT8       *BufferEnd;
  CHAR16      *StringEnd;
  EFI_STATUS  Status;

  if ((Buffer == NULL) || (BlockConfig == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Calculate length of AltResp string
  // Format of Default value array is:
  //  Array length | 4-bytes
  //        Offset | 2-bytes
  //        Width  | 2-bytes
  //        Value  | Variable length
  //        Offset | 2-bytes
  //        Width  | 2-bytes
  //        Value  | Variable length
  //        ... ...
  // When value is 1 byte in length, overhead of AltResp string will be maximum,
  //  BlockConfig ::= <&OFFSET=1234&WIDTH=1234&VALUE=12>+
  //                   |   8   | 4  |  7   | 4 |  7  |2|
  // so the maximum length of BlockConfig could be calculated as:
  // (ArrayLength / 5) * (8 + 4 + 7 + 4 + 7 + 2) = ArrayLength * 6.4 < ArrayLength * 7
  //
  EfiCopyMem (&Length, Buffer, sizeof (UINT32));
  BufferEnd = Buffer + Length;
  StringPtr = EfiLibAllocatePool (Length * 7 * sizeof (CHAR16));
  *BlockConfig = StringPtr;
  if (StringPtr == NULL) {
      return EFI_OUT_OF_RESOURCES;
  }
  StringEnd = StringPtr + (Length * 7);

  Buffer += sizeof (UINT32);
  while (Buffer < BufferEnd) {
    EfiStrCpy (StringPtr, L"&OFFSET=");
    StringPtr += 8;

    HexStringBufferLen = 5;
    BufToHexString (StringPtr, &HexStringBufferLen, Buffer, sizeof (UINT16));
    Buffer += sizeof (UINT16);
    StringPtr += 4;

    EfiStrCpy (StringPtr, L"&WIDTH=");
    StringPtr += 7;

    HexStringBufferLen = 5;
    BufToHexString (StringPtr, &HexStringBufferLen, Buffer, sizeof (UINT16));
    EfiCopyMem (&Width, Buffer, sizeof (UINT16));
    Buffer += sizeof (UINT16);
    StringPtr += 4;

    EfiStrCpy (StringPtr, L"&VALUE=");
    StringPtr += 7;

    HexStringBufferLen = StringEnd - StringPtr;
    Status = BufToHexString (StringPtr, &HexStringBufferLen, Buffer, Width);
    if (EFI_ERROR (Status)) {
      return Status;
    }
    Buffer += Width;
    StringPtr += (Width * 2);
  }

  return EFI_SUCCESS;
}

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
/*++

  Routine Description:

  Construct <ConfigAltResp> for a buffer storage.

  Arguments:
    ConfigRequest         - The Config request string. If set to NULL, all the
                            configurable elements will be extracted from BlockNameArray.
    ConfigAltResp         - The returned <ConfigAltResp>.
    Progress              - On return, points to a character in the Request.
    Guid                  - GUID of the buffer storage.
    Name                  - Name of the buffer storage.
    DriverHandle          - The DriverHandle which is used to invoke HiiDatabase
                            protocol interface NewPackageList().
    BufferStorage         - Content of the buffer storage.
    BufferStorageSize     - Length in bytes of the buffer storage.
    BlockNameArray        - Array generated by VFR compiler.
    NumberAltCfg          - Number of Default value array generated by VFR compiler.
                            The sequential input parameters will be number of
                            AltCfgId and DefaultValueArray pairs. When set to 0,
                            there will be no <AltResp>.

  Returns:
    EFI_OUT_OF_RESOURCES  - Run out of memory resource.
    EFI_INVALID_PARAMETER - ConfigAltResp is NULL.
    EFI_SUCCESS           - Operation successful.

--*/
{
  EFI_STATUS                      Status;
  CHAR16                          *ConfigHdr;
  CHAR16                          *BlockName;
  CHAR16                          *DescHdr;
  CHAR16                          *StringPtr;
  CHAR16                          **AltCfg;
  UINT16                          AltCfgId;
  VOID                            *DefaultValueArray;
  UINTN                           StrBufferLen;
  EFI_STRING                      ConfigResp;
  EFI_STRING                      TempStr;
  VA_LIST                         Args;
  UINTN                           AltRespLen;
  UINTN                           Index;
  BOOLEAN                         NeedFreeConfigRequest;
  EFI_HII_CONFIG_ROUTING_PROTOCOL *HiiConfigRouting;

  if (ConfigAltResp == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Construct <ConfigHdr> : "GUID=...&NAME=...&PATH=..."
  //
  ConfigHdr = NULL;
  StrBufferLen = 0;
  Status = ConstructConfigHdr (
             ConfigHdr,
             &StrBufferLen,
             Guid,
             Name,
             DriverHandle
             );
  if (Status == EFI_BUFFER_TOO_SMALL) {
    ConfigHdr = EfiLibAllocateZeroPool (StrBufferLen);
    Status = ConstructConfigHdr (
               ConfigHdr,
               &StrBufferLen,
               Guid,
               Name,
               DriverHandle
               );
  }

  if (EFI_ERROR (Status) || (ConfigHdr == NULL)) {
    return Status;
  }

  //
  // Construct <ConfigResp>
  //
  NeedFreeConfigRequest = FALSE;
  if (ConfigRequest == NULL) {
    //
    // If ConfigRequest is set to NULL, export all configurable elements in BlockNameArray
    //
    Status = ExtractBlockName (BlockNameArray, &BlockName);
    if (EFI_ERROR (Status)) {
      return Status;
    }

    StrBufferLen = EfiStrSize (ConfigHdr);
    StrBufferLen = StrBufferLen + EfiStrSize (BlockName) - sizeof (CHAR16);
    ConfigRequest = EfiLibAllocateZeroPool (StrBufferLen);
    EfiStrCpy (ConfigRequest, ConfigHdr);
    EfiStrCat (ConfigRequest, BlockName);
    NeedFreeConfigRequest = TRUE;
  }

  Status = gBS->LocateProtocol (&gEfiHiiConfigRoutingProtocolGuid, NULL, (VOID **) &HiiConfigRouting);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  Status = HiiConfigRouting->BlockToConfig (
                               HiiConfigRouting,
                               ConfigRequest,
                               BufferStorage,
                               BufferStorageSize,
                               &ConfigResp,
                               (Progress == NULL) ? &TempStr : Progress
                               );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Construct <AltResp>
  //
  DescHdr = EfiLibAllocateZeroPool (NumberAltCfg * 16 * sizeof (CHAR16));
  StringPtr = DescHdr;
  AltCfg = EfiLibAllocateZeroPool (NumberAltCfg * sizeof (CHAR16 *));
  AltRespLen = 0;
  VA_START (Args, NumberAltCfg);
  for (Index = 0; Index < NumberAltCfg; Index++) {
    AltCfgId = (UINT16) VA_ARG (Args, UINT16);
    DefaultValueArray = (UINT8 *) VA_ARG (Args, VOID *);

    //
    // '&' <ConfigHdr>
    //
    AltRespLen += (EfiStrLen (ConfigHdr) + 1);

    StringPtr = DescHdr + Index * 16;
    EfiStrCpy (StringPtr, L"&ALTCFG=");
    AltRespLen += (8 + sizeof (UINT16) * 2);

    StrBufferLen = 5;
    BufToHexString (StringPtr + 8, &StrBufferLen, (UINT8 *) &AltCfgId, sizeof (UINT16));
    Status = ExtractBlockConfig (DefaultValueArray, &AltCfg[Index]);
    if (EFI_ERROR (Status)) {
      VA_END (Args);
      return Status;
    }
    AltRespLen += EfiStrLen (AltCfg[Index]);
  }
  VA_END (Args);

  //
  // Generate the final <ConfigAltResp>
  //
  StrBufferLen = (EfiStrLen ((CHAR16 *) ConfigResp) + AltRespLen + 1) * sizeof (CHAR16);
  TempStr = EfiLibAllocateZeroPool (StrBufferLen);
  *ConfigAltResp = TempStr;
  if (TempStr == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // <ConfigAltResp> ::= <ConfigResp> ['&' <AltResp>]*
  //
  EfiStrCpy (TempStr, ConfigResp);
  for (Index = 0; Index < NumberAltCfg; Index++) {
    EfiStrCat (TempStr, L"&");
    EfiStrCat (TempStr, ConfigHdr);
    EfiStrCat (TempStr, DescHdr + Index * 16);
    EfiStrCat (TempStr, AltCfg[Index]);

    gBS->FreePool (AltCfg[Index]);
  }

  if (NeedFreeConfigRequest) {
    gBS->FreePool (ConfigRequest);
  }
  gBS->FreePool (ConfigHdr);
  gBS->FreePool (ConfigResp);
  gBS->FreePool (DescHdr);
  gBS->FreePool (AltCfg);

  return EFI_SUCCESS;
}

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

  SwapCount = BufferSize / 2;
  for (Index = 0; Index < SwapCount; Index++) {
    Temp = Buffer[Index];
    Buffer[Index] = Buffer[BufferSize - 1 - Index];
    Buffer[BufferSize - 1 - Index] = Temp;
  }
}

VOID
ToLower (
  IN OUT CHAR16    *Str
  )
/*++

Routine Description:
  Converts the unicode character of the string from uppercase to lowercase.

Arguments:
  Str        -  String to be converted

Returns:

--*/
{
  CHAR16      *Ptr;

  for (Ptr = Str; *Ptr != L'\0'; Ptr++) {
    if (*Ptr >= L'A' && *Ptr <= L'Z') {
      *Ptr = (CHAR16) (*Ptr - L'A' + L'a');
    }
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

  StrBufferLen = BufferSize * 2 + 1;
  Status = BufToHexString (Str, &StrBufferLen, NewBuffer, BufferSize);

  gBS->FreePool (NewBuffer);
  //
  // Convert the uppercase to lowercase since <HexAf> is defined in lowercase format.
  //
  ToLower (Str);

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
    SwapBuffer (Buffer, (ConvertedStrLen + 1) / 2);
  }

  return Status;
}

EFI_STATUS
ConfigStringToUnicode (
  IN OUT CHAR16                *UnicodeString,
  IN OUT UINTN                 *StrBufferLen,
  IN CHAR16                    *ConfigString
  )
/*++

Routine Description:
  Convert binary representation Config string (e.g. "0041004200430044") to the
  original string (e.g. "ABCD"). Config string appears in <ConfigHdr> (i.e.
  "&NAME=<string>"), or Name/Value pair in <ConfigBody> (i.e. "label=<string>").

Arguments:
  UnicodeString - Original Unicode string.
  StrBufferLen  - On input: Length in bytes of buffer to hold the Unicode string.
                  Includes tailing '\0' character.
                  On output:
                    If return EFI_SUCCESS, containing length of Unicode string buffer.
                    If return EFI_BUFFER_TOO_SMALL, containg length of string buffer desired.
  ConfigString  - Binary representation of Unicode String, <string> := (<HexCh>4)+

Returns:
  EFI_SUCCESS          - Routine success.
  EFI_BUFFER_TOO_SMALL - The string buffer is too small.

--*/
{
  UINTN       Index;
  UINTN       Len;
  UINTN       BufferSize;
  CHAR16      BackupChar;

  Len = EfiStrLen (ConfigString) / 4;
  BufferSize = (Len + 1) * sizeof (CHAR16);

  if (*StrBufferLen < BufferSize) {
    *StrBufferLen = BufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StrBufferLen = BufferSize;

  for (Index = 0; Index < Len; Index++) {
    BackupChar = ConfigString[4];
    ConfigString[4] = L'\0';

    HexStringToBuf ((UINT8 *) UnicodeString, &BufferSize, ConfigString, NULL);

    ConfigString[4] = BackupChar;

    ConfigString += 4;
    UnicodeString += 1;
  }

  //
  // Add tailing '\0' character
  //
  *UnicodeString = L'\0';

  return EFI_SUCCESS;
}

EFI_STATUS
UnicodeToConfigString (
  IN OUT CHAR16                *ConfigString,
  IN OUT UINTN                 *StrBufferLen,
  IN CHAR16                    *UnicodeString
  )
/*++

Routine Description:
  Convert Unicode string to binary representation Config string, e.g.
  "ABCD" => "0041004200430044". Config string appears in <ConfigHdr> (i.e.
  "&NAME=<string>"), or Name/Value pair in <ConfigBody> (i.e. "label=<string>").

Arguments:
  ConfigString  - Binary representation of Unicode String, <string> := (<HexCh>4)+
  StrBufferLen  - On input: Length in bytes of buffer to hold the Unicode string.
                  Includes tailing '\0' character.
                  On output:
                    If return EFI_SUCCESS, containing length of Unicode string buffer.
                    If return EFI_BUFFER_TOO_SMALL, containg length of string buffer desired.
  UnicodeString - Original Unicode string.

Returns:
  EFI_SUCCESS          - Routine success.
  EFI_BUFFER_TOO_SMALL - The string buffer is too small.

--*/
{
  UINTN       Index;
  UINTN       Len;
  UINTN       BufferSize;
  CHAR16      *String;

  Len = EfiStrLen (UnicodeString);
  BufferSize = (Len * 4 + 1) * sizeof (CHAR16);

  if (*StrBufferLen < BufferSize) {
    *StrBufferLen = BufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  *StrBufferLen = BufferSize;
  String        = ConfigString;

  for (Index = 0; Index < Len; Index++) {
    BufToHexString (ConfigString, &BufferSize, (UINT8 *) UnicodeString, 2);

    ConfigString += 4;
    UnicodeString += 1;
  }

  //
  // Add tailing '\0' character
  //
  *ConfigString = L'\0';

  //
  // Convert the uppercase to lowercase since <HexAf> is defined in lowercase format.
  //
  ToLower (String);
  return EFI_SUCCESS;
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
  // GUID=<HexCh>32&NAME=<Char>NameStrLen&PATH=<HexChar>DevicePathStrLen <NULL>
  // | 5  |   32   |  6  |  NameStrLen*4 |  6  |    DevicePathStrLen    | 1 |
  //
  BufferSize = (5 + 32 + 6 + NameStrLen * 4 + 6 + DevicePathSize * 2 + 1) * sizeof (CHAR16);
  if (*StrBufferLen < BufferSize) {
    *StrBufferLen = BufferSize;
    return EFI_BUFFER_TOO_SMALL;
  }

  if (ConfigHdr == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *StrBufferLen = BufferSize;

  StrPtr = ConfigHdr;

  EfiStrCpy (StrPtr, L"GUID=");
  StrPtr += 5;
  BufferToHexString (StrPtr, (UINT8 *) Guid, sizeof (EFI_GUID));
  StrPtr += 32;

  //
  // Convert name string, e.g. name "ABCD" => "&NAME=0041004200430044"
  //
  EfiStrCpy (StrPtr, L"&NAME=");
  StrPtr += 6;
  if (Name != NULL) {
    BufferSize = (NameStrLen * 4 + 1) * sizeof (CHAR16);
    UnicodeToConfigString (StrPtr, &BufferSize, Name);
    StrPtr += (NameStrLen * 4);
  }

  EfiStrCpy (StrPtr, L"&PATH=");
  StrPtr += 6;
  BufferToHexString (StrPtr, (UINT8 *) DevicePath, DevicePathSize);

  return EFI_SUCCESS;
}

BOOLEAN
IsConfigHdrMatch (
  IN EFI_STRING                ConfigString,
  IN EFI_GUID                  *StorageGuid, OPTIONAL
  IN CHAR16                    *StorageName  OPTIONAL
  )
/*++

Routine Description:
  Determines if the Routing data (Guid and Name) is correct in <ConfigHdr>.

Arguments:
  ConfigString - Either <ConfigRequest> or <ConfigResp>.
  StorageGuid  - GUID of the storage.
  StorageName  - Name of the stoarge.

Returns:
  TRUE         - Routing information is correct in ConfigString.
  FALSE        - Routing information is incorrect in ConfigString.

--*/
{
  EFI_STATUS  Status;
  BOOLEAN     Match;
  EFI_GUID    Guid;
  CHAR16      *Name;
  CHAR16      *StrPtr;
  UINTN       BufferSize;

  //
  // <ConfigHdr> ::=
  // GUID=<HexCh>32&NAME=<Char>NameStrLen&PATH=<HexChar>DevicePathStrLen <NULL>
  // | 5  |   32   |  6  |  NameStrLen*4 |  6  |    DevicePathStrLen    | 1 |
  //
  if (EfiStrLen (ConfigString) <= (5 + 32 + 6)) {
    return FALSE;
  }

  //
  // Compare GUID
  //
  if (StorageGuid != NULL) {

    StrPtr = ConfigString + 5 + 32;
    if (*StrPtr != L'&') {
      return FALSE;
    }
    *StrPtr = L'\0';

    BufferSize = sizeof (EFI_GUID);
    Status = HexStringToBuffer (
               (UINT8 *) &Guid,
               &BufferSize,
               ConfigString + 5
               );
    *StrPtr = L'&';

    if (EFI_ERROR (Status)) {
      return FALSE;
    }

    if (!EfiCompareGuid (&Guid, StorageGuid)) {
      return FALSE;
    }
  }

  //
  // Compare Name
  //
  Match = TRUE;
  if (StorageName != NULL) {
    StrPtr = ConfigString + 5 + 32 + 6;
    while (*StrPtr != L'\0' && *StrPtr != L'&') {
      StrPtr++;
    }
    if (*StrPtr != L'&') {
      return FALSE;
    }

    *StrPtr = L'\0';
    BufferSize = (EfiStrLen (ConfigString + 5 + 32 + 6) + 1) * sizeof (CHAR16);
    Name = EfiLibAllocatePool (BufferSize);
    ASSERT (Name != NULL);
    Status = ConfigStringToUnicode (
               Name,
               &BufferSize,
               ConfigString + 5 + 32 + 6
               );
    *StrPtr = L'&';

    if (EFI_ERROR (Status) || (EfiStrCmp (Name, StorageName) != 0)) {
      Match = FALSE;
    }
    gBS->FreePool (Name);
  }

  return Match;
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
  ConfigResp = EfiLibAllocateZeroPool (BufferLen + (HeaderLen + 1) * sizeof (CHAR16));

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
    ConfigResp = EfiLibAllocateZeroPool (BufferLen + (HeaderLen + 1) * sizeof (CHAR16));

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
    gBS->FreePool (ConfigRequest);
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
                           VariableGuid,
                           VariableName
                           );
  gBS->FreePool (ConfigResp);
  gBS->FreePool (ConfigRequest);
  return Status;
}
