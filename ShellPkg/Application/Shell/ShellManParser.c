/** @file
  Provides interface to shell MAN file parser.

  Copyright (c) 2009 - 2016, Intel Corporation. All rights reserved.<BR>
  Copyright 2015 Dell Inc.
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Shell.h"

#define SHELL_MAN_HII_GUID \
{ \
  0xf62ccd0c, 0x2449, 0x453c, { 0x8a, 0xcb, 0x8c, 0xc5, 0x7c, 0xf0, 0x2a, 0x97 } \
}

EFI_HII_HANDLE  mShellManHiiHandle    = NULL;
EFI_HANDLE      mShellManDriverHandle = NULL;


SHELL_MAN_HII_VENDOR_DEVICE_PATH  mShellManHiiDevicePath = {
  {
    {
      HARDWARE_DEVICE_PATH,
      HW_VENDOR_DP,
      {
        (UINT8) (sizeof (VENDOR_DEVICE_PATH)),
        (UINT8) ((sizeof (VENDOR_DEVICE_PATH)) >> 8)
      }
    },
    SHELL_MAN_HII_GUID
  },
  {
    END_DEVICE_PATH_TYPE,
    END_ENTIRE_DEVICE_PATH_SUBTYPE,
    {
      (UINT8) (END_DEVICE_PATH_LENGTH),
      (UINT8) ((END_DEVICE_PATH_LENGTH) >> 8)
    }
  }
};


/**
  Convert a Unicode character to upper case only if
  it maps to a valid small-case ASCII character.

  This internal function only deal with Unicode character
  which maps to a valid small-case ASCII character, i.e.
  L'a' to L'z'. For other Unicode character, the input character
  is returned directly.

  @param  Char  The character to convert.

  @retval LowerCharacter   If the Char is with range L'a' to L'z'.
  @retval Unchanged        Otherwise.

**/
CHAR16
EFIAPI 
InternalShellCharToUpper (
  IN CHAR16  Char
  );

/**
  Verifies that the filename has .EFI on the end.

  allocates a new buffer and copies the name (appending .EFI if necessary).
  Caller to free the buffer.

  @param[in] NameString            original name string

  @return                          the new filename with .efi as the extension.
**/
CHAR16 *
EFIAPI
GetExecuatableFileName (
  IN CONST CHAR16    *NameString
  )
{
  CHAR16  *Buffer;
  CHAR16  *SuffixStr;
  if (NameString == NULL) {
    return (NULL);
  }

  //
  // Fix the file name
  //
  if (StrnCmp(NameString+StrLen(NameString)-StrLen(L".efi"), L".efi", StrLen(L".efi"))==0) {
    Buffer = AllocateCopyPool(StrSize(NameString), NameString);
  } else if (StrnCmp(NameString+StrLen(NameString)-StrLen(L".man"), L".man", StrLen(L".man"))==0) {
    Buffer = AllocateCopyPool(StrSize(NameString), NameString);
    if (Buffer != NULL) {
      SuffixStr = Buffer+StrLen(Buffer)-StrLen(L".man");
      StrnCpyS (SuffixStr, StrSize(L".man")/sizeof(CHAR16), L".efi", StrLen(L".efi"));
    }
  } else {
    Buffer = AllocateZeroPool(StrSize(NameString) + StrLen(L".efi")*sizeof(CHAR16));
    if (Buffer != NULL) {
      StrnCpyS( Buffer,
                (StrSize(NameString) + StrLen(L".efi")*sizeof(CHAR16))/sizeof(CHAR16),
                NameString,
                StrLen(NameString)
                );
      StrnCatS( Buffer,
                (StrSize(NameString) + StrLen(L".efi")*sizeof(CHAR16))/sizeof(CHAR16),
                L".efi",
                StrLen(L".efi")
                );
    }
  }
  return (Buffer);

}

/**
  Verifies that the filename has .MAN on the end.

  allocates a new buffer and copies the name (appending .MAN if necessary)

  ASSERT if ManFileName is NULL

  @param[in] ManFileName            original filename

  @return the new filename with .man as the extension.
**/
CHAR16 *
EFIAPI
GetManFileName(
  IN CONST CHAR16 *ManFileName
  )
{
  CHAR16 *Buffer;
  if (ManFileName == NULL) {
    return (NULL);
  }
  //
  // Fix the file name
  //
  if (StrnCmp(ManFileName+StrLen(ManFileName)-4, L".man", 4)==0) {
    Buffer = AllocateCopyPool(StrSize(ManFileName), ManFileName);
  } else {
    Buffer = AllocateZeroPool(StrSize(ManFileName) + 4*sizeof(CHAR16));
    if (Buffer != NULL) {
      StrnCpyS( Buffer, 
                (StrSize(ManFileName) + 4*sizeof(CHAR16))/sizeof(CHAR16), 
                ManFileName, 
                StrLen(ManFileName)
                );
      StrnCatS( Buffer, 
                (StrSize(ManFileName) + 4*sizeof(CHAR16))/sizeof(CHAR16),
                L".man", 
                4
                );
    }
  }
  return (Buffer);
}

/**
  Search the path environment variable for possible locations and test for
  which one contains a man file with the name specified.  If a valid file is found
  stop searching and return the (opened) SHELL_FILE_HANDLE for that file.

  @param[in] FileName           Name of the file to find and open.
  @param[out] Handle            Pointer to the handle of the found file.  The
                                value of this is undefined for return values
                                except EFI_SUCCESS.

  @retval EFI_SUCCESS           The file was found.  Handle is a valid SHELL_FILE_HANDLE
  @retval EFI_INVALID_PARAMETER A parameter had an invalid value.
  @retval EFI_NOT_FOUND         The file was not found.
**/
EFI_STATUS
EFIAPI
SearchPathForFile(
  IN CONST CHAR16             *FileName,
  OUT SHELL_FILE_HANDLE       *Handle
  )
{
  CHAR16          *FullFileName;
  EFI_STATUS      Status;

  if ( FileName     == NULL
    || Handle       == NULL
    || StrLen(FileName) == 0
   ){
    return (EFI_INVALID_PARAMETER);
  }

  FullFileName = ShellFindFilePath(FileName);
  if (FullFileName == NULL) {
    return (EFI_NOT_FOUND);
  }

  //
  // now open that file
  //
  Status = EfiShellOpenFileByName(FullFileName, Handle, EFI_FILE_MODE_READ);
  FreePool(FullFileName);

  return (Status);
}

/**
  parses through Buffer (which is MAN file formatted) and returns the
  detailed help for any sub section specified in the comma seperated list of
  sections provided.  If the end of the file or a .TH section is found then
  return.

  Upon a sucessful return the caller is responsible to free the memory in *HelpText

  @param[in] Buffer             Buffer to read from
  @param[in] Sections           name of command's sub sections to find
  @param[in] HelpText           pointer to pointer to string where text goes.
  @param[in] HelpSize           pointer to size of allocated HelpText (may be updated)

  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
  @retval EFI_SUCCESS           the section was found and its description sotred in
                                an alloceted buffer.
**/
EFI_STATUS
EFIAPI
ManBufferFindSections(
  IN CONST CHAR16 *Buffer,
  IN CONST CHAR16 *Sections,
  IN CHAR16       **HelpText,
  IN UINTN        *HelpSize
  )
{
  EFI_STATUS          Status;
  CONST CHAR16        *CurrentLocation;
  BOOLEAN             CurrentlyReading;
  CHAR16              *SectionName;
  UINTN               SectionLen;
  BOOLEAN             Found;
  CHAR16              *TempString;
  CHAR16              *TempString2;

  if ( Buffer     == NULL
    || HelpText   == NULL
    || HelpSize   == NULL
   ){
    return (EFI_INVALID_PARAMETER);
  }

  Status            = EFI_SUCCESS;
  CurrentlyReading  = FALSE;
  Found             = FALSE;

  for (CurrentLocation = Buffer,TempString = NULL
    ;  CurrentLocation != NULL && *CurrentLocation != CHAR_NULL
    ;  CurrentLocation=StrStr(CurrentLocation, L"\r\n"),TempString = NULL
   ){
    while(CurrentLocation[0] == L'\r' || CurrentLocation[0] == L'\n') {
      CurrentLocation++;
    }
    if (CurrentLocation[0] == L'#') {
      //
      // Skip comment lines
      //
      continue;
    }
    if (StrnCmp(CurrentLocation, L".TH", 3) == 0) {
      //
      // we hit the end of this commands section so stop.
      //
      break;
    }
    if (StrnCmp(CurrentLocation, L".SH ", 4) == 0) {
      if (Sections == NULL) {
        CurrentlyReading = TRUE;
        continue;
      } else if (CurrentlyReading) {
        CurrentlyReading = FALSE;
      }
      CurrentLocation += 4;
      //
      // is this a section we want to read in?
      //
      if (StrLen(CurrentLocation)!=0) {
        TempString2 = StrStr(CurrentLocation, L" ");
        TempString2 = MIN(TempString2, StrStr(CurrentLocation, L"\r"));
        TempString2 = MIN(TempString2, StrStr(CurrentLocation, L"\n"));
        ASSERT(TempString == NULL);
        TempString = StrnCatGrow(&TempString, NULL, CurrentLocation, TempString2==NULL?0:TempString2 - CurrentLocation);
        if (TempString == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }
        SectionName = TempString;
        SectionLen = StrLen(SectionName);
        SectionName = StrStr(Sections, SectionName);
        if (SectionName == NULL) {
          SHELL_FREE_NON_NULL(TempString);
          continue;
        }
        if (*(SectionName + SectionLen) == CHAR_NULL || *(SectionName + SectionLen) == L',') {
          CurrentlyReading = TRUE;
        }
      }
    } else if (CurrentlyReading) {
      Found = TRUE;
      if (StrLen(CurrentLocation)!=0) {
        TempString2 = StrStr(CurrentLocation, L"\r");
        TempString2 = MIN(TempString2, StrStr(CurrentLocation, L"\n"));
        ASSERT(TempString == NULL);
        TempString = StrnCatGrow(&TempString, NULL, CurrentLocation, TempString2==NULL?0:TempString2 - CurrentLocation);
        if (TempString == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }
        //
        // copy and save the current line.
        //
        ASSERT((*HelpText == NULL && *HelpSize == 0) || (*HelpText != NULL));
        StrnCatGrow (HelpText, HelpSize, TempString, 0);
        if (HelpText == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }
        StrnCatGrow (HelpText, HelpSize, L"\r\n", 0);
        if (HelpText == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
          break;
        }
      }
    }
    SHELL_FREE_NON_NULL(TempString);
  }
  SHELL_FREE_NON_NULL(TempString);
  if (!Found && !EFI_ERROR(Status)) {
    return (EFI_NOT_FOUND);
  }
  return (Status);
}

/**
  parses through the MAN file specified by SHELL_FILE_HANDLE and returns the
  detailed help for any sub section specified in the comma seperated list of
  sections provided.  If the end of the file or a .TH section is found then
  return.

  Upon a sucessful return the caller is responsible to free the memory in *HelpText

  @param[in] Handle             FileHandle to read from
  @param[in] Sections           name of command's sub sections to find
  @param[out] HelpText          pointer to pointer to string where text goes.
  @param[out] HelpSize          pointer to size of allocated HelpText (may be updated)
  @param[in] Ascii              TRUE if the file is ASCII, FALSE otherwise.

  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
  @retval EFI_SUCCESS           the section was found and its description sotred in
                                an alloceted buffer.
**/
EFI_STATUS
EFIAPI
ManFileFindSections(
  IN SHELL_FILE_HANDLE  Handle,
  IN CONST CHAR16       *Sections,
  OUT CHAR16            **HelpText,
  OUT UINTN             *HelpSize,
  IN BOOLEAN            Ascii
  )
{
  EFI_STATUS          Status;
  CHAR16              *ReadLine;
  UINTN               Size;
  BOOLEAN             CurrentlyReading;
  CHAR16              *SectionName;
  UINTN               SectionLen;
  BOOLEAN             Found;

  if ( Handle     == NULL
    || HelpText   == NULL
    || HelpSize   == NULL
   ){
    return (EFI_INVALID_PARAMETER);
  }

  Status            = EFI_SUCCESS;
  CurrentlyReading  = FALSE;
  Size              = 1024;
  Found             = FALSE;

  ReadLine          = AllocateZeroPool(Size);
  if (ReadLine == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  for (;!ShellFileHandleEof(Handle);Size = 1024) {
    Status = ShellFileHandleReadLine(Handle, ReadLine, &Size, TRUE, &Ascii);
    if (ReadLine[0] == L'#') {
      //
      // Skip comment lines
      //
      continue;
    }
    //
    // ignore too small of buffer...
    //
    if (Status == EFI_BUFFER_TOO_SMALL) {
      Status = EFI_SUCCESS;
    }
    if (EFI_ERROR(Status)) {
      break;
    } else if (StrnCmp(ReadLine, L".TH", 3) == 0) {
      //
      // we hit the end of this commands section so stop.
      //
      break;
    } else if (StrnCmp(ReadLine, L".SH", 3) == 0) {
      if (Sections == NULL) {
        CurrentlyReading = TRUE;
        continue;
      }
      //
      // we found a section
      //
      if (CurrentlyReading) {
        CurrentlyReading = FALSE;
      }
      //
      // is this a section we want to read in?
      //
      for ( SectionName = ReadLine + 3
          ; *SectionName == L' '
          ; SectionName++);
      SectionLen = StrLen(SectionName);
      SectionName = StrStr(Sections, SectionName);
      if (SectionName == NULL) {
        continue;
      }
      if (*(SectionName + SectionLen) == CHAR_NULL || *(SectionName + SectionLen) == L',') {
        CurrentlyReading = TRUE;
      }
    } else if (CurrentlyReading) {
      Found = TRUE;
      //
      // copy and save the current line.
      //
      ASSERT((*HelpText == NULL && *HelpSize == 0) || (*HelpText != NULL));
      StrnCatGrow (HelpText, HelpSize, ReadLine, 0);
      StrnCatGrow (HelpText, HelpSize, L"\r\n", 0);
    }
  }
  FreePool(ReadLine);
  if (!Found && !EFI_ERROR(Status)) {
    return (EFI_NOT_FOUND);
  }
  return (Status);
}

/**
  parses through the MAN file formatted Buffer and returns the
  "Brief Description" for the .TH section as specified by Command.  If the
  command section is not found return EFI_NOT_FOUND.

  Upon a sucessful return the caller is responsible to free the memory in *BriefDesc

  @param[in] Buffer             Buffer to read from
  @param[in] Command            name of command's section to find
  @param[in] BriefDesc          pointer to pointer to string where description goes.
  @param[in] BriefSize          pointer to size of allocated BriefDesc

  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
  @retval EFI_SUCCESS           the section was found and its description sotred in
                                an alloceted buffer.
**/
EFI_STATUS
EFIAPI
ManBufferFindTitleSection(
  IN CHAR16         **Buffer,
  IN CONST CHAR16   *Command,
  IN CHAR16         **BriefDesc,
  IN UINTN          *BriefSize
  )
{
  EFI_STATUS    Status;
  CHAR16        *TitleString;
  CHAR16        *TitleEnd;
  CHAR16        *CurrentLocation;
  UINTN         TitleLength;
  UINTN         Start;
  CONST CHAR16  StartString[] = L".TH ";
  CONST CHAR16  EndString[]   = L" 0 ";

  if ( Buffer     == NULL
    || Command    == NULL
    || (BriefDesc != NULL && BriefSize == NULL)
   ){
    return (EFI_INVALID_PARAMETER);
  }

  Status    = EFI_SUCCESS;

  //
  // Do not pass any leading path information that may be present to IsTitleHeader().
  //
  Start = StrLen(Command);
  while ((Start != 0)
         && (*(Command + Start - 1) != L'\\')
         && (*(Command + Start - 1) != L'/')
         && (*(Command + Start - 1) != L':')) {
    --Start;
  }

  //
  // more characters for StartString and EndString
  //
  TitleLength = StrSize(Command + Start) + (StrLen(StartString) + StrLen(EndString)) * sizeof(CHAR16);
  TitleString = AllocateZeroPool(TitleLength);
  if (TitleString == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }
  StrCpyS(TitleString, TitleLength/sizeof(CHAR16), StartString);
  StrCatS(TitleString, TitleLength/sizeof(CHAR16), Command + Start);
  StrCatS(TitleString, TitleLength/sizeof(CHAR16), EndString);

  CurrentLocation = StrStr(*Buffer, TitleString);
  if (CurrentLocation == NULL){
    Status = EFI_NOT_FOUND;
  } else {
    //
    // we found it so copy out the rest of the line into BriefDesc
    // After skipping any spaces or zeroes
    //
    for (CurrentLocation += StrLen(TitleString)
      ;  *CurrentLocation == L' ' || *CurrentLocation == L'0' || *CurrentLocation == L'1' || *CurrentLocation == L'\"'
      ;  CurrentLocation++);

    TitleEnd = StrStr(CurrentLocation, L"\"");
    if (TitleEnd == NULL) {
      Status = EFI_DEVICE_ERROR;
    } else {
      if (BriefDesc != NULL) {
        *BriefSize = StrSize(TitleEnd);
        *BriefDesc = AllocateZeroPool(*BriefSize);
        if (*BriefDesc == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          StrnCpyS(*BriefDesc, (*BriefSize)/sizeof(CHAR16), CurrentLocation, TitleEnd-CurrentLocation);
        }
      }

      for (CurrentLocation = TitleEnd
        ;  *CurrentLocation != L'\n'
        ;  CurrentLocation++);
      for (
        ;  *CurrentLocation == L' ' || *CurrentLocation == L'\n' || *CurrentLocation == L'\r'
        ;  CurrentLocation++);
      *Buffer = CurrentLocation;
    }
  }

  FreePool(TitleString);
  return (Status);
}

/**
  Parses a line from a MAN file to see if it is the Title Header. If it is, then
  if the "Brief Description" is desired, allocate a buffer for it and return a
  copy. Upon a sucessful return the caller is responsible to free the memory in
  *BriefDesc

  Uses a simple state machine that allows "unlimited" whitespace before and after the
  ".TH", compares Command and the MAN file commnd name without respect to case, and
  allows "unlimited" whitespace and '0' and '1' characters before the Short Description.
  The PCRE regex describing this functionality is: ^\s*\.TH\s+(\S)\s[\s01]*(.*)$
  where group 1 is the Command Name and group 2 is the Short Description.

  @param[in] Command             name of command whose MAN file we think Line came from
  @param[in] Line                Pointer to a line from the MAN file
  @param[out] BriefDesc          pointer to pointer to string where description goes.
  @param[out] BriefSize          pointer to size of allocated BriefDesc
  @param[out] Found              TRUE if the Title Header was found and it belongs to Command

  @retval TRUE   Line contained the Title Header
  @retval FALSE  Line did not contain the Title Header
**/
BOOLEAN
IsTitleHeader(
  IN CONST CHAR16       *Command,
  IN CHAR16             *Line,
  OUT CHAR16            **BriefDesc OPTIONAL,
  OUT UINTN             *BriefSize OPTIONAL,
  OUT BOOLEAN           *Found
  )
{
  // The states of a simple state machine used to recognize a title header line
  // and to extract the Short Description, if desired.
  typedef enum {
    LookForThMacro, LookForCommandName, CompareCommands, GetBriefDescription, Final
  } STATEVALUES;

  STATEVALUES  State;
  UINTN        CommandIndex; // Indexes Command as we compare its chars to the MAN file.
  BOOLEAN      ReturnValue;  // TRUE if this the Title Header line of *some* MAN file.
  BOOLEAN      ReturnFound;  // TRUE if this the Title Header line of *the desired* MAN file.

  ReturnValue = FALSE;
  ReturnFound = FALSE;
  CommandIndex = 0;
  State = LookForThMacro;

  do {

    if (*Line == L'\0') {
      break;
    }

    switch (State) {

      // Handle "^\s*.TH\s"
      // Go to state LookForCommandName if the title header macro is present; otherwise,
      // eat white space. If we see something other than white space, this is not a
      // title header line.
      case LookForThMacro:
        if (StrnCmp (L".TH ", Line, 4) == 0 || StrnCmp (L".TH\t", Line, 4) == 0) {
          Line += 4;
          State = LookForCommandName;
        }
        else if (*Line == L' ' || *Line == L'\t') {
          Line++;
        }
        else {
          State = Final;
        }
      break;

      // Handle "\s*"
      // Eat any "extra" whitespace after the title header macro (we have already seen
      // at least one white space character). Go to state CompareCommands when a
      // non-white space is seen.
      case LookForCommandName:
        if (*Line == L' ' || *Line == L'\t') {
          Line++;
        }
        else {
          ReturnValue = TRUE;  // This is *some* command's title header line.
          State = CompareCommands;
          // Do not increment Line; it points to the first character of the command
          // name on the title header line.
        }
      break;

      // Handle "(\S)\s"
      // Compare Command to the title header command name, ignoring case. When we
      // reach the end of the command (i.e. we see white space), the next state
      // depends on whether the caller wants a copy of the Brief Description.
      case CompareCommands:
        if (*Line == L' ' || *Line == L'\t') {
          ReturnFound = TRUE;  // This is the desired command's title header line.
          State = (BriefDesc == NULL) ? Final : GetBriefDescription;
        }
        else if (InternalShellCharToUpper (*Line) != InternalShellCharToUpper (*(Command + CommandIndex++))) {
          State = Final;
        }
        Line++;
      break;

      // Handle "[\s01]*(.*)$"
      // Skip whitespace, '0', and '1' characters, if any, prior to the brief description.
      // Return the description to the caller.
      case GetBriefDescription:
        if (*Line != L' ' && *Line != L'\t' && *Line != L'0' && *Line != L'1') {
          *BriefSize = StrSize(Line);
          *BriefDesc = AllocateZeroPool(*BriefSize);
          if (*BriefDesc != NULL) {
            StrCpyS(*BriefDesc, (*BriefSize)/sizeof(CHAR16), Line);
          }
          State = Final;
        }
        Line++;
      break;

      default:
       break;
    }

  } while (State < Final);

  *Found = ReturnFound;
  return ReturnValue;
}

/**
  parses through the MAN file specified by SHELL_FILE_HANDLE and returns the
  "Brief Description" for the .TH section as specified by Command.  If the
  command section is not found return EFI_NOT_FOUND.

  Upon a sucessful return the caller is responsible to free the memory in *BriefDesc

  @param[in] Handle              FileHandle to read from
  @param[in] Command             name of command's section to find as entered on the
                                 command line (may be a relative or absolute path or
                                 be in any case: upper, lower, or mixed in numerous ways!).
  @param[out] BriefDesc          pointer to pointer to string where description goes.
  @param[out] BriefSize          pointer to size of allocated BriefDesc
  @param[in, out] Ascii          TRUE if the file is ASCII, FALSE otherwise, will be
                                 set if the file handle is at the 0 position.

  @retval EFI_OUT_OF_RESOURCES  a memory allocation failed.
  @retval EFI_SUCCESS           the section was found and its description stored in
                                an allocated buffer if requested.
**/
EFI_STATUS
EFIAPI
ManFileFindTitleSection(
  IN SHELL_FILE_HANDLE  Handle,
  IN CONST CHAR16       *Command,
  OUT CHAR16            **BriefDesc OPTIONAL,
  OUT UINTN             *BriefSize OPTIONAL,
  IN OUT BOOLEAN        *Ascii
  )
{
  EFI_STATUS  Status;
  CHAR16      *ReadLine;
  UINTN       Size;
  BOOLEAN     Found;
  UINTN       Start;

  if ( Handle     == NULL
    || Command    == NULL
    || (BriefDesc != NULL && BriefSize == NULL)
   ){
    return (EFI_INVALID_PARAMETER);
  }

  Status    = EFI_SUCCESS;
  Size      = 1024;
  Found     = FALSE;

  ReadLine  = AllocateZeroPool(Size);
  if (ReadLine == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // Do not pass any leading path information that may be present to IsTitleHeader().
  //
  Start = StrLen(Command);
  while ((Start != 0)
         && (*(Command + Start - 1) != L'\\')
         && (*(Command + Start - 1) != L'/')
         && (*(Command + Start - 1) != L':')) {
    --Start;
  }

  for (;!ShellFileHandleEof(Handle);Size = 1024) {
   Status = ShellFileHandleReadLine(Handle, ReadLine, &Size, TRUE, Ascii);
    //
    // ignore too small of buffer...
    //
    if (EFI_ERROR(Status) && Status != EFI_BUFFER_TOO_SMALL) {
      break;
    }

    Status = EFI_NOT_FOUND;
    if (IsTitleHeader (Command+Start, ReadLine, BriefDesc, BriefSize, &Found)) {
      Status = Found ? EFI_SUCCESS : EFI_NOT_FOUND;
      break;
    }
  }

  FreePool(ReadLine);
  return (Status);
}

/**
  This function returns the help information for the specified command. The help text
  will be parsed from a UEFI Shell manual page. (see UEFI Shell 2.0 Appendix B)

  If Sections is specified, then each section name listed will be compared in a casesensitive
  manner, to the section names described in Appendix B. If the section exists,
  it will be appended to the returned help text. If the section does not exist, no
  information will be returned. If Sections is NULL, then all help text information
  available will be returned.

  if BriefDesc is NULL, then the breif description will not be savedd seperatly,
  but placed first in the main HelpText.

  @param[in] ManFileName        Points to the NULL-terminated UEFI Shell MAN file name.
  @param[in] Command            Points to the NULL-terminated UEFI Shell command name.
  @param[in] Sections           Points to the NULL-terminated comma-delimited
                                section names to return. If NULL, then all
                                sections will be returned.
  @param[out] BriefDesc         On return, points to a callee-allocated buffer
                                containing brief description text.
  @param[out] HelpText          On return, points to a callee-allocated buffer
                                containing all specified help text.

  @retval EFI_SUCCESS           The help text was returned.
  @retval EFI_OUT_OF_RESOURCES  The necessary buffer could not be allocated to hold the
                                returned help text.
  @retval EFI_INVALID_PARAMETER HelpText is NULL.
  @retval EFI_INVALID_PARAMETER ManFileName is invalid.
  @retval EFI_NOT_FOUND         There is no help text available for Command.
**/
EFI_STATUS
EFIAPI
ProcessManFile(
  IN CONST CHAR16 *ManFileName,
  IN CONST CHAR16 *Command,
  IN CONST CHAR16 *Sections OPTIONAL,
  OUT CHAR16      **BriefDesc OPTIONAL,
  OUT CHAR16      **HelpText
  )
{
  CHAR16            *TempString;
  SHELL_FILE_HANDLE FileHandle;
  EFI_HANDLE        CmdFileImgHandle;
  EFI_STATUS        Status;
  UINTN             HelpSize;
  UINTN             BriefSize;
  UINTN             StringIdWalker;
  BOOLEAN           Ascii;
  CHAR16            *TempString2;
  CHAR16            *CmdFileName;
  CHAR16            *CmdFilePathName;
  CHAR16            *StringBuff;
  EFI_DEVICE_PATH_PROTOCOL      *FileDevPath;
  EFI_DEVICE_PATH_PROTOCOL      *DevPath;
  EFI_HII_PACKAGE_LIST_HEADER   *PackageListHeader;

  if ( ManFileName == NULL
    || Command     == NULL
    || HelpText    == NULL
   ){
    return (EFI_INVALID_PARAMETER);
  }

  HelpSize          = 0;
  BriefSize         = 0;
  StringIdWalker    = 0;
  TempString        = NULL;
  Ascii             = FALSE;
  CmdFileName       = NULL;
  CmdFilePathName   = NULL;
  CmdFileImgHandle  = NULL;
  StringBuff        = NULL;
  PackageListHeader = NULL;
  FileDevPath       = NULL;
  DevPath           = NULL;

  //
  // See if it's in HII first
  //
  TempString = ShellCommandGetCommandHelp(Command);
  if (TempString != NULL) {
    TempString2 = TempString;
    Status = ManBufferFindTitleSection(&TempString2, Command, BriefDesc, &BriefSize);
    if (!EFI_ERROR(Status) && HelpText != NULL){
      Status = ManBufferFindSections(TempString2, Sections, HelpText, &HelpSize);
    }
  } else {
    //
    // If the image is a external app, check .MAN file first.
    //
    FileHandle    = NULL;
    TempString  = GetManFileName(ManFileName);
    if (TempString == NULL) {
      return (EFI_INVALID_PARAMETER);
    }

    Status = SearchPathForFile(TempString, &FileHandle);
    if (EFI_ERROR(Status)) {
      FileDevPath = FileDevicePath(NULL, TempString);
      DevPath = AppendDevicePath (ShellInfoObject.ImageDevPath, FileDevPath);
      Status = InternalOpenFileDevicePath(DevPath, &FileHandle, EFI_FILE_MODE_READ, 0);
      SHELL_FREE_NON_NULL(FileDevPath);
      SHELL_FREE_NON_NULL(DevPath);
    }

    if (!EFI_ERROR(Status)) {
      HelpSize  = 0;
      BriefSize = 0;
      Status = ManFileFindTitleSection(FileHandle, Command, BriefDesc, &BriefSize, &Ascii);
      if (!EFI_ERROR(Status) && HelpText != NULL){
        Status = ManFileFindSections(FileHandle, Sections, HelpText, &HelpSize, Ascii);
      }
      ShellInfoObject.NewEfiShellProtocol->CloseFile(FileHandle);
      if (!EFI_ERROR(Status)) {
        //
        // Get help text from .MAN file success.
        //
        goto Done;
      }
    }

    //
    // Load the app image to check  EFI_HII_PACKAGE_LIST_PROTOCOL.
    //
    CmdFileName     = GetExecuatableFileName(TempString);
    if (CmdFileName == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }
    //
    // If the file in CWD then use the file name, else use the full
    // path name.
    //
    CmdFilePathName = ShellFindFilePath(CmdFileName);
    if (CmdFilePathName == NULL) {
      Status = EFI_NOT_FOUND;
      goto Done;
    }
    DevPath = ShellInfoObject.NewEfiShellProtocol->GetDevicePathFromFilePath(CmdFilePathName);
    Status      = gBS->LoadImage(FALSE, gImageHandle, DevPath, NULL, 0, &CmdFileImgHandle);
    if(EFI_ERROR(Status)) {
      *HelpText = NULL;
      goto Done;
    }
    Status = gBS->OpenProtocol(
                    CmdFileImgHandle,
                    &gEfiHiiPackageListProtocolGuid,
                    (VOID**)&PackageListHeader,
                    gImageHandle,
                    NULL,
                    EFI_OPEN_PROTOCOL_GET_PROTOCOL
                    );
    if(EFI_ERROR(Status)) {
      *HelpText = NULL;
      goto Done;
    }

    //
    // If get package list on image handle, install it on HiiDatabase.
    //
    Status = gBS->InstallProtocolInterface (
                    &mShellManDriverHandle,
                    &gEfiDevicePathProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    &mShellManHiiDevicePath
                    );
    if (EFI_ERROR(Status)) {
      goto Done;
    }

    Status = gHiiDatabase->NewPackageList (
                            gHiiDatabase,
                            PackageListHeader,
                            mShellManDriverHandle,
                            &mShellManHiiHandle
                            );
    if (EFI_ERROR (Status)) {
      goto Done;
    }

    StringIdWalker = 1;
    do {
        SHELL_FREE_NON_NULL(StringBuff);
        if (BriefDesc != NULL) {
          SHELL_FREE_NON_NULL(*BriefDesc);
        }
        StringBuff = HiiGetString (mShellManHiiHandle, (EFI_STRING_ID)StringIdWalker, NULL);
        if (StringBuff == NULL) {
          Status = EFI_NOT_FOUND;
          goto Done;
        }
        TempString2 = StringBuff;
        Status = ManBufferFindTitleSection(&TempString2, Command, BriefDesc, &BriefSize);
        if (!EFI_ERROR(Status) && HelpText != NULL){
          Status = ManBufferFindSections(TempString2, Sections, HelpText, &HelpSize);
        }
        if (!EFI_ERROR(Status)){
          //
          // Found what we need and return
          //
          goto Done;
        }

        StringIdWalker += 1;
    } while (StringIdWalker < 0xFFFF && StringBuff != NULL);

  }

Done:
  if (mShellManDriverHandle != NULL) {
    gBS->UninstallProtocolInterface (
            mShellManDriverHandle,
            &gEfiDevicePathProtocolGuid,
            &mShellManHiiDevicePath
           );
    mShellManDriverHandle = NULL;
  }

  if (mShellManHiiHandle != NULL) {
    HiiRemovePackages (mShellManHiiHandle);
    mShellManHiiHandle = NULL;
  }

  if (CmdFileImgHandle != NULL) {
    Status = gBS->UnloadImage (CmdFileImgHandle);
  }

  SHELL_FREE_NON_NULL(StringBuff);
  SHELL_FREE_NON_NULL(TempString);
  SHELL_FREE_NON_NULL(CmdFileName);
  SHELL_FREE_NON_NULL(CmdFilePathName);
  SHELL_FREE_NON_NULL(FileDevPath);
  SHELL_FREE_NON_NULL(DevPath);

  return (Status);
}

