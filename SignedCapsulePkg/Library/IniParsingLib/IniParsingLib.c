/** @file
  This library parses the INI configuration file.

  The INI file format is:
    ================
    [SectionName]
    EntryName=EntryValue
    ================

    Where:
      1) SectionName is an ASCII string. The valid format is [A-Za-z0-9_]+
      2) EntryName is an ASCII string. The valid format is [A-Za-z0-9_]+
      3) EntryValue can be:
         3.1) an ASCII String. The valid format is [A-Za-z0-9_]+
         3.2) a GUID. The valid format is xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx, where x is [A-Fa-f0-9]
         3.3) a decimal value. The valid format is [0-9]+
         3.4) a heximal value. The valid format is 0x[A-Fa-f0-9]+
      4) '#' or ';' can be used as comment at anywhere.
      5) TAB(0x20) or SPACE(0x9) can be used as separator.
      6) LF(\n, 0xA) or CR(\r, 0xD) can be used as line break.

  Caution: This module requires additional review when modified.
  This driver will have external input - INI data file.

  OpenIniFile(), PreProcessDataFile(), ProfileGetSection(), ProfileGetEntry()
  will receive untrusted input and do basic validation.

  Copyright (c) 2016 - 2017, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/MemoryAllocationLib.h>

#define IS_HYPHEN(a)               ((a) == '-')
#define IS_NULL(a)                 ((a) == '\0')

// This is default allocation. Reallocation will happen if it is not enough.
#define MAX_LINE_LENGTH           512

typedef struct _INI_SECTION_ITEM SECTION_ITEM;
struct _INI_SECTION_ITEM {
  CHAR8                           *PtrSection;
  UINTN                           SecNameLen;
  CHAR8                           *PtrEntry;
  CHAR8                           *PtrValue;
  SECTION_ITEM                    *PtrNext;
};

typedef struct _INI_COMMENT_LINE COMMENT_LINE;
struct _INI_COMMENT_LINE {
  CHAR8                           *PtrComment;
  COMMENT_LINE                    *PtrNext;
};

typedef struct {
  SECTION_ITEM                  *SectionHead;
  COMMENT_LINE                  *CommentHead;
} INI_PARSING_LIB_CONTEXT;

/**
  Return if the digital char is valid.

  @param[in] DigitalChar    The digital char to be checked.
  @param[in] IncludeHex     If it include HEX char.

  @retval TRUE   The digital char is valid.
  @retval FALSE  The digital char is invalid.
**/
BOOLEAN
IsValidDigitalChar (
  IN CHAR8    DigitalChar,
  IN BOOLEAN  IncludeHex
  )
{
  if (DigitalChar >= '0' && DigitalChar <= '9') {
    return TRUE;
  }
  if (IncludeHex) {
    if (DigitalChar >= 'a' && DigitalChar <= 'f') {
      return TRUE;
    }
    if (DigitalChar >= 'A' && DigitalChar <= 'F') {
      return TRUE;
    }
  }
  return FALSE;
}

/**
  Return if the name char is valid.

  @param[in] NameChar    The name char to be checked.

  @retval TRUE   The name char is valid.
  @retval FALSE  The name char is invalid.
**/
BOOLEAN
IsValidNameChar (
  IN CHAR8  NameChar
  )
{
  if (NameChar >= 'a' && NameChar <= 'z') {
    return TRUE;
  }
  if (NameChar >= 'A' && NameChar <= 'Z') {
    return TRUE;
  }
  if (NameChar >= '0' && NameChar <= '9') {
    return TRUE;
  }
  if (NameChar == '_') {
    return TRUE;
  }
  return FALSE;
}

/**
  Return if the digital string is valid.

  @param[in] Digital        The digital to be checked.
  @param[in] Length         The length of digital string in bytes.
  @param[in] IncludeHex     If it include HEX char.

  @retval TRUE   The digital string is valid.
  @retval FALSE  The digital string is invalid.
**/
BOOLEAN
IsValidDigital (
  IN CHAR8    *Digital,
  IN UINTN    Length,
  IN BOOLEAN  IncludeHex
  )
{
  UINTN  Index;
  for (Index = 0; Index < Length; Index++) {
    if (!IsValidDigitalChar(Digital[Index], IncludeHex)) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
  Return if the decimal string is valid.

  @param[in] Decimal The decimal string to be checked.
  @param[in] Length  The length of decimal string in bytes.

  @retval TRUE   The decimal string is valid.
  @retval FALSE  The decimal string is invalid.
**/
BOOLEAN
IsValidDecimalString (
  IN CHAR8  *Decimal,
  IN UINTN  Length
  )
{
  return IsValidDigital(Decimal, Length, FALSE);
}

/**
  Return if the heximal string is valid.

  @param[in] Hex     The heximal string to be checked.
  @param[in] Length  The length of heximal string in bytes.

  @retval TRUE   The heximal string is valid.
  @retval FALSE  The heximal string is invalid.
**/
BOOLEAN
IsValidHexString (
  IN CHAR8  *Hex,
  IN UINTN  Length
  )
{
  if (Length <= 2) {
    return FALSE;
  }
  if (Hex[0] != '0') {
    return FALSE;
  }
  if (Hex[1] != 'x' && Hex[1] != 'X') {
    return FALSE;
  }
  return IsValidDigital(&Hex[2], Length - 2, TRUE);
}

/**
  Return if the name string is valid.

  @param[in] Name    The name to be checked.
  @param[in] Length  The length of name string in bytes.

  @retval TRUE   The name string is valid.
  @retval FALSE  The name string is invalid.
**/
BOOLEAN
IsValidName (
  IN CHAR8  *Name,
  IN UINTN  Length
  )
{
  UINTN  Index;
  for (Index = 0; Index < Length; Index++) {
    if (!IsValidNameChar(Name[Index])) {
      return FALSE;
    }
  }
  return TRUE;
}

/**
  Return if the value string is valid GUID.

  @param[in] Value   The value to be checked.
  @param[in] Length  The length of value string in bytes.

  @retval TRUE   The value string is valid GUID.
  @retval FALSE  The value string is invalid GUID.
**/
BOOLEAN
IsValidGuid (
  IN CHAR8  *Value,
  IN UINTN  Length
  )
{
  if (Length != sizeof("xxxxxxxx-xxxx-xxxx-xxxx-xxxxxxxxxxxx") - 1) {
    return FALSE;
  }
  if (!IS_HYPHEN(Value[8])) {
    return FALSE;
  }
  if (!IS_HYPHEN(Value[13])) {
    return FALSE;
  }
  if (!IS_HYPHEN(Value[18])) {
    return FALSE;
  }
  if (!IS_HYPHEN(Value[23])) {
    return FALSE;
  }
  if (!IsValidDigital(&Value[0], 8, TRUE)) {
    return FALSE;
  }
  if (!IsValidDigital(&Value[9], 4, TRUE)) {
    return FALSE;
  }
  if (!IsValidDigital(&Value[14], 4, TRUE)) {
    return FALSE;
  }
  if (!IsValidDigital(&Value[19], 4, TRUE)) {
    return FALSE;
  }
  if (!IsValidDigital(&Value[24], 12, TRUE)) {
    return FALSE;
  }
  return TRUE;
}

/**
  Return if the value string is valid.

  @param[in] Value    The value to be checked.
  @param[in] Length  The length of value string in bytes.

  @retval TRUE   The name string is valid.
  @retval FALSE  The name string is invalid.
**/
BOOLEAN
IsValidValue (
  IN CHAR8  *Value,
  IN UINTN  Length
  )
{
  if (IsValidName(Value, Length) || IsValidGuid(Value, Length)) {
    return TRUE;
  }
  return FALSE;
}

/**
  Dump an INI config file context.

  @param[in] Context         INI Config file context.
**/
VOID
DumpIniSection (
  IN VOID  *Context
  )
{
  INI_PARSING_LIB_CONTEXT               *IniContext;
  SECTION_ITEM                          *PtrSection;
  SECTION_ITEM                          *Section;

  if (Context == NULL) {
    return;
  }

  IniContext = Context;
  Section = IniContext->SectionHead;

  while (Section != NULL) {
    PtrSection = Section;
    Section = Section->PtrNext;
    if (PtrSection->PtrSection != NULL) {
      DEBUG((DEBUG_VERBOSE, "Section - %a\n", PtrSection->PtrSection));
    }
    if (PtrSection->PtrEntry != NULL) {
      DEBUG ((DEBUG_VERBOSE, "  Entry - %a\n", PtrSection->PtrEntry));
    }
    if (PtrSection->PtrValue != NULL) {
      DEBUG((DEBUG_VERBOSE, "  Value - %a\n", PtrSection->PtrValue));
    }
  }
}

/**
  Copy one line data from buffer data to the line buffer.

  @param[in]      Buffer          Buffer data.
  @param[in]      BufferSize      Buffer Size.
  @param[in, out] LineBuffer      Line buffer to store the found line data.
  @param[in, out] LineSize        On input, size of the input line buffer.
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
    if (*PtrBuf == 0x0D || *PtrBuf == 0x0A) {
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

  @param[in, out] Buffer          On input,  buffer data to be trimed.
                                  On output, the trimmed buffer.
  @param[in, out] BufferSize      On input,  size of original buffer data.
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

  @param[in]      Buffer          Comment buffer to be added.
  @param[in]      BufferSize      Size of comment buffer.
  @param[in, out] CommentHead     Comment Item head entry.

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

  CommentItem->PtrNext  = *CommentHead;
  *CommentHead          = CommentItem;

  //
  // Add a trailing '\0'
  //
  CommentItem->PtrComment = AllocatePool (BufferSize + 1);
  if (CommentItem->PtrComment == NULL) {
    FreePool (CommentItem);
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (CommentItem->PtrComment, Buffer, BufferSize);
  *(CommentItem->PtrComment + BufferSize) = '\0';

  return EFI_SUCCESS;
}

/**
  Add new section item into Section head.

  @param[in]      Buffer          Section item data buffer.
  @param[in]      BufferSize      Size of section item.
  @param[in, out] SectionHead     Section item head entry.

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
  SECTION_ITEM                          *SectionItem;
  UINTN                                 Length;
  UINT8                                 *PtrBuf;
  UINT8                                 *PtrEnd;

  ASSERT(BufferSize >= 1);
  //
  // The first character of Buffer is '[', now we want for ']'
  //
  PtrEnd      = (UINT8 *)((UINTN)Buffer + BufferSize - 1);
  PtrBuf      = (UINT8 *)((UINTN)Buffer + 1);
  while (PtrBuf <= PtrEnd) {
    if (*PtrBuf == ']') {
      break;
    }
    PtrBuf ++;
  }
  if (PtrBuf > PtrEnd) {
    //
    // Not found. Invalid line
    //
    return EFI_NOT_FOUND;
  }
  if (PtrBuf <= Buffer + 1) {
    // Empty name
    return EFI_NOT_FOUND;
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
  // Invalid line if the section name is null
  //
  if (Length == 0) {
    return EFI_NOT_FOUND;
  }

  if (!IsValidName((CHAR8 *)Buffer + 1, Length)) {
    return EFI_INVALID_PARAMETER;
  }

  SectionItem = AllocatePool (sizeof (SECTION_ITEM));
  if (SectionItem == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  SectionItem->PtrSection = NULL;
  SectionItem->SecNameLen = Length;
  SectionItem->PtrEntry   = NULL;
  SectionItem->PtrValue   = NULL;
  SectionItem->PtrNext    = *SectionHead;
  *SectionHead            = SectionItem;

  //
  // Add a trailing '\0'
  //
  SectionItem->PtrSection = AllocatePool (Length + 1);
  if (SectionItem->PtrSection == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // excluding the heading '['
  //
  CopyMem (SectionItem->PtrSection, Buffer + 1, Length);
  *(SectionItem->PtrSection + Length) = '\0';

  return EFI_SUCCESS;
}

/**
  Add new section entry and entry value into Section head.

  @param[in]      Buffer          Section entry data buffer.
  @param[in]      BufferSize      Size of section entry.
  @param[in, out] SectionHead     Section item head entry.

  @retval EFI_OUT_OF_RESOURCES   No enough memory is allocated.
  @retval EFI_SUCCESS            Section entry is added.
  @retval EFI_NOT_FOUND          Section entry is not found.
  @retval EFI_INVALID_PARAMETER  Section entry is invalid.

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
    // Not found. Invalid line
    //
    return EFI_NOT_FOUND;
  }
  if (PtrBuf <= Buffer) {
    // Empty name
    return EFI_NOT_FOUND;
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
  // Invalid line if the entry name is null
  //
  if (Length == 0) {
    return EFI_NOT_FOUND;
  }

  if (!IsValidName((CHAR8 *)Buffer, Length)) {
    return EFI_INVALID_PARAMETER;
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

  SectionItem->PtrSection = NULL;
  SectionItem->PtrEntry   = NULL;
  SectionItem->PtrValue   = NULL;
  SectionItem->SecNameLen = PtrSection->SecNameLen;
  SectionItem->PtrNext    = *SectionHead;
  *SectionHead            = SectionItem;

  //
  // SectionName, add a trailing '\0'
  //
  SectionItem->PtrSection = AllocatePool (PtrSection->SecNameLen + 1);
  if (SectionItem->PtrSection == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (SectionItem->PtrSection, PtrSection->PtrSection, PtrSection->SecNameLen + 1);

  //
  // EntryName, add a trailing '\0'
  //
  SectionItem->PtrEntry = AllocatePool (Length + 1);
  if (SectionItem->PtrEntry == NULL) {
    FreePool(SectionItem->PtrSection);
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (SectionItem->PtrEntry, Buffer, Length);
  *(SectionItem->PtrEntry + Length) = '\0';

  //
  // Next search for '#' or ';'
  //
  PtrBuf      = PtrBuf + 1;
  Buffer      = PtrBuf;
  while (PtrBuf <= PtrEnd) {
    if (*PtrBuf == '#' || *PtrBuf == ';') {
      break;
    }
    PtrBuf++;
  }
  if (PtrBuf <= Buffer) {
    // Empty name
    FreePool(SectionItem->PtrEntry);
    FreePool(SectionItem->PtrSection);
    return EFI_NOT_FOUND;
  }
  Length      = PtrBuf - Buffer;
  ProfileTrim (
    Buffer,
    &Length
  );

  //
  // Invalid line if the entry value is null
  //
  if (Length == 0) {
    FreePool(SectionItem->PtrEntry);
    FreePool(SectionItem->PtrSection);
    return EFI_NOT_FOUND;
  }

  if (!IsValidValue((CHAR8 *)Buffer, Length)) {
    FreePool(SectionItem->PtrEntry);
    FreePool(SectionItem->PtrSection);
    return EFI_INVALID_PARAMETER;
  }

  //
  // EntryValue, add a trailing '\0'
  //
  SectionItem->PtrValue = AllocatePool (Length + 1);
  if (SectionItem->PtrValue == NULL) {
    FreePool(SectionItem->PtrEntry);
    FreePool(SectionItem->PtrSection);
    return EFI_OUT_OF_RESOURCES;
  }
  CopyMem (SectionItem->PtrValue, Buffer, Length);
  *(SectionItem->PtrValue + Length) = '\0';

  return EFI_SUCCESS;
}

/**
  Free all comment entry and section entry.

  @param[in] Section         Section entry list.
  @param[in] Comment         Comment entry list.

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
    Section       = Section->PtrNext;
    if (PtrSection->PtrEntry != NULL) {
      FreePool (PtrSection->PtrEntry);
    }
    if (PtrSection->PtrSection != NULL) {
      FreePool (PtrSection->PtrSection);
    }
    if (PtrSection->PtrValue != NULL) {
      FreePool (PtrSection->PtrValue);
    }
    FreePool (PtrSection);
  }

  while (Comment != NULL) {
    PtrComment    = Comment;
    Comment       = Comment->PtrNext;
    if (PtrComment->PtrComment != NULL) {
      FreePool (PtrComment->PtrComment);
    }
    FreePool (PtrComment);
  }

  return;
}

/**
  Get section entry value.

  @param[in]  Section         Section entry list.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] EntryValue      Point to the got entry value.

  @retval EFI_NOT_FOUND  Section is not found.
  @retval EFI_SUCCESS    Section entry value is got.

**/
EFI_STATUS
UpdateGetProfileString (
  IN      SECTION_ITEM                  *Section,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     CHAR8                         **EntryValue
  )
{
  *EntryValue   = NULL;

  while (Section != NULL) {
    if (AsciiStrCmp ((CONST CHAR8 *) Section->PtrSection, (CONST CHAR8 *) SectionName) == 0) {
      if (Section->PtrEntry != NULL) {
        if (AsciiStrCmp ((CONST CHAR8 *) Section->PtrEntry, (CONST CHAR8 *) EntryName) == 0) {
          break;
        }
      }
    }
    Section     = Section->PtrNext;
  }

  if (Section == NULL) {
    return EFI_NOT_FOUND;
  }

  *EntryValue   = Section->PtrValue;

  return EFI_SUCCESS;
}

/**
  Pre process config data buffer into Section entry list and Comment entry list.

  @param[in]      DataBuffer      Config raw file buffer.
  @param[in]      BufferSize      Size of raw buffer.
  @param[in, out] SectionHead     Pointer to the section entry list.
  @param[in, out] CommentHead     Pointer to the comment entry list.

  @retval EFI_OUT_OF_RESOURCES  No enough memory is allocated.
  @retval EFI_SUCCESS           Config data buffer is preprocessed.
  @retval EFI_NOT_FOUND         Config data buffer is invalid, because Section or Entry is not found.
  @retval EFI_INVALID_PARAMETER Config data buffer is invalid, because Section or Entry is invalid.

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

    if (PtrLine[0] == '#' || PtrLine[0] == ';') {
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
  Open an INI config file and return a context.

  @param[in] DataBuffer      Config raw file buffer.
  @param[in] BufferSize      Size of raw buffer.

  @return       Config data buffer is opened and context is returned.
  @retval NULL  No enough memory is allocated.
  @retval NULL  Config data buffer is invalid.
**/
VOID *
EFIAPI
OpenIniFile (
  IN      UINT8                         *DataBuffer,
  IN      UINTN                         BufferSize
  )
{
  EFI_STATUS                            Status;
  INI_PARSING_LIB_CONTEXT               *IniContext;

  if (DataBuffer == NULL || BufferSize == 0) {
    return NULL;
  }

  IniContext = AllocateZeroPool(sizeof(INI_PARSING_LIB_CONTEXT));
  if (IniContext == NULL) {
    return NULL;
  }

  //
  // First process the data buffer and get all sections and entries
  //
  Status = PreProcessDataFile (
             DataBuffer,
             BufferSize,
             &IniContext->SectionHead,
             &IniContext->CommentHead
             );
  if (EFI_ERROR(Status)) {
    FreePool(IniContext);
    return NULL;
  }
  DEBUG_CODE_BEGIN ();
    DumpIniSection(IniContext);
  DEBUG_CODE_END ();
  return IniContext;
}

/**
  Get section entry string value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] EntryValue      Point to the got entry string value.

  @retval EFI_SUCCESS    Section entry string value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetStringFromDataFile(
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     CHAR8                         **EntryValue
  )
{
  INI_PARSING_LIB_CONTEXT               *IniContext;
  EFI_STATUS                            Status;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || EntryValue == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  IniContext = Context;

  *EntryValue  = NULL;
  Status = UpdateGetProfileString (
             IniContext->SectionHead,
             SectionName,
             EntryName,
             EntryValue
             );
  return Status;
}

/**
  Get section entry GUID value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Guid            Point to the got GUID value.

  @retval EFI_SUCCESS    Section entry GUID value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetGuidFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     EFI_GUID                      *Guid
  )
{
  CHAR8                                 *Value;
  EFI_STATUS                            Status;
  RETURN_STATUS                         RStatus;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || Guid == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetStringFromDataFile(
             Context,
             SectionName,
             EntryName,
             &Value
             );
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }
  ASSERT (Value != NULL);
  RStatus = AsciiStrToGuid (Value, Guid);
  if (RETURN_ERROR (RStatus) || (Value[GUID_STRING_LENGTH] != '\0')) {
    return EFI_NOT_FOUND;
  }
  return EFI_SUCCESS;
}

/**
  Get section entry decimal UINTN value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got decimal UINTN value.

  @retval EFI_SUCCESS    Section entry decimal UINTN value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetDecimalUintnFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     UINTN                         *Data
  )
{
  CHAR8                                 *Value;
  EFI_STATUS                            Status;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetStringFromDataFile(
             Context,
             SectionName,
             EntryName,
             &Value
             );
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }
  ASSERT (Value != NULL);
  if (!IsValidDecimalString(Value, AsciiStrLen(Value))) {
    return EFI_NOT_FOUND;
  }
  *Data = AsciiStrDecimalToUintn(Value);
  return EFI_SUCCESS;
}

/**
  Get section entry heximal UINTN value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got heximal UINTN value.

  @retval EFI_SUCCESS    Section entry heximal UINTN value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetHexUintnFromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     UINTN                         *Data
  )
{
  CHAR8                                 *Value;
  EFI_STATUS                            Status;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetStringFromDataFile(
             Context,
             SectionName,
             EntryName,
             &Value
             );
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }
  ASSERT (Value != NULL);
  if (!IsValidHexString(Value, AsciiStrLen(Value))) {
    return EFI_NOT_FOUND;
  }
  *Data = AsciiStrHexToUintn(Value);
  return EFI_SUCCESS;
}

/**
  Get section entry heximal UINT64 value.

  @param[in]  Context         INI Config file context.
  @param[in]  SectionName     Section name.
  @param[in]  EntryName       Section entry name.
  @param[out] Data            Point to the got heximal UINT64 value.

  @retval EFI_SUCCESS    Section entry heximal UINT64 value is got.
  @retval EFI_NOT_FOUND  Section is not found.
**/
EFI_STATUS
EFIAPI
GetHexUint64FromDataFile (
  IN      VOID                          *Context,
  IN      CHAR8                         *SectionName,
  IN      CHAR8                         *EntryName,
  OUT     UINT64                        *Data
  )
{
  CHAR8                                 *Value;
  EFI_STATUS                            Status;

  if (Context == NULL || SectionName == NULL || EntryName == NULL || Data == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = GetStringFromDataFile(
             Context,
             SectionName,
             EntryName,
             &Value
             );
  if (EFI_ERROR(Status)) {
    return EFI_NOT_FOUND;
  }
  ASSERT (Value != NULL);
  if (!IsValidHexString(Value, AsciiStrLen(Value))) {
    return EFI_NOT_FOUND;
  }
  *Data = AsciiStrHexToUint64(Value);
  return EFI_SUCCESS;
}

/**
  Close an INI config file and free the context.

  @param[in] Context         INI Config file context.
**/
VOID
EFIAPI
CloseIniFile (
  IN      VOID                          *Context
  )
{
  INI_PARSING_LIB_CONTEXT               *IniContext;

  if (Context == NULL) {
    return ;
  }

  IniContext = Context;
  FreeAllList(IniContext->SectionHead, IniContext->CommentHead);

  return;
}
