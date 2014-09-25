/** @file
  Main file for NULL named library for level 2 shell command functions.

  these functions are:
  attrib,
  cd,
  cp,
  date*,
  time*,
  load,
  ls,
  map,
  mkdir,
  mv,
  parse,
  rm,
  reset,
  set,
  timezone*,
  vol

  * functions are non-interactive only

  Copyright (c) 2014 Hewlett-Packard Development Company, L.P.
  Copyright (c) 2009 - 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#include "UefiShellLevel2CommandsLib.h"

CONST CHAR16 mFileName[] = L"ShellCommands";
EFI_HANDLE gShellLevel2HiiHandle = NULL;

/**
  Get the filename to get help text from if not using HII.

  @retval The filename.
**/
CONST CHAR16*
EFIAPI
ShellCommandGetManFileNameLevel2 (
  VOID
  )
{
  return (mFileName);
}

/**
  Constructor for the Shell Level 2 Commands library.

  Install the handlers for level 2 UEFI Shell 2.0 commands.

  @param ImageHandle    the image handle of the process
  @param SystemTable    the EFI System Table pointer

  @retval EFI_SUCCESS        the shell command handlers were installed sucessfully
  @retval EFI_UNSUPPORTED    the shell level required was not found.
**/
EFI_STATUS
EFIAPI
ShellLevel2CommandsLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  //
  // if shell level is less than 2 do nothing
  //
  if (PcdGet8(PcdShellSupportLevel) < 2) {
    return (EFI_SUCCESS);
  }

  gShellLevel2HiiHandle = HiiAddPackages (&gShellLevel2HiiGuid, gImageHandle, UefiShellLevel2CommandsLibStrings, NULL);
  if (gShellLevel2HiiHandle == NULL) {
    return (EFI_DEVICE_ERROR);
  }

  //
  // install our shell command handlers that are always installed
  //
  ShellCommandRegisterCommandName(L"attrib",   ShellCommandRunAttrib  , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_ATTRIB) );
  ShellCommandRegisterCommandName(L"cd",       ShellCommandRunCd      , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_CD)     );
  ShellCommandRegisterCommandName(L"cp",       ShellCommandRunCp      , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_CP)     );
  ShellCommandRegisterCommandName(L"load",     ShellCommandRunLoad    , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_LOAD)   );
  ShellCommandRegisterCommandName(L"map",      ShellCommandRunMap     , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_MAP)    );
  ShellCommandRegisterCommandName(L"mkdir",    ShellCommandRunMkDir   , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_MKDIR)  );
  ShellCommandRegisterCommandName(L"mv",       ShellCommandRunMv      , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_MV)     );
  ShellCommandRegisterCommandName(L"parse",    ShellCommandRunParse   , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_PARSE)  );
  ShellCommandRegisterCommandName(L"reset",    ShellCommandRunReset   , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_RESET)  );
  ShellCommandRegisterCommandName(L"set",      ShellCommandRunSet     , ShellCommandGetManFileNameLevel2, 2, L"",FALSE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_SET)    );
  ShellCommandRegisterCommandName(L"ls",       ShellCommandRunLs      , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_LS)     );
  ShellCommandRegisterCommandName(L"rm",       ShellCommandRunRm      , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_RM)     );
  ShellCommandRegisterCommandName(L"vol",      ShellCommandRunVol     , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_VOL)    );

  //
  // support for permenant (built in) aliases
  //
  ShellCommandRegisterAlias(L"rm", L"del");
  ShellCommandRegisterAlias(L"ls", L"dir");
  ShellCommandRegisterAlias(L"cp", L"copy");
  ShellCommandRegisterAlias(L"mkdir", L"md");
  ShellCommandRegisterAlias(L"cd ..", L"cd..");
  ShellCommandRegisterAlias(L"cd \\", L"cd\\");
  ShellCommandRegisterAlias(L"mv", L"ren");
  ShellCommandRegisterAlias(L"mv", L"move");
  ShellCommandRegisterAlias(L"map", L"mount");
  //
  // These are installed in level 2 or 3...
  //
  if (PcdGet8(PcdShellSupportLevel) == 2 || PcdGet8(PcdShellSupportLevel) == 3) {
    ShellCommandRegisterCommandName(L"date",     ShellCommandRunDate    , ShellCommandGetManFileNameLevel2, PcdGet8(PcdShellSupportLevel), L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_DATE)   );
    ShellCommandRegisterCommandName(L"time",     ShellCommandRunTime    , ShellCommandGetManFileNameLevel2, PcdGet8(PcdShellSupportLevel), L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_TIME)   );
    ShellCommandRegisterCommandName(L"timezone", ShellCommandRunTimeZone, ShellCommandGetManFileNameLevel2, PcdGet8(PcdShellSupportLevel), L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_TIMEZONE));
  } else {
    DEBUG_CODE_BEGIN();
    //
    // we want to be able to test these so install them under a different name in debug mode...
    //
    ShellCommandRegisterCommandName(L"l2date",     ShellCommandRunDate    , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_DATE)   );
    ShellCommandRegisterCommandName(L"l2time",     ShellCommandRunTime    , ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_TIME)   );
    ShellCommandRegisterCommandName(L"l2timezone", ShellCommandRunTimeZone, ShellCommandGetManFileNameLevel2, 2, L"", TRUE, gShellLevel2HiiHandle, STRING_TOKEN(STR_GET_HELP_TIMEZONE));
    DEBUG_CODE_END();
  }

  return (EFI_SUCCESS);
}

/**
  Destructor for the library.  free any resources.

  @param ImageHandle    The image handle of the process.
  @param SystemTable    The EFI System Table pointer.

  @retval EFI_SUCCESS   Always returned.
**/
EFI_STATUS
EFIAPI
ShellLevel2CommandsLibDestructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  if (gShellLevel2HiiHandle != NULL) {
    HiiRemovePackages(gShellLevel2HiiHandle);
  }
  return (EFI_SUCCESS);
}

/**
  returns a fully qualified directory (contains a map drive at the begining)
  path from a unknown directory path.

  If Path is already fully qualified this will return a duplicat otherwise this
  will use get the current directory and use that to build the fully qualified
  version.

  if the return value is not NULL it must be caller freed.

  @param[in] Path         The unknown Path Value

  @retval NULL            A memory allocation failed
  @retval NULL            A fully qualified path could not be discovered.
  @retval other           An allocated pointer to a fuly qualified path.
**/
CHAR16*
EFIAPI
GetFullyQualifiedPath(
  IN CONST CHAR16* Path
  )
{
  CHAR16        *PathToReturn;
  UINTN         Size;
  CONST CHAR16  *CurDir;

  PathToReturn  = NULL;
  Size          = 0;

  ASSERT((PathToReturn == NULL && Size == 0) || (PathToReturn != NULL));
  //
  // convert a local path to an absolute path
  //
  if (StrStr(Path, L":") == NULL) {
    CurDir = gEfiShellProtocol->GetCurDir(NULL);
    StrnCatGrow(&PathToReturn, &Size, CurDir, 0);
    if (*Path == L'\\') {
      Path++;
    }
  }
  StrnCatGrow(&PathToReturn, &Size, Path, 0);

  PathCleanUpDirectories(PathToReturn);

  if (PathToReturn == NULL) {
    return NULL;
  }

  while (PathToReturn[StrLen(PathToReturn)-1] == L'*') {
    PathToReturn[StrLen(PathToReturn)-1] = CHAR_NULL;
  }

  return (PathToReturn);
}

/**
  Function to verify all intermediate directories in the path.

  @param[in] Path       The pointer to the path to fix.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
VerifyIntermediateDirectories (
  IN CONST CHAR16 *Path
  )
{
  EFI_STATUS      Status;
  CHAR16          *PathCopy;
  CHAR16          *TempSpot;
  SHELL_FILE_HANDLE          FileHandle;

  ASSERT(Path != NULL);

  Status      = EFI_SUCCESS;
  PathCopy    = NULL;
  PathCopy    = StrnCatGrow(&PathCopy, NULL, Path, 0);
  FileHandle  = NULL;

  if (PathCopy == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  for (TempSpot = &PathCopy[StrLen(PathCopy)-1] ; *TempSpot != CHAR_NULL && *TempSpot != L'\\' ; TempSpot = &PathCopy[StrLen(PathCopy)-1]){
    *TempSpot = CHAR_NULL;
  }
  if (*TempSpot == L'\\') {
    *TempSpot = CHAR_NULL;
  }

  if (PathCopy != NULL && *PathCopy != CHAR_NULL) {
    Status = VerifyIntermediateDirectories(PathCopy);

    if (PathCopy[StrLen(PathCopy)-1] != L':') {
      if (!EFI_ERROR(Status)) {
        Status = ShellOpenFileByName(PathCopy, &FileHandle, EFI_FILE_MODE_READ, 0);
        if (FileHandle != NULL) {
          ShellCloseFile(&FileHandle);
        }
      }
    }
  }

  SHELL_FREE_NON_NULL(PathCopy);

  return (Status);
}

/**
  Be lazy and borrow from baselib.

  @param[in] Char   The character to convert to upper case.

  @return Char as an upper case character.
**/
CHAR16
EFIAPI
InternalCharToUpper (
  IN CONST CHAR16                    Char
  );

/**
  String comparison without regard to case for a limited number of characters.

  @param[in] Source   The first item to compare.
  @param[in] Target   The second item to compare.
  @param[in] Count    How many characters to compare.

  @retval NULL Source and Target are identical strings without regard to case.
  @return The location in Source where there is a difference.
**/
CONST CHAR16*
EFIAPI
StrniCmp(
  IN CONST CHAR16 *Source,
  IN CONST CHAR16 *Target,
  IN CONST UINTN  Count
  )
{
  UINTN   LoopCount;
  CHAR16  Char1;
  CHAR16  Char2;

  ASSERT(Source != NULL);
  ASSERT(Target != NULL);

  for (LoopCount = 0 ; LoopCount < Count ; LoopCount++) {
    Char1 = InternalCharToUpper(Source[LoopCount]);
    Char2 = InternalCharToUpper(Target[LoopCount]);
    if (Char1 != Char2) {
      return (&Source[LoopCount]);
    }
  }
  return (NULL);
}


/**
  Cleans off all the quotes in the string.

  @param[in]     OriginalString   pointer to the string to be cleaned.
  @param[out]   CleanString      The new string with all quotes removed. 
                                                  Memory allocated in the function and free 
                                                  by caller.

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
EFIAPI
ShellLevel2StripQuotes (
  IN  CONST CHAR16     *OriginalString,
  OUT CHAR16           **CleanString
  )
{
  CHAR16            *Walker;
  
  if (OriginalString == NULL || CleanString == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *CleanString = AllocateCopyPool (StrSize (OriginalString), OriginalString);
  if (*CleanString == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  for (Walker = *CleanString; Walker != NULL && *Walker != CHAR_NULL ; Walker++) {
    if (*Walker == L'\"') {
      CopyMem(Walker, Walker+1, StrSize(Walker) - sizeof(Walker[0]));
    }
  }

  return EFI_SUCCESS;
}


