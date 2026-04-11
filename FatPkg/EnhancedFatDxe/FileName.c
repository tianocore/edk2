/** @file
  Functions for manipulating file names.

Copyright (c) 2005 - 2015, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Fat.h"

/**

  This function checks whether the input FileName is a valid 8.3 short name.
  If the input FileName is a valid 8.3, the output is the 8.3 short name;
  otherwise, the output is the base tag of 8.3 short name.

  @param  FileName              - The input unicode filename.
  @param  File8Dot3Name         - The output ascii 8.3 short name or base tag of 8.3 short name.

  @retval TRUE                  - The input unicode filename is a valid 8.3 short name.
  @retval FALSE                 - The input unicode filename is not a valid 8.3 short name.

**/
BOOLEAN
FatCheckIs8Dot3Name (
  IN  CHAR16  *FileName,
  OUT CHAR8   *File8Dot3Name
  )
{
  BOOLEAN  PossibleShortName;
  CHAR16   *TempName;
  CHAR16   *ExtendName;
  CHAR16   *SeparateDot;
  UINTN    MainNameLen;
  UINTN    ExtendNameLen;

  PossibleShortName = TRUE;
  SeparateDot       = NULL;
  SetMem (File8Dot3Name, FAT_NAME_LEN, ' ');
  for (TempName = FileName; *TempName != '\0'; TempName++) {
    if (*TempName == L'.') {
      SeparateDot = TempName;
    }
  }

  if (SeparateDot == NULL) {
    //
    // Extended filename is not detected
    //
    MainNameLen   = TempName - FileName;
    ExtendName    = TempName;
    ExtendNameLen = 0;
  } else {
    //
    // Extended filename is detected
    //
    MainNameLen   = SeparateDot - FileName;
    ExtendName    = SeparateDot + 1;
    ExtendNameLen = TempName - ExtendName;
  }

  //
  // We scan the filename for the second time
  // to check if there exists any extra blanks and dots
  //
  while (--TempName >= FileName) {
    if (((*TempName == L'.') || (*TempName == L' ')) && (TempName != SeparateDot)) {
      //
      // There exist extra blanks and dots
      //
      PossibleShortName = FALSE;
    }
  }

  if (MainNameLen == 0) {
    PossibleShortName = FALSE;
  }

  if (MainNameLen > FAT_MAIN_NAME_LEN) {
    PossibleShortName = FALSE;
    MainNameLen       = FAT_MAIN_NAME_LEN;
  }

  if (ExtendNameLen > FAT_EXTEND_NAME_LEN) {
    PossibleShortName = FALSE;
    ExtendNameLen     = FAT_EXTEND_NAME_LEN;
  }

  if (FatStrToFat (FileName, MainNameLen, File8Dot3Name)) {
    PossibleShortName = FALSE;
  }

  if (FatStrToFat (ExtendName, ExtendNameLen, File8Dot3Name + FAT_MAIN_NAME_LEN)) {
    PossibleShortName = FALSE;
  }

  return PossibleShortName;
}

/**

  Trim the trailing blanks of fat name.

  @param  Name                  - The Char8 string needs to be trimmed.
  @param  Len                   - The length of the fat name.

  The real length of the fat name after the trailing blanks are trimmed.

**/
STATIC
UINTN
FatTrimAsciiTrailingBlanks (
  IN CHAR8  *Name,
  IN UINTN  Len
  )
{
  while (Len > 0 && Name[Len - 1] == ' ') {
    Len--;
  }

  return Len;
}

/**

  Convert the ascii fat name to the unicode string and strip trailing spaces,
  and if necessary, convert the unicode string to lower case.

  @param  FatName               - The Char8 string needs to be converted.
  @param  Len                   - The length of the fat name.
  @param  LowerCase             - Indicate whether to convert the string to lower case.
  @param  Str                   - The result of the conversion.

**/
VOID
FatNameToStr (
  IN  CHAR8   *FatName,
  IN  UINTN   Len,
  IN  UINTN   LowerCase,
  OUT CHAR16  *Str
  )
{
  //
  // First, trim the trailing blanks
  //
  Len = FatTrimAsciiTrailingBlanks (FatName, Len);
  //
  // Convert fat string to unicode string
  //
  FatFatToStr (Len, FatName, Str);

  //
  // If the name is to be lower cased, do it now
  //
  if (LowerCase != 0) {
    FatStrLwr (Str);
  }
}

/**

  This function generates 8Dot3 name from user specified name for a newly created file.

  @param  Parent                - The parent directory.
  @param  DirEnt                - The directory entry whose 8Dot3Name needs to be generated.

**/
VOID
FatCreate8Dot3Name (
  IN FAT_OFILE   *Parent,
  IN FAT_DIRENT  *DirEnt
  )
{
  CHAR8  *ShortName;
  CHAR8  *ShortNameChar;
  UINTN  BaseTagLen;
  UINTN  Index;
  UINTN  Retry;
  UINT8  Segment;

  union {
    UINT32    Crc;
    struct HEX_DATA {
      UINT8    Segment : HASH_VALUE_TAG_LEN;
    } Hex[HASH_VALUE_TAG_LEN];
  } HashValue;
  //
  // Make sure the whole directory has been loaded
  //
  ASSERT (Parent->ODir->EndOfDir);
  ShortName = DirEnt->Entry.FileName;

  //
  // Trim trailing blanks of 8.3 name
  //
  BaseTagLen = FatTrimAsciiTrailingBlanks (ShortName, FAT_MAIN_NAME_LEN);
  if (BaseTagLen > SPEC_BASE_TAG_LEN) {
    BaseTagLen = SPEC_BASE_TAG_LEN;
  }

  //
  // We first use the algorithm described by spec.
  //
  ShortNameChar    = ShortName + BaseTagLen;
  *ShortNameChar++ = '~';
  *ShortNameChar   = '1';
  Retry            = 0;
  while (*FatShortNameHashSearch (Parent->ODir, ShortName) != NULL) {
    *ShortNameChar = (CHAR8)(*ShortNameChar + 1);
    if (++Retry == MAX_SPEC_RETRY) {
      //
      // We use new algorithm to generate 8.3 name
      //
      ASSERT (DirEnt->FileString != NULL);
      gBS->CalculateCrc32 (DirEnt->FileString, StrSize (DirEnt->FileString), &HashValue.Crc);

      if (BaseTagLen > HASH_BASE_TAG_LEN) {
        BaseTagLen = HASH_BASE_TAG_LEN;
      }

      ShortNameChar = ShortName + BaseTagLen;
      for (Index = 0; Index < HASH_VALUE_TAG_LEN; Index++) {
        Segment = HashValue.Hex[Index].Segment;
        if (Segment > 9) {
          *ShortNameChar++ = (CHAR8)(Segment - 10 + 'A');
        } else {
          *ShortNameChar++ = (CHAR8)(Segment + '0');
        }
      }

      *ShortNameChar++ = '~';
      *ShortNameChar   = '1';
    }
  }
}

/**

  Check the string is lower case or upper case
  and it is used by fatname to dir entry count

  @param Str                   - The string which needs to be checked.
  @param InCaseFlag            - The input case flag which is returned when the string is lower case.

  @retval OutCaseFlag           - The output case flag.

**/
STATIC
UINT8
FatCheckNameCase (
  IN CHAR16  *Str,
  IN UINT8   InCaseFlag
  )
{
  CHAR16  Buffer[FAT_MAIN_NAME_LEN + 1 + FAT_EXTEND_NAME_LEN + 1];
  UINT8   OutCaseFlag;

  //
  // Assume the case of input string is mixed
  //
  OutCaseFlag = FAT_CASE_MIXED;
  //
  // Lower case a copy of the string, if it matches the
  // original then the string is lower case
  //
  StrCpyS (Buffer, ARRAY_SIZE (Buffer), Str);
  FatStrLwr (Buffer);
  if (StrCmp (Str, Buffer) == 0) {
    OutCaseFlag = InCaseFlag;
  }

  //
  // Upper case a copy of the string, if it matches the
  // original then the string is upper case
  //
  StrCpyS (Buffer, ARRAY_SIZE (Buffer), Str);
  FatStrUpr (Buffer);
  if (StrCmp (Str, Buffer) == 0) {
    OutCaseFlag = 0;
  }

  return OutCaseFlag;
}

/**

  Set the caseflag value for the directory entry.

  @param DirEnt                - The logical directory entry whose caseflag value is to be set.

**/
VOID
FatSetCaseFlag (
  IN FAT_DIRENT  *DirEnt
  )
{
  CHAR16  LfnBuffer[FAT_MAIN_NAME_LEN + 1 + FAT_EXTEND_NAME_LEN + 1];
  CHAR16  *TempCharPtr;
  CHAR16  *ExtendName;
  CHAR16  *FileNameCharPtr;
  UINT8   CaseFlag;

  ExtendName      = NULL;
  TempCharPtr     = LfnBuffer;
  FileNameCharPtr = DirEnt->FileString;
  ASSERT (StrSize (DirEnt->FileString) <= sizeof (LfnBuffer));
  while ((*TempCharPtr = *FileNameCharPtr) != 0) {
    if (*TempCharPtr == L'.') {
      ExtendName = TempCharPtr;
    }

    TempCharPtr++;
    FileNameCharPtr++;
  }

  CaseFlag = 0;
  if (ExtendName != NULL) {
    *ExtendName = 0;
    ExtendName++;
    CaseFlag = (UINT8)(CaseFlag | FatCheckNameCase (ExtendName, FAT_CASE_EXT_LOWER));
  }

  CaseFlag = (UINT8)(CaseFlag | FatCheckNameCase (LfnBuffer, FAT_CASE_NAME_LOWER));
  if ((CaseFlag & FAT_CASE_MIXED) == 0) {
    //
    // We just need one directory entry to store this file name entry
    //
    DirEnt->Entry.CaseFlag = CaseFlag;
  } else {
    //
    // We need one extra directory entry to store the mixed case entry
    //
    DirEnt->Entry.CaseFlag = 0;
    DirEnt->EntryCount++;
  }
}

/**

  Convert the 8.3 ASCII fat name to cased Unicode string according to case flag.

  @param  DirEnt                - The corresponding directory entry.
  @param  FileString            - The output Unicode file name.
  @param  FileStringMax           The max length of FileString.

**/
VOID
FatGetFileNameViaCaseFlag (
  IN     FAT_DIRENT  *DirEnt,
  IN OUT CHAR16      *FileString,
  IN     UINTN       FileStringMax
  )
{
  UINT8   CaseFlag;
  CHAR8   *File8Dot3Name;
  CHAR16  TempExt[1 + FAT_EXTEND_NAME_LEN + 1];

  //
  // Store file extension like ".txt"
  //
  CaseFlag      = DirEnt->Entry.CaseFlag;
  File8Dot3Name = DirEnt->Entry.FileName;

  FatNameToStr (File8Dot3Name, FAT_MAIN_NAME_LEN, CaseFlag & FAT_CASE_NAME_LOWER, FileString);
  FatNameToStr (File8Dot3Name + FAT_MAIN_NAME_LEN, FAT_EXTEND_NAME_LEN, CaseFlag & FAT_CASE_EXT_LOWER, &TempExt[1]);
  if (TempExt[1] != 0) {
    TempExt[0] = L'.';
    StrCatS (FileString, FileStringMax, TempExt);
  }
}

/**

  Get the Check sum for a short name.

  @param  ShortNameString       - The short name for a file.

  @retval Sum                   - UINT8 checksum.

**/
UINT8
FatCheckSum (
  IN CHAR8  *ShortNameString
  )
{
  UINTN  ShortNameLen;
  UINT8  Sum;

  Sum = 0;
  for (ShortNameLen = FAT_NAME_LEN; ShortNameLen != 0; ShortNameLen--) {
    Sum = (UINT8)((((Sum & 1) != 0) ? 0x80 : 0) + (Sum >> 1) + *ShortNameString++);
  }

  return Sum;
}

/**

  Takes Path as input, returns the next name component
  in Name, and returns the position after Name (e.g., the
  start of the next name component)

  @param  Path                  - The path of one file.
  @param  Name                  - The next name component in Path.

  The position after Name in the Path

**/
CHAR16 *
FatGetNextNameComponent (
  IN  CHAR16  *Path,
  OUT CHAR16  *Name
  )
{
  while (*Path != 0 && *Path != PATH_NAME_SEPARATOR) {
    *Name++ = *Path++;
  }

  *Name = 0;
  //
  // Get off of trailing path name separator
  //
  while (*Path == PATH_NAME_SEPARATOR) {
    Path++;
  }

  return Path;
}

/**

  Check whether the IFileName is valid long file name. If the IFileName is a valid
  long file name, then we trim the possible leading blanks and leading/trailing dots.
  the trimmed filename is stored in OutputFileName

  @param  InputFileName         - The input file name.
  @param  OutputFileName        - The output file name.

  @retval TRUE                  - The InputFileName is a valid long file name.
  @retval FALSE                 - The InputFileName is not a valid long file name.

**/
BOOLEAN
FatFileNameIsValid (
  IN  CHAR16  *InputFileName,
  OUT CHAR16  *OutputFileName
  )
{
  CHAR16  *TempNamePointer;
  CHAR16  TempChar;

  //
  // Trim Leading blanks
  //
  while (*InputFileName == L' ') {
    InputFileName++;
  }

  TempNamePointer = OutputFileName;
  while (*InputFileName != 0) {
    *TempNamePointer++ = *InputFileName++;
  }

  //
  // Trim Trailing blanks and dots
  //
  while (TempNamePointer > OutputFileName) {
    TempChar = *(TempNamePointer - 1);
    if ((TempChar != L' ') && (TempChar != L'.')) {
      break;
    }

    TempNamePointer--;
  }

  *TempNamePointer = 0;

  //
  // Per FAT Spec the file name should meet the following criteria:
  //   C1. Length (FileLongName) <= 255
  //   C2. Length (X:FileFullPath<NUL>) <= 260
  // Here we check C1.
  //
  if (TempNamePointer - OutputFileName > EFI_FILE_STRING_LENGTH) {
    return FALSE;
  }

  //
  // See if there is any illegal characters within the name
  //
  do {
    if ((*OutputFileName < 0x20) ||
        (*OutputFileName == '\"') ||
        (*OutputFileName == '*') ||
        (*OutputFileName == '/') ||
        (*OutputFileName == ':') ||
        (*OutputFileName == '<') ||
        (*OutputFileName == '>') ||
        (*OutputFileName == '?') ||
        (*OutputFileName == '\\') ||
        (*OutputFileName == '|')
        )
    {
      return FALSE;
    }

    OutputFileName++;
  } while (*OutputFileName != 0);

  return TRUE;
}
