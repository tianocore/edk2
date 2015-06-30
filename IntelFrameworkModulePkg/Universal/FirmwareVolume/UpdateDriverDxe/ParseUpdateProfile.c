/** @file
  Source file for the component update driver. It parse the update
  configuration file and pass the information to the update driver
  so that the driver can perform updates accordingly.

  Copyright (c) 2002 - 2015, Intel Corporation. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions
  of the BSD License which accompanies this distribution.  The
  full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UpdateDriver.h"

/**
  Copy one line data from buffer data to the line buffer.

  @param Buffer          Buffer data.
  @param BufferSize      Buffer Size.
  @param LineBuffer      Line buffer to store the found line data.
  @param LineSize        On input, size of the input line buffer.
                         On output, size of the actual line buffer.

  @retval EFI_BUFFER_TOO_SMALL  The size of input line buffer is not enough.
  @retval EFI_SUCCESS           Copy line data into the line buffer.

**/
EFI_STATUS
ProfileGetLine (
  IN      UINT8                         *Buffer,
  IN      UINTN                         BufferSize,
  IN OUT  UINT8                         *LineBuffer,
  IN OUT  UINTN                         *LineSize
  )
{
  UINTN                                 Length;
  UINT8                                 *PtrBuf;
  UINTN                                 PtrEnd;

  PtrBuf      = Buffer;
  PtrEnd      = (UINTN)Buffer + BufferSize;

  //
  // 0x0D indicates a line break. Otherwise there is no line break
  //
  while ((UINTN)PtrBuf < PtrEnd) {
    if (*PtrBuf == 0x0D) {
      break;
    }
    PtrBuf++;
  }

  if ((UINTN)PtrBuf >= (PtrEnd - 1)) {
    //
    // The buffer ends without any line break
    // or it is the last character of the buffer
    //
    Length    = BufferSize;
  } else if (*(PtrBuf + 1) == 0x0A) {
    //
    // Further check if a 0x0A follows. If yes, count 0xA
    //
    Length    = (UINTN) PtrBuf - (UINTN) Buffer + 2;
  } else {
    Length    = (UINTN) PtrBuf - (UINTN) Buffer + 1;
  }

  if (Length > (*LineSize)) {
    *LineSize = Length;
    return EFI_BUFFER_TOO_SMALL;
  }

  SetMem (LineBuffer, *LineSize, 0x0);
  *LineSize   = Length;
  CopyMem (LineBuffer, Buffer, Length);

  return EFI_SUCCESS;
}

/**
  Trim Buffer by removing all CR, LF, TAB, and SPACE chars in its head and tail.

  @param Buffer          On input,  buffer data to be trimed.
                         On output, the trimmed buffer.
  @param BufferSize      On input,  size of original buffer data.
                         On output, size of the trimmed buffer.

**/
VOID
ProfileTrim (
  IN OUT  UINT8                         *Buffer,
  IN OUT  UINTN                         *BufferSize
  )
{
  UINTN                                 Length;
  UINT8                                 *PtrBuf;
  UINT8                                 *PtrEnd;

  if (*BufferSize == 0) {
    return;
  }

  //
  // Trim the tail first, include CR, LF, TAB, and SPACE.
  //
  Length          = *BufferSize;
  PtrBuf          = (UINT8 *) ((UINTN) Buffer + Length - 1);
  while (PtrBuf >= Buffer) {
    if ((*PtrBuf != 0x0D) && (*PtrBuf != 0x0A )
      && (*PtrBuf != 0x20) && (*PtrBuf != 0x09)) {
      break;
    }
    PtrBuf --;
  }

  //
  // all spaces, a blank line, return directly;
  //
  if (PtrBuf < Buffer) {
    *BufferSize   = 0;
    return;
  }

  Length          = (UINTN)PtrBuf - (UINTN)Buffer + 1;
  PtrEnd          = PtrBuf;
  PtrBuf          = Buffer;

  //
  // Now skip the heading CR, LF, TAB and SPACE
  //
  while (PtrBuf <= PtrEnd) {
    if ((*PtrBuf != 0x0D) && (*PtrBuf != 0x0A )
      && (*PtrBuf != 0x20) && (*PtrBuf != 0x09)) {
      break;
    }
    PtrBuf++;
  }

  //
  // If no heading CR, LF, TAB or SPACE, directly return
  //
  if (PtrBuf == Buffer) {
    *BufferSize   = Length;
    return;
  }

  *BufferSize     = (UINTN)PtrEnd - (UINTN)PtrBuf + 1;

  //
  // The first Buffer..PtrBuf characters are CR, LF, TAB or SPACE.
  // Now move out all these characters.
  //
  while (PtrBuf <= PtrEnd) {
    *Buffer       = *PtrBuf;
    Buffer++;
    PtrBuf++;
  }

  return;
}

/**
  Insert new comment item into comment head.

  @param Buffer          Comment buffer to be added.
  @param BufferSize      Size of comment buffer.
  @param CommentHead     Comment Item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            New comment item is inserted.

**/
EFI_STATUS
ProfileGetComments (
  IN      UINT8                         *Buffer,
  IN      UINTN                         BufferSize,
  IN OUT  COMMENT_LINE                  **CommentHead
  )
{
  COMMENT_LINE                          *CommentItem;

  CommentItem = NULL;
  CommentItem = AllocatePool (sizeof (COMMENT_LINE));
  if (CommentItem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  CommentItem->ptrNext  = *CommentHead;
  *CommentHead          = CommentItem;

  //
  // Add a trailing '\0'
  //
  CommentItem->ptrComment = AllocatePool (BufferSize + 1);
  if (CommentItem->ptrComment == NULL) {
    FreePool (CommentItem);
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (CommentItem->ptrComment, Buffer, BufferSize);
  *(CommentItem->ptrComment + BufferSize) = '\0';

  return EFI_SUCCESS;
}

/**
  Add new section item into Section head.

  @param Buffer          Section item data buffer.
  @param BufferSize      Size of section item.
  @param SectionHead     Section item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            Section item is NULL or Section item is added.

**/
EFI_STATUS
ProfileGetSection (
  IN      UINT8                         *Buffer,
  IN      UINTN                         BufferSize,
  IN OUT  SECTION_ITEM                  **SectionHead
  )
{
  EFI_STATUS                            Status;
  SECTION_ITEM                          *SectionItem;
  UINTN                                 Length;
  UINT8                                 *PtrBuf;

  Status      = EFI_SUCCESS;
  //
  // The first character of Buffer is '[', now we want for ']'
  //
  PtrBuf      = (UINT8 *)((UINTN)Buffer + BufferSize - 1);
  while (PtrBuf > Buffer) {
    if (*PtrBuf == ']') {
      break;
    }
    PtrBuf --;
  }
  if (PtrBuf <= Buffer) {
    //
    // Not found. Omit this line
    //
    return Status;
  }

  //
  // excluding the heading '[' and tailing ']'
  //
  Length      = PtrBuf - Buffer - 1;
  ProfileTrim (
    Buffer + 1,
    &Length
  );

  //
  // omit this line if the section name is null
  //
  if (Length == 0) {
    return Status;
  }

  SectionItem = AllocatePool (sizeof (SECTION_ITEM));
  if (SectionItem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SectionItem->ptrSection = NULL;
  SectionItem->SecNameLen = Length;
  SectionItem->ptrEntry   = NULL;
  SectionItem->ptrValue   = NULL;
  SectionItem->ptrNext    = *SectionHead;
  *SectionHead            = SectionItem;

  //
  // Add a trailing '\0'
  //
  SectionItem->ptrSection = AllocatePool (Length + 1);
  if (SectionItem->ptrSection == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // excluding the heading '['
  //
  CopyMem (SectionItem->ptrSection, Buffer + 1, Length);
  *(SectionItem->ptrSection + Length) = '\0';

  return EFI_SUCCESS;
}

/**
  Add new section entry and entry value into Section head.

  @param Buffer          Section entry data buffer.
  @param BufferSize      Size of section entry.
  @param SectionHead     Section item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            Section entry is NULL or Section entry is added.

**/
EFI_STATUS
ProfileGetEntry (
  IN      UINT8                         *Buffer,
  IN      UINTN                         BufferSize,
  IN OUT  SECTION_ITEM                  **SectionHead
  )
{
  EFI_STATUS                            Status;
  SECTION_ITEM                          *SectionItem;
  SECTION_ITEM                          *PtrSection;
  UINTN                                 Length;
  UINT8                                 *PtrBuf;
  UINT8                                 *PtrEnd;

  Status      = EFI_SUCCESS;
  PtrBuf      = Buffer;
  PtrEnd      = (UINT8 *) ((UINTN)Buffer + BufferSize - 1);

  //
  // First search for '='
  //
  while (PtrBuf <= PtrEnd) {
    if (*PtrBuf == '=') {
      break;
    }
    PtrBuf++;
  }
  if (PtrBuf > PtrEnd) {
    //
    // Not found. Omit this line
    //
    return Status;
  }

  //
  // excluding the tailing '='
  //
  Length      = PtrBuf - Buffer;
  ProfileTrim (
    Buffer,
    &Length
  );

  //
  // Omit this line if the entry name is null
  //
  if (Length == 0) {
    return Status;
  }

  //
  // Omit this line if no section header has been found before
  //
  if (*SectionHead == NULL) {
    return Status;
  }
  PtrSection  = *SectionHead;

  SectionItem = AllocatePool (sizeof (SECTION_ITEM));
  if (SectionItem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SectionItem->ptrSection = NULL;
  SectionItem->ptrEntry   = NULL;
  SectionItem->ptrValue   = NULL;
  SectionItem->SecNameLen = PtrSection->SecNameLen;
  SectionItem->ptrNext    = *SectionHead;
  *SectionHead            = SectionItem;

  //
  // SectionName, add a trailing '\0'
  //
  SectionItem->ptrSection = AllocatePool (PtrSection->SecNameLen + 1);
  if (SectionItem->ptrSection == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (SectionItem->ptrSection, PtrSection->ptrSection, PtrSection->SecNameLen + 1);

  //
  // EntryName, add a trailing '\0'
  //
  SectionItem->ptrEntry = AllocatePool (Length + 1);
  if (SectionItem->ptrEntry == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (SectionItem->ptrEntry, Buffer, Length);
  *(SectionItem->ptrEntry + Length) = '\0';

  //
  // Next search for '#'
  //
  PtrBuf      = PtrBuf + 1;
  Buffer      = PtrBuf;
  while (PtrBuf <= PtrEnd) {
    if (*PtrBuf == '#') {
      break;
    }
    PtrBuf++;
  }
  Length      = PtrBuf - Buffer;
  ProfileTrim (
    Buffer,
    &Length
  );

  if (Length > 0) {
    //
    // EntryValue, add a trailing '\0'
    //
    SectionItem->ptrValue = AllocatePool (Length + 1);
    if (SectionItem->ptrValue == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }
    CopyMem (SectionItem->ptrValue, Buffer, Length);
    *(SectionItem->ptrValue + Length) = '\0';
  }

  return EFI_SUCCESS;
}

/**
  Free all comment entry and section entry.

  @param Section         Section entry list.
  @param Comment         Comment entry list.

**/
VOID
FreeAllList (
  IN      SECTION_ITEM                  *Section,
  IN      COMMENT_LINE                  *Comment
  )
{
  SECTION_ITEM                          *PtrSection;
  COMMENT_LINE                          *PtrComment;

  while (Section != NULL) {
    PtrSection    = Section;
    Section       = Section->ptrNext;
    if (PtrSection->ptrEntry != NULL) {
      FreePool (PtrSection->ptrEntry);
    }
    if (PtrSection->ptrSection != NULL) {
      FreePool (PtrSection->ptrSection);
    }
    if (PtrSection->ptrValue != NULL) {
      FreePool (PtrSection->ptrValue);
    }
    FreePool (PtrSection);
  }

  while (Comment != NULL) {
    PtrComment    = Comment;
    Comment       = Comment->ptrNext;
    if (PtrComment->ptrComment != NULL) {
      FreePool (PtrComment->ptrComment);
    }
    FreePool (PtrComment);
  }

  return;
}

/**
  Get section entry value.

  @param Section         Section entry list.
  @param SectionName     Section name.
  @param EntryName       Section entry name.
  @param EntryValue      Point to the got entry value.

  @retval EFI_NOT_FOUND  Section is not found.
  @retval EFI_SUCCESS    Section entry value is got.

**/
EFI_STATUS
UpdateGetProfileString (
  IN      SECTION_ITEM                  *Section,
  IN      UINT8                         *SectionName,
  IN      UINT8                         *EntryName,
  OUT     UINT8                         **EntryValue
  )
{
  *EntryValue   = NULL;

  while (Section != NULL) {
    if (AsciiStrCmp ((CONST CHAR8 *) Section->ptrSection, (CONST CHAR8 *) SectionName) == 0) {
      if (Section->ptrEntry != NULL) {
        if (AsciiStrCmp ((CONST CHAR8 *) Section->ptrEntry, (CONST CHAR8 *) EntryName) == 0) {
          break;
        }
      }
    }
    Section     = Section->ptrNext;
  }

  if (Section == NULL) {
    return EFI_NOT_FOUND;
  }

  *EntryValue   = (UINT8 *) Section->ptrValue;

  return EFI_SUCCESS;
}

/**
  Convert the dec or hex ascii string to value.

  @param Str             ascii string to be converted.

  @return the converted value.

**/
UINTN
UpdateAtoi (
  IN      UINT8                         *Str
  )
{
  UINTN Number;

  Number = 0;

  //
  // Skip preceeding while spaces
  //
  while (*Str != '\0') {
    if (*Str != 0x20) {
      break;
    }
    Str++;
  }

  if (*Str == '\0') {
    return Number;
  }

  //
  // Find whether the string is prefixed by 0x.
  // That is, it should be xtoi or atoi.
  //
  if (*Str == '0') {
    if ((*(Str+1) == 'x' ) || ( *(Str+1) == 'X')) {
      return AsciiStrHexToUintn ((CONST CHAR8 *) Str);
    }
  }

  while (*Str != '\0') {
    if ((*Str >= '0') && (*Str <= '9')) {
      Number  = Number * 10 + *Str - '0';
    } else {
      break;
    }
    Str++;
  }

  return Number;
}

/**
  Converts a decimal value to a Null-terminated ascii string.

  @param  Buffer  Pointer to the output buffer for the produced Null-terminated
                  ASCII string.
  @param  Value   The 64-bit sgned value to convert to a string.

  @return The number of ASCII characters in Buffer not including the Null-terminator.

**/
UINTN
UpdateValueToString (
  IN  OUT UINT8                         *Buffer,
  IN      INT64                         Value
  )
{
  UINT8                                 TempBuffer[30];
  UINT8                                 *TempStr;
  UINT8                                 *BufferPtr;
  UINTN                                 Count;
  UINT32                                Remainder;

  TempStr           = TempBuffer;
  BufferPtr         = Buffer;
  Count             = 0;

  if (Value < 0) {
    *BufferPtr      = '-';
    BufferPtr++;
    Value           = -Value;
    Count++;
  }

  do {
    Value = (INT64) DivU64x32Remainder  ((UINT64)Value, 10, &Remainder);
    //
    // The first item of TempStr is not occupied. It's kind of flag
    //
    TempStr++;
    Count++;
    *TempStr        = (UINT8) ((UINT8)Remainder + '0');
  } while (Value != 0);

  //
  // Reverse temp string into Buffer.
  //
  while (TempStr != TempBuffer) {
    *BufferPtr      = *TempStr;
    BufferPtr++;
    TempStr --;
  }

  *BufferPtr = 0;

  return Count;
}

/**
  Convert the input value to a ascii string, 
  and concatenates this string to the input string.

  @param Str             Pointer to a Null-terminated ASCII string.
  @param Number          The unsgned value to convert to a string.

**/
VOID
UpdateStrCatNumber (
  IN OUT  UINT8                         *Str,
  IN      UINTN                         Number
  )
{
  UINTN                                 Count;

  while (*Str != '\0') {
    Str++;
  }

  Count = UpdateValueToString (Str, (INT64)Number);

  *(Str + Count) = '\0';

  return;
}

/**
  Convert the input ascii string into GUID value.

  @param Str             Ascii GUID string to be converted.
  @param Guid            Pointer to the converted GUID value.

  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_NOT_FOUND         The input ascii string is not a valid GUID format string.
  @retval EFI_SUCCESS           GUID value is got.

**/
EFI_STATUS
UpdateStringToGuid (
  IN      UINT8                         *Str,
  IN OUT  EFI_GUID                      *Guid
  )
{
  UINT8                                 *PtrBuffer;
  UINT8                                 *PtrPosition;
  UINT8                                 *Buffer;
  UINTN                                 Data;
  UINTN                                 StrLen;
  UINTN                                 Index;
  UINT8                                 Digits[3];

  StrLen          = AsciiStrLen  ((CONST CHAR8 *) Str);
  Buffer          = AllocateCopyPool (StrLen + 1, Str);
  if (Buffer == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Data1
  //
  PtrBuffer       = Buffer;
  PtrPosition     = PtrBuffer;
  while (*PtrBuffer != '\0') {
    if (*PtrBuffer == '-') {
      break;
    }
    PtrBuffer++;
  }
  if (*PtrBuffer == '\0') {
    FreePool (Buffer);
    return EFI_NOT_FOUND;
  }

  *PtrBuffer      = '\0';
  Data            = AsciiStrHexToUintn ((CONST CHAR8 *) PtrPosition);
  Guid->Data1     = (UINT32)Data;

  //
  // Data2
  //
  PtrBuffer++;
  PtrPosition     = PtrBuffer;
  while (*PtrBuffer != '\0') {
    if (*PtrBuffer == '-') {
      break;
    }
    PtrBuffer++;
  }
  if (*PtrBuffer == '\0') {
    FreePool (Buffer);
    return EFI_NOT_FOUND;
  }
  *PtrBuffer      = '\0';
  Data            = AsciiStrHexToUintn ((CONST CHAR8 *) PtrPosition);
  Guid->Data2     = (UINT16)Data;

  //
  // Data3
  //
  PtrBuffer++;
  PtrPosition     = PtrBuffer;
  while (*PtrBuffer != '\0') {
    if (*PtrBuffer == '-') {
      break;
    }
    PtrBuffer++;
  }
  if (*PtrBuffer == '\0') {
    FreePool (Buffer);
    return EFI_NOT_FOUND;
  }
  *PtrBuffer      = '\0';
  Data            = AsciiStrHexToUintn ((CONST CHAR8 *) PtrPosition);
  Guid->Data3     = (UINT16)Data;

  //
  // Data4[0..1]
  //
  for ( Index = 0 ; Index < 2 ; Index++) {
    PtrBuffer++;
    if ((*PtrBuffer == '\0') || ( *(PtrBuffer + 1) == '\0')) {
      FreePool (Buffer);
      return EFI_NOT_FOUND;
    }
    Digits[0]     = *PtrBuffer;
    PtrBuffer++;
    Digits[1]     = *PtrBuffer;
    Digits[2]     = '\0';
    Data          = AsciiStrHexToUintn ((CONST CHAR8 *) Digits);
    Guid->Data4[Index] = (UINT8)Data;
  }

  //
  // skip the '-'
  //
  PtrBuffer++;
  if ((*PtrBuffer != '-' ) || ( *PtrBuffer == '\0')) {
    return EFI_NOT_FOUND;
  }

  //
  // Data4[2..7]
  //
  for ( ; Index < 8; Index++) {
    PtrBuffer++;
    if ((*PtrBuffer == '\0') || ( *(PtrBuffer + 1) == '\0')) {
      FreePool (Buffer);
      return EFI_NOT_FOUND;
    }
    Digits[0]     = *PtrBuffer;
    PtrBuffer++;
    Digits[1]     = *PtrBuffer;
    Digits[2]     = '\0';
    Data          = AsciiStrHexToUintn ((CONST CHAR8 *) Digits);
    Guid->Data4[Index] = (UINT8)Data;
  }

  FreePool (Buffer);

  return EFI_SUCCESS;
}

/**
  Pre process config data buffer into Section entry list and Comment entry list.
 
  @param DataBuffer      Config raw file buffer.
  @param BufferSize      Size of raw buffer.
  @param SectionHead     Pointer to the section entry list.
  @param CommentHead     Pointer to the comment entry list.

  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Config data buffer is preprocessed.

**/
EFI_STATUS
PreProcessDataFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize,
  IN OUT  SECTION_ITEM                  **SectionHead,
  IN OUT  COMMENT_LINE                  **CommentHead
  )
{
  EFI_STATUS                            Status;
  CHAR8                                 *Source;
  CHAR8                                 *CurrentPtr;
  CHAR8                                 *BufferEnd;
  CHAR8                                 *PtrLine;
  UINTN                                 LineLength;
  UINTN                                 SourceLength;
  UINTN                                 MaxLineLength;

  *SectionHead          = NULL;
  *CommentHead          = NULL;
  BufferEnd             = (CHAR8 *) ( (UINTN) DataBuffer + BufferSize);
  CurrentPtr            = (CHAR8 *) DataBuffer;
  MaxLineLength         = MAX_LINE_LENGTH;
  Status                = EFI_SUCCESS;

  PtrLine = AllocatePool (MaxLineLength);
  if (PtrLine == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  while (CurrentPtr < BufferEnd) {
    Source              = CurrentPtr;
    SourceLength        = (UINTN)BufferEnd - (UINTN)CurrentPtr;
    LineLength          = MaxLineLength;
    //
    // With the assumption that line length is less than 512
    // characters. Otherwise BUFFER_TOO_SMALL will be returned.
    //
    Status              = ProfileGetLine (
                            (UINT8 *) Source,
                            SourceLength,
                            (UINT8 *) PtrLine,
                            &LineLength
                            );
    if (EFI_ERROR (Status)) {
      if (Status == EFI_BUFFER_TOO_SMALL) {
        //
        // If buffer too small, re-allocate the buffer according
        // to the returned LineLength and try again.
        //
        FreePool (PtrLine);
        PtrLine         = NULL;
        PtrLine = AllocatePool (LineLength);
        if (PtrLine == NULL) {
          Status        = EFI_OUT_OF_RESOURCES;
          break;
        }
        SourceLength    = LineLength;
        Status          = ProfileGetLine (
                            (UINT8 *) Source,
                            SourceLength,
                            (UINT8 *) PtrLine,
                            &LineLength
                            );
        if (EFI_ERROR (Status)) {
          break;
        }
        MaxLineLength   = LineLength;
      } else {
        break;
      }
    }
    CurrentPtr          = (CHAR8 *) ( (UINTN) CurrentPtr + LineLength);

    //
    // Line got. Trim the line before processing it.
    //
    ProfileTrim (
      (UINT8 *) PtrLine,
      &LineLength
   );

    //
    // Blank line
    //
    if (LineLength == 0) {
      continue;
    }

    if (PtrLine[0] == '#') {
      Status            = ProfileGetComments (
                            (UINT8 *) PtrLine,
                            LineLength,
                            CommentHead
                            );
    } else if (PtrLine[0] == '[') {
      Status            = ProfileGetSection (
                            (UINT8 *) PtrLine,
                            LineLength,
                            SectionHead
                            );
    } else {
      Status            = ProfileGetEntry (
                            (UINT8 *) PtrLine,
                            LineLength,
                            SectionHead
                            );
    }

    if (EFI_ERROR (Status)) {
      break;
    }
  }

  //
  // Free buffer
  //
  FreePool (PtrLine);

  return Status;
}

/**
  Parse Config data file to get the updated data array.

  @param DataBuffer      Config raw file buffer.
  @param BufferSize      Size of raw buffer.
  @param NumOfUpdates    Pointer to the number of update data.
  @param UpdateArray     Pointer to the config of update data.

  @retval EFI_NOT_FOUND         No config data is found.
  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Parse the config file successfully.

**/
EFI_STATUS
ParseUpdateDataFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize,
  IN OUT  UINTN                         *NumOfUpdates,
  IN OUT  UPDATE_CONFIG_DATA            **UpdateArray
  )
{
  EFI_STATUS                            Status;
  CHAR8                                 *Value;
  CHAR8                                 *SectionName;
  CHAR8                                 Entry[MAX_LINE_LENGTH];
  SECTION_ITEM                          *SectionHead;
  COMMENT_LINE                          *CommentHead;
  UINTN                                 Num;
  UINTN                                 Index;
  EFI_GUID                              FileGuid;

  SectionHead           = NULL;
  CommentHead           = NULL;

  //
  // First process the data buffer and get all sections and entries
  //
  Status                = PreProcessDataFile (
                            DataBuffer,
                            BufferSize,
                            &SectionHead,
                            &CommentHead
                            );
  if (EFI_ERROR (Status)) {
    FreeAllList (SectionHead, CommentHead);
    return Status;
  }

  //
  // Now get NumOfUpdate
  //
  Value                 = NULL;
  Status                = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) "Head",
                            (UINT8 *) "NumOfUpdate",
                            (UINT8 **) &Value
                            );
  if (Value == NULL) {
    FreeAllList (SectionHead, CommentHead);
    return EFI_NOT_FOUND;
  }
  Num                   = UpdateAtoi((UINT8 *) Value);
  if (Num <= 0) {
    FreeAllList (SectionHead, CommentHead);
    return EFI_NOT_FOUND;
  }

  *NumOfUpdates         = Num;
  *UpdateArray = AllocatePool ((sizeof (UPDATE_CONFIG_DATA) * Num));
  if (*UpdateArray == NULL) {
    FreeAllList (SectionHead, CommentHead);
    return EFI_OUT_OF_RESOURCES;
  }

  for ( Index = 0 ; Index < *NumOfUpdates ; Index++) {
    //
    // Get the section name of each update
    //
    AsciiStrCpyS (Entry, MAX_LINE_LENGTH, "Update");
    UpdateStrCatNumber ((UINT8 *) Entry, Index);
    Value               = NULL;
    Status              = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) "Head",
                            (UINT8 *) Entry,
                            (UINT8 **) &Value
                            );
    if (Value == NULL) {
      FreeAllList (SectionHead, CommentHead);
      return EFI_NOT_FOUND;
    }

    //
    // The section name of this update has been found.
    // Now looks for all the config data of this update
    //
    SectionName         = Value;

    //
    // UpdateType
    //
    Value               = NULL;
    Status              = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) SectionName,
                            (UINT8 *) "UpdateType",
                            (UINT8 **) &Value
                            );
    if (Value == NULL) {
      FreeAllList (SectionHead, CommentHead);
      return EFI_NOT_FOUND;
    }

    Num                 = UpdateAtoi((UINT8 *) Value);
    if (( Num >= (UINTN) UpdateOperationMaximum)) {
      FreeAllList (SectionHead, CommentHead);
      return Status;
    }
    (*UpdateArray)[Index].Index       = Index;
    (*UpdateArray)[Index].UpdateType  = (UPDATE_OPERATION_TYPE) Num;

    //
    // FvBaseAddress
    //
    Value               = NULL;
    Status              = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) SectionName,
                            (UINT8 *) "FvBaseAddress",
                            (UINT8 **) &Value
                            );
    if (Value == NULL) {
      FreeAllList (SectionHead, CommentHead);
      return EFI_NOT_FOUND;
    }

    Num                 = AsciiStrHexToUintn ((CONST CHAR8 *) Value);
    (*UpdateArray)[Index].BaseAddress = (EFI_PHYSICAL_ADDRESS) Num;

    //
    // FileBuid
    //
    Value               = NULL;
    Status              = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) SectionName,
                            (UINT8 *) "FileGuid",
                            (UINT8 **) &Value
                            );
    if (Value == NULL) {
      FreeAllList (SectionHead, CommentHead);
      return EFI_NOT_FOUND;
    }

    Status              = UpdateStringToGuid ((UINT8 *) Value, &FileGuid);
    if (EFI_ERROR (Status)) {
      FreeAllList (SectionHead, CommentHead);
      return Status;
    }
    CopyMem (&((*UpdateArray)[Index].FileGuid), &FileGuid, sizeof(EFI_GUID));

    //
    // FaultTolerant
    // Default value is FALSE
    //
    Value               = NULL;
    (*UpdateArray)[Index].FaultTolerant = FALSE;
    Status              = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) SectionName,
                            (UINT8 *) "FaultTolerant",
                            (UINT8 **) &Value
                           );
    if (EFI_ERROR (Status) && (Status != EFI_NOT_FOUND)) {
      FreeAllList (SectionHead, CommentHead);
      return Status;
    } else if (Value != NULL) {
      if (AsciiStriCmp ((CONST CHAR8 *) Value, (CONST CHAR8 *) "TRUE") == 0) {
        (*UpdateArray)[Index].FaultTolerant = TRUE;
      } else if (AsciiStriCmp ((CONST CHAR8 *) Value, (CONST CHAR8 *) "FALSE") == 0) {
        (*UpdateArray)[Index].FaultTolerant = FALSE;
      }
    }

    if ((*UpdateArray)[Index].UpdateType == UpdateFvRange) {
      //
      // Length
      //
      Value             = NULL;
      Status            = UpdateGetProfileString (
                            SectionHead,
                            (UINT8 *) SectionName,
                            (UINT8 *) "Length",
                            (UINT8 **) &Value
                            );
      if (Value == NULL) {
        FreeAllList (SectionHead, CommentHead);
        return EFI_NOT_FOUND;
      }

      Num               = AsciiStrHexToUintn ((CONST CHAR8 *) Value);
      (*UpdateArray)[Index].Length = (UINTN) Num;
    }
  }

  //
  // Now all configuration data got. Free those temporary buffers
  //
  FreeAllList (SectionHead, CommentHead);

  return EFI_SUCCESS;
}

