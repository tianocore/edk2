/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (C) 2014, Red Hat, Inc.
  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Uefi.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/FileHandleLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/ShellLib.h>
#include <Library/ShellProtocolsLib.h>
#include <Library/ShellProtocolInteractivityLib.h>
#include <Library/SortLib.h>
#include <Library/UefiBootServicesTableLib.h>

BOOLEAN  AsciiRedirection = FALSE;

/**
  Determine if a file name represents a unicode file.

  @param[in] FileName     Pointer to the filename to open.

  @retval EFI_SUCCESS     The file is a unicode file.
  @return An error upon failure.
**/
EFI_STATUS
IsUnicodeFile (
  IN CONST CHAR16  *FileName
  )
{
  SHELL_FILE_HANDLE  Handle;
  EFI_STATUS         Status;
  UINT64             OriginalFilePosition;
  UINTN              CharSize;
  CHAR16             CharBuffer;

  Status = gEfiShellProtocol->OpenFileByName (FileName, &Handle, EFI_FILE_MODE_READ);
  if (EFI_ERROR (Status)) {
    return (Status);
  }

  gEfiShellProtocol->GetFilePosition (Handle, &OriginalFilePosition);
  gEfiShellProtocol->SetFilePosition (Handle, 0);
  CharSize = sizeof (CHAR16);
  Status   = gEfiShellProtocol->ReadFile (Handle, &CharSize, &CharBuffer);
  if (EFI_ERROR (Status) || (CharBuffer != gUnicodeFileTag)) {
    Status = EFI_BUFFER_TOO_SMALL;
  }

  gEfiShellProtocol->SetFilePosition (Handle, OriginalFilePosition);
  gEfiShellProtocol->CloseFile (Handle);
  return (Status);
}

/**
  Strips out quotes sections of a string.

  All of the characters between quotes is replaced with spaces.

  @param[in, out] TheString  A pointer to the string to update.
**/
VOID
StripQuotes (
  IN OUT CHAR16  *TheString
  )
{
  BOOLEAN  RemoveNow;

  for (RemoveNow = FALSE; TheString != NULL && *TheString != CHAR_NULL; TheString++) {
    if ((*TheString == L'^') && (*(TheString + 1) == L'\"')) {
      TheString++;
    } else if (*TheString == L'\"') {
      RemoveNow = (BOOLEAN) !RemoveNow;
    } else if (RemoveNow) {
      *TheString = L' ';
    }
  }
}

/**
  Calculate the 32-bit CRC in a EFI table using the service provided by the
  gRuntime service.

  @param  Hdr                    Pointer to an EFI standard header

**/
VOID
CalculateEfiHdrCrc (
  IN  OUT EFI_TABLE_HEADER  *Hdr
  )
{
  UINT32  Crc;

  Hdr->CRC32 = 0;

  //
  // If gBS->CalculateCrce32 () == CoreEfiNotAvailableYet () then
  //  Crc will come back as zero if we set it to zero here
  //
  Crc = 0;
  gBS->CalculateCrc32 ((UINT8 *)Hdr, Hdr->HeaderSize, &Crc);
  Hdr->CRC32 = Crc;
}

/**
  Fix a string to only have the file name, removing starting at the first space of whatever is quoted.

  @param[in]  FileName    The filename to start with.

  @retval NULL  FileName was invalid.
  @return       The modified FileName.
**/
CHAR16 *
FixFileName (
  IN CHAR16  *FileName
  )
{
  CHAR16  *Copy;
  CHAR16  *TempLocation;

  if (FileName == NULL) {
    return (NULL);
  }

  if (FileName[0] == L'\"') {
    Copy = FileName+1;
    if ((TempLocation = StrStr (Copy, L"\"")) != NULL) {
      TempLocation[0] = CHAR_NULL;
    }
  } else {
    Copy = FileName;
    while (Copy[0] == L' ') {
      Copy++;
    }

    if ((TempLocation = StrStr (Copy, L" ")) != NULL) {
      TempLocation[0] = CHAR_NULL;
    }
  }

  if (Copy[0] == CHAR_NULL) {
    return (NULL);
  }

  return (Copy);
}

/**
  Fix a string to only have the environment variable name, removing starting at the first space of whatever is quoted and removing the leading and trailing %.

  @param[in]  FileName    The filename to start with.

  @retval NULL  FileName was invalid.
  @return       The modified FileName.
**/
CHAR16 *
FixVarName (
  IN CHAR16  *FileName
  )
{
  CHAR16  *Copy;
  CHAR16  *TempLocation;

  Copy = FileName;

  if (FileName[0] == L'%') {
    Copy = FileName+1;
    if ((TempLocation = StrStr (Copy, L"%")) != NULL) {
      TempLocation[0] = CHAR_NULL;
    }
  }

  return (FixFileName (Copy));
}

/**
  Write the unicode file tag to the specified file.

  It is the caller's responsibility to ensure that
  ShellProtocolsInfoObject.NewEfiShellProtocol has been initialized before calling this
  function.

  @param[in] FileHandle  The file to write the unicode file tag to.

  @return  Status code from ShellProtocolsInfoObject.NewEfiShellProtocol->WriteFile.
**/
EFI_STATUS
WriteFileTag (
  IN SHELL_FILE_HANDLE  FileHandle
  )
{
  CHAR16      FileTag;
  UINTN       Size;
  EFI_STATUS  Status;

  FileTag = gUnicodeFileTag;
  Size    = sizeof FileTag;
  Status  = ShellProtocolsInfoObject.NewEfiShellProtocol->WriteFile (
                                                            FileHandle,
                                                            &Size,
                                                            &FileTag
                                                            );
  ASSERT (EFI_ERROR (Status) || Size == sizeof FileTag);
  return Status;
}

/**
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  This will also update the system table.

  @param[in, out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine              The new command line to parse and use.
  @param[out] OldStdIn                   Pointer to old StdIn.
  @param[out] OldStdOut                  Pointer to old StdOut.
  @param[out] OldStdErr                  Pointer to old StdErr.
  @param[out] SystemTableInfo            Pointer to old system table information.

  @retval   EFI_SUCCESS                 Operation was successful, Argv and Argc are valid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
UpdateStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             *NewCommandLine,
  OUT SHELL_FILE_HANDLE                 *OldStdIn,
  OUT SHELL_FILE_HANDLE                 *OldStdOut,
  OUT SHELL_FILE_HANDLE                 *OldStdErr,
  OUT SYSTEM_TABLE_INFO                 *SystemTableInfo
  )
{
  CHAR16             *CommandLineCopy;
  CHAR16             *CommandLineWalker;
  CHAR16             *StdErrFileName;
  CHAR16             *StdOutFileName;
  CHAR16             *StdInFileName;
  CHAR16             *StdInVarName;
  CHAR16             *StdOutVarName;
  CHAR16             *StdErrVarName;
  EFI_STATUS         Status;
  SHELL_FILE_HANDLE  TempHandle;
  UINT64             FileSize;
  BOOLEAN            OutUnicode;
  BOOLEAN            InUnicode;
  BOOLEAN            ErrUnicode;
  BOOLEAN            OutAppend;
  BOOLEAN            ErrAppend;
  UINTN              Size;
  SPLIT_LIST         *Split;
  CHAR16             *FirstLocation;
  BOOLEAN            Volatile;

  OutUnicode       = TRUE;
  InUnicode        = TRUE;
  AsciiRedirection = FALSE;
  ErrUnicode       = TRUE;
  StdInVarName     = NULL;
  StdOutVarName    = NULL;
  StdErrVarName    = NULL;
  StdErrFileName   = NULL;
  StdInFileName    = NULL;
  StdOutFileName   = NULL;
  ErrAppend        = FALSE;
  OutAppend        = FALSE;
  CommandLineCopy  = NULL;
  FirstLocation    = NULL;
  TempHandle       = NULL;

  if ((ShellParameters == NULL) || (SystemTableInfo == NULL) || (OldStdIn == NULL) || (OldStdOut == NULL) || (OldStdErr == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  SystemTableInfo->ConIn        = gST->ConIn;
  SystemTableInfo->ConInHandle  = gST->ConsoleInHandle;
  SystemTableInfo->ConOut       = gST->ConOut;
  SystemTableInfo->ConOutHandle = gST->ConsoleOutHandle;
  SystemTableInfo->ErrOut       = gST->StdErr;
  SystemTableInfo->ErrOutHandle = gST->StandardErrorHandle;
  *OldStdIn                     = ShellParameters->StdIn;
  *OldStdOut                    = ShellParameters->StdOut;
  *OldStdErr                    = ShellParameters->StdErr;

  if (NewCommandLine == NULL) {
    return (EFI_SUCCESS);
  }

  CommandLineCopy = StrnCatGrow (&CommandLineCopy, NULL, NewCommandLine, 0);
  if (CommandLineCopy == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  Status        = EFI_SUCCESS;
  Split         = NULL;
  FirstLocation = CommandLineCopy + StrLen (CommandLineCopy);

  StripQuotes (CommandLineCopy);

  if (!IsListEmpty (&ShellProtocolInteractivityInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST *)GetFirstNode (&ShellProtocolInteractivityInfoObject.SplitList.Link);
    if ((Split != NULL) && (Split->SplitStdIn != NULL)) {
      ShellParameters->StdIn = Split->SplitStdIn;
    }

    if ((Split != NULL) && (Split->SplitStdOut != NULL)) {
      ShellParameters->StdOut = Split->SplitStdOut;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 2>>v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 12, L' ');
    StdErrVarName = CommandLineWalker += 6;
    ErrAppend     = TRUE;
    if (StrStr (CommandLineWalker, L" 2>>v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 1>>v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 12, L' ');
    StdOutVarName = CommandLineWalker += 6;
    OutAppend     = TRUE;
    if (StrStr (CommandLineWalker, L" 1>>v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  } else if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" >>v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    StdOutVarName = CommandLineWalker += 5;
    OutAppend     = TRUE;
    if (StrStr (CommandLineWalker, L" >>v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  } else if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" >v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    StdOutVarName = CommandLineWalker += 4;
    OutAppend     = FALSE;
    if (StrStr (CommandLineWalker, L" >v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 1>>a ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 12, L' ');
    StdOutFileName = CommandLineWalker += 6;
    OutAppend      = TRUE;
    OutUnicode     = FALSE;
    if (StrStr (CommandLineWalker, L" 1>>a ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 1>> ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 5;
      OutAppend      = TRUE;
    }

    if (StrStr (CommandLineWalker, L" 1>> ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" >> ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 4;
      OutAppend      = TRUE;
    }

    if (StrStr (CommandLineWalker, L" >> ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" >>a ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 5;
      OutAppend      = TRUE;
      OutUnicode     = FALSE;
    }

    if (StrStr (CommandLineWalker, L" >>a ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 1>a ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 5;
      OutAppend      = FALSE;
      OutUnicode     = FALSE;
    }

    if (StrStr (CommandLineWalker, L" 1>a ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" >a ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 4;
      OutAppend      = FALSE;
      OutUnicode     = FALSE;
    }

    if (StrStr (CommandLineWalker, L" >a ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 2>> ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdErrFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrFileName = CommandLineWalker += 5;
      ErrAppend      = TRUE;
    }

    if (StrStr (CommandLineWalker, L" 2>> ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 2>v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdErrVarName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrVarName = CommandLineWalker += 5;
      ErrAppend     = FALSE;
    }

    if (StrStr (CommandLineWalker, L" 2>v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 1>v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdOutVarName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutVarName = CommandLineWalker += 5;
      OutAppend     = FALSE;
    }

    if (StrStr (CommandLineWalker, L" 1>v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 2>a ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 10, L' ');
    if (StdErrFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrFileName = CommandLineWalker += 5;
      ErrAppend      = FALSE;
      ErrUnicode     = FALSE;
    }

    if (StrStr (CommandLineWalker, L" 2>a ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 2> ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    if (StdErrFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrFileName = CommandLineWalker += 4;
      ErrAppend      = FALSE;
    }

    if (StrStr (CommandLineWalker, L" 2> ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" 1> ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 4;
      OutAppend      = FALSE;
    }

    if (StrStr (CommandLineWalker, L" 1> ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" > ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 6, L' ');
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName = CommandLineWalker += 3;
      OutAppend      = FALSE;
    }

    if (StrStr (CommandLineWalker, L" > ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" < ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 6, L' ');
    if (StdInFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdInFileName = CommandLineWalker += 3;
    }

    if (StrStr (CommandLineWalker, L" < ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" <a ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    if (StdInFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdInFileName    = CommandLineWalker += 4;
      InUnicode        = FALSE;
      AsciiRedirection = TRUE;
    }

    if (StrStr (CommandLineWalker, L" <a ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  if (!EFI_ERROR (Status) && ((CommandLineWalker = StrStr (CommandLineCopy, L" <v ")) != NULL)) {
    FirstLocation = MIN (CommandLineWalker, FirstLocation);
    SetMem16 (CommandLineWalker, 8, L' ');
    if (StdInVarName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdInVarName = CommandLineWalker += 4;
    }

    if (StrStr (CommandLineWalker, L" <v ") != NULL) {
      Status = EFI_NOT_FOUND;
    }
  }

  //
  // re-populate the string to support any filenames that were in quotes.
  //
  StrnCpyS (CommandLineCopy, StrSize (CommandLineCopy)/sizeof (CHAR16), NewCommandLine, StrLen (NewCommandLine));

  if (  (FirstLocation != CommandLineCopy + StrLen (CommandLineCopy))
     && (((UINTN)FirstLocation - (UINTN)CommandLineCopy)/sizeof (CHAR16) < StrLen (NewCommandLine))
        )
  {
    *(NewCommandLine + ((UINTN)FirstLocation - (UINTN)CommandLineCopy)/sizeof (CHAR16)) = CHAR_NULL;
  }

  if (!EFI_ERROR (Status)) {
    if (StdErrFileName != NULL) {
      if ((StdErrFileName    = FixFileName (StdErrFileName)) == NULL) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    if (StdOutFileName != NULL) {
      if ((StdOutFileName    = FixFileName (StdOutFileName)) == NULL) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    if (StdInFileName  != NULL) {
      if ((StdInFileName     = FixFileName (StdInFileName)) == NULL) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    if (StdErrVarName  != NULL) {
      if ((StdErrVarName     = FixVarName (StdErrVarName)) == NULL) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    if (StdOutVarName  != NULL) {
      if ((StdOutVarName     = FixVarName (StdOutVarName)) == NULL) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    if (StdInVarName   != NULL) {
      if ((StdInVarName      = FixVarName (StdInVarName)) == NULL) {
        Status = EFI_INVALID_PARAMETER;
      }
    }

    //
    // Verify not the same and not duplicating something from a split
    //
    if (
          //
          // Check that no 2 filenames are the same
          //
          ((StdErrFileName != NULL) && (StdOutFileName != NULL) && (StringNoCaseCompare (&StdErrFileName, &StdOutFileName) == 0))
       || ((StdErrFileName != NULL) && (StdInFileName != NULL) && (StringNoCaseCompare (&StdErrFileName, &StdInFileName) == 0))
       || ((StdOutFileName != NULL) && (StdInFileName != NULL) && (StringNoCaseCompare (&StdOutFileName, &StdInFileName) == 0))
          //
          // Check that no 2 variable names are the same
          //
       || ((StdErrVarName  != NULL) && (StdInVarName  != NULL) && (StringNoCaseCompare (&StdErrVarName, &StdInVarName) == 0))
       || ((StdOutVarName  != NULL) && (StdInVarName != NULL) && (StringNoCaseCompare (&StdOutVarName, &StdInVarName) == 0))
       || ((StdErrVarName  != NULL) && (StdOutVarName != NULL) && (StringNoCaseCompare (&StdErrVarName, &StdOutVarName) == 0))
          //
          // When a split (using | operator) is in place some are not allowed
          //
       || ((Split != NULL) && (Split->SplitStdIn  != NULL) && ((StdInVarName  != NULL) || (StdInFileName  != NULL)))
       || ((Split != NULL) && (Split->SplitStdOut != NULL) && ((StdOutVarName != NULL) || (StdOutFileName != NULL)))
          //
          // Check that nothing is trying to be output to 2 locations.
          //
       || ((StdErrFileName != NULL) && (StdErrVarName != NULL))
       || ((StdOutFileName != NULL) && (StdOutVarName != NULL))
       || ((StdInFileName  != NULL) && (StdInVarName  != NULL))
          //
          // Check for no volatile environment variables
          //
       || ((StdErrVarName  != NULL) && !EFI_ERROR (IsVolatileEnv (StdErrVarName, &Volatile)) && !Volatile)
       || ((StdOutVarName  != NULL) && !EFI_ERROR (IsVolatileEnv (StdOutVarName, &Volatile)) && !Volatile)
          //
          // Cant redirect during a reconnect operation.
          //
       || (  (StrStr (NewCommandLine, L"connect -r") != NULL)
          && ((StdOutVarName != NULL) || (StdOutFileName != NULL) || (StdErrFileName != NULL) || (StdErrVarName != NULL)))
          //
          // Check that filetypes (Unicode/Ascii) do not change during an append
          //
       || ((StdOutFileName != NULL) && OutUnicode && OutAppend && (!EFI_ERROR (ShellFileExists (StdOutFileName)) && EFI_ERROR (IsUnicodeFile (StdOutFileName))))
       || ((StdErrFileName != NULL) && ErrUnicode && ErrAppend && (!EFI_ERROR (ShellFileExists (StdErrFileName)) && EFI_ERROR (IsUnicodeFile (StdErrFileName))))
       || ((StdOutFileName != NULL) && !OutUnicode && OutAppend && (!EFI_ERROR (ShellFileExists (StdOutFileName)) && !EFI_ERROR (IsUnicodeFile (StdOutFileName))))
       || ((StdErrFileName != NULL) && !ErrUnicode && ErrAppend && (!EFI_ERROR (ShellFileExists (StdErrFileName)) && !EFI_ERROR (IsUnicodeFile (StdErrFileName))))
          )
    {
      Status                  = EFI_INVALID_PARAMETER;
      ShellParameters->StdIn  = *OldStdIn;
      ShellParameters->StdOut = *OldStdOut;
      ShellParameters->StdErr = *OldStdErr;
    } else if (!EFI_ERROR (Status)) {
      //
      // Open the Std<Whatever> and we should not have conflicts here...
      //

      //
      // StdErr to a file
      //
      if (StdErrFileName != NULL) {
        if (!ErrAppend) {
          //
          // delete existing file.
          //
          ShellProtocolsInfoObject.NewEfiShellProtocol->DeleteFileByName (StdErrFileName);
        }

        Status = ShellOpenFileByName (StdErrFileName, &TempHandle, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE, 0);
        if (!ErrAppend && ErrUnicode && !EFI_ERROR (Status)) {
          Status = WriteFileTag (TempHandle);
        }

        if (!ErrUnicode && !EFI_ERROR (Status)) {
          TempHandle = CreateFileInterfaceFile (TempHandle, FALSE);
          if (TempHandle == NULL) {
            ASSERT (TempHandle != NULL);
            Status = EFI_OUT_OF_RESOURCES;
          }
        }

        if (!EFI_ERROR (Status)) {
          ShellParameters->StdErr = TempHandle;
          gST->StdErr             = CreateSimpleTextOutOnFile (TempHandle, &gST->StandardErrorHandle, gST->StdErr);
        }
      }

      //
      // StdOut to a file
      //
      if (!EFI_ERROR (Status) && (StdOutFileName != NULL)) {
        if (!OutAppend) {
          //
          // delete existing file.
          //
          ShellProtocolsInfoObject.NewEfiShellProtocol->DeleteFileByName (StdOutFileName);
        }

        Status = ShellOpenFileByName (StdOutFileName, &TempHandle, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE, 0);
        if (TempHandle == NULL) {
          Status = EFI_INVALID_PARAMETER;
        } else {
          if (gUnicodeCollation->MetaiMatch (gUnicodeCollation, StdOutFileName, L"NUL")) {
            // no-op
          } else if (!OutAppend && OutUnicode && !EFI_ERROR (Status)) {
            Status = WriteFileTag (TempHandle);
          } else if (OutAppend) {
            Status = ShellProtocolsInfoObject.NewEfiShellProtocol->GetFileSize (TempHandle, &FileSize);
            if (!EFI_ERROR (Status)) {
              //
              // When appending to a new unicode file, write the file tag.
              // Otherwise (ie. when appending to a new ASCII file, or an
              // existent file with any encoding), just seek to the end.
              //
              Status = (FileSize == 0 && OutUnicode) ?
                       WriteFileTag (TempHandle) :
                       ShellProtocolsInfoObject.NewEfiShellProtocol->SetFilePosition (
                                                                       TempHandle,
                                                                       FileSize
                                                                       );
            }
          }

          if (!OutUnicode && !EFI_ERROR (Status)) {
            TempHandle = CreateFileInterfaceFile (TempHandle, FALSE);
            if (TempHandle == NULL) {
              ASSERT (TempHandle != NULL);
              Status = EFI_OUT_OF_RESOURCES;
            }
          }

          if (!EFI_ERROR (Status)) {
            ShellParameters->StdOut = TempHandle;
            gST->ConOut             = CreateSimpleTextOutOnFile (TempHandle, &gST->ConsoleOutHandle, gST->ConOut);
          }
        }
      }

      //
      // StdOut to a var
      //
      if (!EFI_ERROR (Status) && (StdOutVarName != NULL)) {
        if (!OutAppend) {
          //
          // delete existing variable.
          //
          SHELL_SET_ENVIRONMENT_VARIABLE_V (StdOutVarName, 0, L"");
        }

        TempHandle = CreateFileInterfaceEnv (StdOutVarName);
        if (TempHandle == NULL) {
          ASSERT (TempHandle != NULL);
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          ShellParameters->StdOut = TempHandle;
          gST->ConOut             = CreateSimpleTextOutOnFile (TempHandle, &gST->ConsoleOutHandle, gST->ConOut);
        }
      }

      //
      // StdErr to a var
      //
      if (!EFI_ERROR (Status) && (StdErrVarName != NULL)) {
        if (!ErrAppend) {
          //
          // delete existing variable.
          //
          SHELL_SET_ENVIRONMENT_VARIABLE_V (StdErrVarName, 0, L"");
        }

        TempHandle = CreateFileInterfaceEnv (StdErrVarName);
        if (TempHandle == NULL) {
          ASSERT (TempHandle != NULL);
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          ShellParameters->StdErr = TempHandle;
          gST->StdErr             = CreateSimpleTextOutOnFile (TempHandle, &gST->StandardErrorHandle, gST->StdErr);
        }
      }

      //
      // StdIn from a var
      //
      if (!EFI_ERROR (Status) && (StdInVarName != NULL)) {
        TempHandle = CreateFileInterfaceEnv (StdInVarName);
        if (TempHandle == NULL) {
          Status = EFI_OUT_OF_RESOURCES;
        } else {
          if (!InUnicode) {
            TempHandle = CreateFileInterfaceFile (TempHandle, FALSE);
          }

          Size = 0;
          if ((TempHandle == NULL) || (((EFI_FILE_PROTOCOL *)TempHandle)->Read (TempHandle, &Size, NULL) != EFI_BUFFER_TOO_SMALL)) {
            Status = EFI_INVALID_PARAMETER;
          } else {
            ShellParameters->StdIn = TempHandle;
            gST->ConIn             = CreateSimpleTextInOnFile (TempHandle, &gST->ConsoleInHandle);
          }
        }
      }

      //
      // StdIn from a file
      //
      if (!EFI_ERROR (Status) && (StdInFileName != NULL)) {
        Status = ShellOpenFileByName (
                   StdInFileName,
                   &TempHandle,
                   EFI_FILE_MODE_READ,
                   0
                   );
        if (!EFI_ERROR (Status)) {
          if (!InUnicode) {
            //
            // Create the ASCII->Unicode conversion layer
            //
            TempHandle = CreateFileInterfaceFile (TempHandle, FALSE);
          }

          if (TempHandle == NULL) {
            Status = EFI_OUT_OF_RESOURCES;
          } else {
            ShellParameters->StdIn = TempHandle;
            gST->ConIn             = CreateSimpleTextInOnFile (TempHandle, &gST->ConsoleInHandle);
          }
        }
      }
    }
  }

  FreePool (CommandLineCopy);

  CalculateEfiHdrCrc (&gST->Hdr);

  if ((gST->ConIn == NULL) || (gST->ConOut == NULL)) {
    Status = EFI_OUT_OF_RESOURCES;
  }

  if (Status == EFI_NOT_FOUND) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_REDUNDA_REDIR), ShellProtocolInteractivityInfoObject.HiiHandle);
  } else if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_REDIR), ShellProtocolInteractivityInfoObject.HiiHandle);
  }

  return (Status);
}

/**
  Function will replace the current StdIn and StdOut in the ShellParameters protocol
  structure with StdIn and StdOut.  The current values are de-allocated.

  @param[in, out] ShellParameters      Pointer to parameter structure to modify.
  @param[in] OldStdIn                  Pointer to old StdIn.
  @param[in] OldStdOut                 Pointer to old StdOut.
  @param[in] OldStdErr                 Pointer to old StdErr.
  @param[in] SystemTableInfo           Pointer to old system table information.
**/
EFI_STATUS
RestoreStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN  SHELL_FILE_HANDLE                 *OldStdIn,
  IN  SHELL_FILE_HANDLE                 *OldStdOut,
  IN  SHELL_FILE_HANDLE                 *OldStdErr,
  IN  SYSTEM_TABLE_INFO                 *SystemTableInfo
  )
{
  SPLIT_LIST  *Split;

  if (  (ShellParameters == NULL)
     || (OldStdIn        == NULL)
     || (OldStdOut       == NULL)
     || (OldStdErr       == NULL)
     || (SystemTableInfo == NULL))
  {
    return (EFI_INVALID_PARAMETER);
  }

  if (!IsListEmpty (&ShellProtocolInteractivityInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST *)GetFirstNode (&ShellProtocolInteractivityInfoObject.SplitList.Link);
  } else {
    Split = NULL;
  }

  if (ShellParameters->StdIn  != *OldStdIn) {
    if (((Split != NULL) && (Split->SplitStdIn != ShellParameters->StdIn)) || (Split == NULL)) {
      gEfiShellProtocol->CloseFile (ShellParameters->StdIn);
    }

    ShellParameters->StdIn = *OldStdIn;
  }

  if (ShellParameters->StdOut != *OldStdOut) {
    if (((Split != NULL) && (Split->SplitStdOut != ShellParameters->StdOut)) || (Split == NULL)) {
      gEfiShellProtocol->CloseFile (ShellParameters->StdOut);
    }

    ShellParameters->StdOut = *OldStdOut;
  }

  if (ShellParameters->StdErr != *OldStdErr) {
    gEfiShellProtocol->CloseFile (ShellParameters->StdErr);
    ShellParameters->StdErr = *OldStdErr;
  }

  if (gST->ConIn != SystemTableInfo->ConIn) {
    CloseSimpleTextInOnFile (gST->ConIn);
    gST->ConIn           = SystemTableInfo->ConIn;
    gST->ConsoleInHandle = SystemTableInfo->ConInHandle;
  }

  if (gST->ConOut != SystemTableInfo->ConOut) {
    CloseSimpleTextOutOnFile (gST->ConOut);
    gST->ConOut           = SystemTableInfo->ConOut;
    gST->ConsoleOutHandle = SystemTableInfo->ConOutHandle;
  }

  if (gST->StdErr != SystemTableInfo->ErrOut) {
    CloseSimpleTextOutOnFile (gST->StdErr);
    gST->StdErr              = SystemTableInfo->ErrOut;
    gST->StandardErrorHandle = SystemTableInfo->ErrOutHandle;
  }

  CalculateEfiHdrCrc (&gST->Hdr);

  return (EFI_SUCCESS);
}
