/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  (C) Copyright 2016 Hewlett Packard Enterprise Development LP<BR>
  Copyright (C) 2014, Red Hat, Inc.
  (C) Copyright 2013 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Shell.h"

BOOLEAN  AsciiRedirection = FALSE;

/**
  Return the next parameter's end from a command line string.

  @param[in] String        the string to parse
**/
CONST CHAR16 *
FindEndOfParameter (
  IN CONST CHAR16  *String
  )
{
  CONST CHAR16  *First;
  CONST CHAR16  *CloseQuote;

  First = FindFirstCharacter (String, L" \"", L'^');

  //
  // nothing, all one parameter remaining
  //
  if (*First == CHAR_NULL) {
    return (First);
  }

  //
  // If space before a quote (or neither found, i.e. both CHAR_NULL),
  // then that's the end.
  //
  if (*First == L' ') {
    return (First);
  }

  CloseQuote = FindFirstCharacter (First+1, L"\"", L'^');

  //
  // We did not find a terminator...
  //
  if (*CloseQuote == CHAR_NULL) {
    return (NULL);
  }

  return (FindEndOfParameter (CloseQuote+1));
}

/**
  Return the next parameter from a command line string.

  This function moves the next parameter from Walker into TempParameter and moves
  Walker up past that parameter for recursive calling.  When the final parameter
  is moved *Walker will be set to NULL;

  Temp Parameter must be large enough to hold the parameter before calling this
  function.

  This will also remove all remaining ^ characters after processing.

  @param[in, out] Walker          pointer to string of command line.  Adjusted to
                                  remaining command line on return
  @param[in, out] TempParameter   pointer to string of command line item extracted.
  @param[in]      Length          buffer size of TempParameter.
  @param[in]      StripQuotation  if TRUE then strip the quotation marks surrounding
                                  the parameters.

  @return   EFI_INVALID_PARAMETER A required parameter was NULL or pointed to a NULL or empty string.
  @return   EFI_NOT_FOUND         A closing " could not be found on the specified string
**/
EFI_STATUS
GetNextParameter (
  IN OUT CHAR16   **Walker,
  IN OUT CHAR16   **TempParameter,
  IN CONST UINTN  Length,
  IN BOOLEAN      StripQuotation
  )
{
  CONST CHAR16  *NextDelim;

  if (  (Walker           == NULL)
     || (*Walker          == NULL)
     || (TempParameter    == NULL)
     || (*TempParameter   == NULL)
        )
  {
    return (EFI_INVALID_PARAMETER);
  }

  //
  // make sure we dont have any leading spaces
  //
  while ((*Walker)[0] == L' ') {
    (*Walker)++;
  }

  //
  // make sure we still have some params now...
  //
  if (StrLen (*Walker) == 0) {
    DEBUG_CODE_BEGIN ();
    *Walker = NULL;
    DEBUG_CODE_END ();
    return (EFI_INVALID_PARAMETER);
  }

  NextDelim = FindEndOfParameter (*Walker);

  if (NextDelim == NULL) {
    DEBUG_CODE_BEGIN ();
    *Walker = NULL;
    DEBUG_CODE_END ();
    return (EFI_NOT_FOUND);
  }

  StrnCpyS (*TempParameter, Length / sizeof (CHAR16), (*Walker), NextDelim - *Walker);

  //
  // Add a CHAR_NULL if we didn't get one via the copy
  //
  if (*NextDelim != CHAR_NULL) {
    (*TempParameter)[NextDelim - *Walker] = CHAR_NULL;
  }

  //
  // Update Walker for the next iteration through the function
  //
  *Walker = (CHAR16 *)NextDelim;

  //
  // Remove any non-escaped quotes in the string
  // Remove any remaining escape characters in the string
  //
  for (NextDelim = FindFirstCharacter (*TempParameter, L"\"^", CHAR_NULL)
       ; *NextDelim != CHAR_NULL
       ; NextDelim = FindFirstCharacter (NextDelim, L"\"^", CHAR_NULL)
       )
  {
    if (*NextDelim == L'^') {
      //
      // eliminate the escape ^
      //
      CopyMem ((CHAR16 *)NextDelim, NextDelim + 1, StrSize (NextDelim + 1));
      NextDelim++;
    } else if (*NextDelim == L'\"') {
      //
      // eliminate the unescaped quote
      //
      if (StripQuotation) {
        CopyMem ((CHAR16 *)NextDelim, NextDelim + 1, StrSize (NextDelim + 1));
      } else {
        NextDelim++;
      }
    }
  }

  return EFI_SUCCESS;
}

/**
  Function to populate Argc and Argv.

  This function parses the CommandLine and divides it into standard C style Argc/Argv
  parameters for inclusion in EFI_SHELL_PARAMETERS_PROTOCOL.  this supports space
  delimited and quote surrounded parameter definition.

  All special character processing (alias, environment variable, redirection,
  etc... must be complete before calling this API.

  @param[in] CommandLine          String of command line to parse
  @param[in] StripQuotation       if TRUE then strip the quotation marks surrounding
                                  the parameters.
  @param[in, out] Argv            pointer to array of strings; one for each parameter
  @param[in, out] Argc            pointer to number of strings in Argv array

  @return EFI_SUCCESS           the operation was successful
  @return EFI_INVALID_PARAMETER some parameters are invalid
  @return EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
ParseCommandLineToArgs (
  IN CONST CHAR16  *CommandLine,
  IN BOOLEAN       StripQuotation,
  IN OUT CHAR16    ***Argv,
  IN OUT UINTN     *Argc
  )
{
  UINTN       Count;
  CHAR16      *TempParameter;
  CHAR16      *Walker;
  CHAR16      *NewParam;
  CHAR16      *NewCommandLine;
  UINTN       Size;
  EFI_STATUS  Status;

  ASSERT (Argc != NULL);
  ASSERT (Argv != NULL);

  if ((CommandLine == NULL) || (StrLen (CommandLine) == 0)) {
    (*Argc) = 0;
    (*Argv) = NULL;
    return (EFI_SUCCESS);
  }

  NewCommandLine = AllocateCopyPool (StrSize (CommandLine), CommandLine);
  if (NewCommandLine == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  TrimSpaces (&NewCommandLine);
  Size          = StrSize (NewCommandLine);
  TempParameter = AllocateZeroPool (Size);
  if (TempParameter == NULL) {
    SHELL_FREE_NON_NULL (NewCommandLine);
    return (EFI_OUT_OF_RESOURCES);
  }

  for ( Count = 0,
        Walker = (CHAR16 *)NewCommandLine
        ; Walker != NULL && *Walker != CHAR_NULL
        ; Count++
        )
  {
    if (EFI_ERROR (GetNextParameter (&Walker, &TempParameter, Size, TRUE))) {
      break;
    }
  }

  //
  // lets allocate the pointer array
  //
  (*Argv) = AllocateZeroPool ((Count)*sizeof (CHAR16 *));
  if (*Argv == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto Done;
  }

  *Argc  = 0;
  Walker = (CHAR16 *)NewCommandLine;
  while (Walker != NULL && *Walker != CHAR_NULL) {
    SetMem16 (TempParameter, Size, CHAR_NULL);
    if (EFI_ERROR (GetNextParameter (&Walker, &TempParameter, Size, StripQuotation))) {
      Status = EFI_INVALID_PARAMETER;
      goto Done;
    }

    NewParam = AllocateCopyPool (StrSize (TempParameter), TempParameter);
    if (NewParam == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    ((CHAR16 **)(*Argv))[(*Argc)] = NewParam;
    (*Argc)++;
  }

  ASSERT (Count >= (*Argc));
  Status = EFI_SUCCESS;

Done:
  SHELL_FREE_NON_NULL (TempParameter);
  SHELL_FREE_NON_NULL (NewCommandLine);
  return (Status);
}

/**
  creates a new EFI_SHELL_PARAMETERS_PROTOCOL instance and populates it and then
  installs it on our handle and if there is an existing version of the protocol
  that one is cached for removal later.

  @param[in, out] NewShellParameters on a successful return, a pointer to pointer
                                     to the newly installed interface.
  @param[in, out] RootShellInstance  on a successful return, pointer to boolean.
                                     TRUE if this is the root shell instance.

  @retval EFI_SUCCESS               the operation completed successfully.
  @return other                     the operation failed.
  @sa ReinstallProtocolInterface
  @sa InstallProtocolInterface
  @sa ParseCommandLineToArgs
**/
EFI_STATUS
CreatePopulateInstallShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  **NewShellParameters,
  IN OUT BOOLEAN                        *RootShellInstance
  )
{
  EFI_STATUS                 Status;
  EFI_LOADED_IMAGE_PROTOCOL  *LoadedImage;
  CHAR16                     *FullCommandLine;
  UINTN                      Size;

  Size            = 0;
  FullCommandLine = NULL;
  LoadedImage     = NULL;

  //
  // Assert for valid parameters
  //
  ASSERT (NewShellParameters != NULL);
  ASSERT (RootShellInstance  != NULL);

  //
  // See if we have a shell parameters placed on us
  //
  Status = gBS->OpenProtocol (
                  gImageHandle,
                  &gEfiShellParametersProtocolGuid,
                  (VOID **)&ShellInfoObject.OldShellParameters,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  //
  // if we don't then we must be the root shell (error is expected)
  //
  if (EFI_ERROR (Status)) {
    *RootShellInstance = TRUE;
  }

  //
  // Allocate the new structure
  //
  *NewShellParameters = AllocateZeroPool (sizeof (EFI_SHELL_PARAMETERS_PROTOCOL));
  if ((*NewShellParameters) == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // get loaded image protocol
  //
  Status = gBS->OpenProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **)&LoadedImage,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                  );
  ASSERT_EFI_ERROR (Status);
  //
  // Build the full command line
  //
  Status = SHELL_GET_ENVIRONMENT_VARIABLE (L"ShellOpt", &Size, FullCommandLine);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FullCommandLine = AllocateZeroPool (Size + LoadedImage->LoadOptionsSize);
    Status          = SHELL_GET_ENVIRONMENT_VARIABLE (L"ShellOpt", &Size, FullCommandLine);
  }

  if (Status == EFI_NOT_FOUND) {
    //
    // no parameters via environment... ok
    //
  } else {
    if (EFI_ERROR (Status)) {
      return (Status);
    }
  }

  if ((Size == 0) && (LoadedImage->LoadOptionsSize != 0)) {
    ASSERT (FullCommandLine == NULL);
    //
    // Now we need to include a NULL terminator in the size.
    //
    Size            = LoadedImage->LoadOptionsSize + sizeof (FullCommandLine[0]);
    FullCommandLine = AllocateZeroPool (Size);
  }

  if (FullCommandLine != NULL) {
    CopyMem (FullCommandLine, LoadedImage->LoadOptions, LoadedImage->LoadOptionsSize);
    //
    // Populate Argc and Argv
    //
    Status = ParseCommandLineToArgs (
               FullCommandLine,
               TRUE,
               &(*NewShellParameters)->Argv,
               &(*NewShellParameters)->Argc
               );

    FreePool (FullCommandLine);

    ASSERT_EFI_ERROR (Status);
  } else {
    (*NewShellParameters)->Argv = NULL;
    (*NewShellParameters)->Argc = 0;
  }

  //
  // Populate the 3 faked file systems...
  //
  if (*RootShellInstance) {
    (*NewShellParameters)->StdIn  = &FileInterfaceStdIn;
    (*NewShellParameters)->StdOut = &FileInterfaceStdOut;
    (*NewShellParameters)->StdErr = &FileInterfaceStdErr;
    Status                        = gBS->InstallProtocolInterface (
                                           &gImageHandle,
                                           &gEfiShellParametersProtocolGuid,
                                           EFI_NATIVE_INTERFACE,
                                           (VOID *)(*NewShellParameters)
                                           );
  } else {
    //
    // copy from the existing ones
    //
    (*NewShellParameters)->StdIn  = ShellInfoObject.OldShellParameters->StdIn;
    (*NewShellParameters)->StdOut = ShellInfoObject.OldShellParameters->StdOut;
    (*NewShellParameters)->StdErr = ShellInfoObject.OldShellParameters->StdErr;
    Status                        = gBS->ReinstallProtocolInterface (
                                           gImageHandle,
                                           &gEfiShellParametersProtocolGuid,
                                           (VOID *)ShellInfoObject.OldShellParameters,
                                           (VOID *)(*NewShellParameters)
                                           );
  }

  return (Status);
}

/**
  frees all memory used by creation and installation of shell parameters protocol
  and if there was an old version installed it will restore that one.

  @param NewShellParameters the interface of EFI_SHELL_PARAMETERS_PROTOCOL that is
  being cleaned up.

  @retval EFI_SUCCESS     the cleanup was successful
  @return other           the cleanup failed
  @sa ReinstallProtocolInterface
  @sa UninstallProtocolInterface
**/
EFI_STATUS
CleanUpShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *NewShellParameters
  )
{
  EFI_STATUS  Status;
  UINTN       LoopCounter;

  //
  // If the old exists we need to restore it
  //
  if (ShellInfoObject.OldShellParameters != NULL) {
    Status = gBS->ReinstallProtocolInterface (
                    gImageHandle,
                    &gEfiShellParametersProtocolGuid,
                    (VOID *)NewShellParameters,
                    (VOID *)ShellInfoObject.OldShellParameters
                    );
    DEBUG_CODE (
      ShellInfoObject.OldShellParameters = NULL;
      );
  } else {
    //
    // No old one, just uninstall us...
    //
    Status = gBS->UninstallProtocolInterface (
                    gImageHandle,
                    &gEfiShellParametersProtocolGuid,
                    (VOID *)NewShellParameters
                    );
  }

  if (NewShellParameters->Argv != NULL) {
    for ( LoopCounter = 0
          ; LoopCounter < NewShellParameters->Argc
          ; LoopCounter++
          )
    {
      FreePool (NewShellParameters->Argv[LoopCounter]);
    }

    FreePool (NewShellParameters->Argv);
  }

  FreePool (NewShellParameters);
  return (Status);
}

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
  ShellInfoObject.NewEfiShellProtocol has been initialized before calling this
  function.

  @param[in] FileHandle  The file to write the unicode file tag to.

  @return  Status code from ShellInfoObject.NewEfiShellProtocol->WriteFile.
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
  Status  = ShellInfoObject.NewEfiShellProtocol->WriteFile (
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

  if (!IsListEmpty (&ShellInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST *)GetFirstNode (&ShellInfoObject.SplitList.Link);
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
          ShellInfoObject.NewEfiShellProtocol->DeleteFileByName (StdErrFileName);
        }

        Status = ShellOpenFileByName (StdErrFileName, &TempHandle, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE, 0);
        if (!ErrAppend && ErrUnicode && !EFI_ERROR (Status)) {
          Status = WriteFileTag (TempHandle);
        }

        if (!ErrUnicode && !EFI_ERROR (Status)) {
          TempHandle = CreateFileInterfaceFile (TempHandle, FALSE);
          ASSERT (TempHandle != NULL);
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
          ShellInfoObject.NewEfiShellProtocol->DeleteFileByName (StdOutFileName);
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
            Status = ShellInfoObject.NewEfiShellProtocol->GetFileSize (TempHandle, &FileSize);
            if (!EFI_ERROR (Status)) {
              //
              // When appending to a new unicode file, write the file tag.
              // Otherwise (ie. when appending to a new ASCII file, or an
              // existent file with any encoding), just seek to the end.
              //
              Status = (FileSize == 0 && OutUnicode) ?
                       WriteFileTag (TempHandle) :
                       ShellInfoObject.NewEfiShellProtocol->SetFilePosition (
                                                              TempHandle,
                                                              FileSize
                                                              );
            }
          }

          if (!OutUnicode && !EFI_ERROR (Status)) {
            TempHandle = CreateFileInterfaceFile (TempHandle, FALSE);
            ASSERT (TempHandle != NULL);
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
        ASSERT (TempHandle != NULL);
        ShellParameters->StdOut = TempHandle;
        gST->ConOut             = CreateSimpleTextOutOnFile (TempHandle, &gST->ConsoleOutHandle, gST->ConOut);
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
        ASSERT (TempHandle != NULL);
        ShellParameters->StdErr = TempHandle;
        gST->StdErr             = CreateSimpleTextOutOnFile (TempHandle, &gST->StandardErrorHandle, gST->StdErr);
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

          ShellParameters->StdIn = TempHandle;
          gST->ConIn             = CreateSimpleTextInOnFile (TempHandle, &gST->ConsoleInHandle);
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
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_REDUNDA_REDIR), ShellInfoObject.HiiHandle);
  } else if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_SHELL_INVALID_REDIR), ShellInfoObject.HiiHandle);
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

  if (!IsListEmpty (&ShellInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST *)GetFirstNode (&ShellInfoObject.SplitList.Link);
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

/**
  Function will replace the current Argc and Argv in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  If OldArgv or OldArgc is NULL then that value is not returned.

  @param[in, out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine              The new command line to parse and use.
  @param[in] Type                        The type of operation.
  @param[out] OldArgv                    Pointer to old list of parameters.
  @param[out] OldArgc                    Pointer to old number of items in Argv list.


  @retval   EFI_SUCCESS                 Operation was successful, Argv and Argc are valid.
  @return   EFI_INVALID_PARAMETER       Some parameters are invalid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
UpdateArgcArgv (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CONST CHAR16                       *NewCommandLine,
  IN SHELL_OPERATION_TYPES              Type,
  OUT CHAR16                            ***OldArgv OPTIONAL,
  OUT UINTN                             *OldArgc OPTIONAL
  )
{
  BOOLEAN  StripParamQuotation;

  ASSERT (ShellParameters != NULL);
  StripParamQuotation = TRUE;

  if (OldArgc != NULL) {
    *OldArgc = ShellParameters->Argc;
  }

  if (OldArgc != NULL) {
    *OldArgv = ShellParameters->Argv;
  }

  if (Type == Script_File_Name) {
    StripParamQuotation = FALSE;
  }

  return ParseCommandLineToArgs (
           NewCommandLine,
           StripParamQuotation,
           &(ShellParameters->Argv),
           &(ShellParameters->Argc)
           );
}

/**
  Function will replace the current Argc and Argv in the ShellParameters protocol
  structure with Argv and Argc.  The current values are de-allocated and the
  OldArgv must not be deallocated by the caller.

  @param[in, out] ShellParameters       pointer to parameter structure to modify
  @param[in] OldArgv                    pointer to old list of parameters
  @param[in] OldArgc                    pointer to old number of items in Argv list
**/
VOID
RestoreArgcArgv (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             ***OldArgv,
  IN UINTN                              *OldArgc
  )
{
  UINTN  LoopCounter;

  ASSERT (ShellParameters != NULL);
  ASSERT (OldArgv         != NULL);
  ASSERT (OldArgc         != NULL);

  if (ShellParameters->Argv != NULL) {
    for ( LoopCounter = 0
          ; LoopCounter < ShellParameters->Argc
          ; LoopCounter++
          )
    {
      FreePool (ShellParameters->Argv[LoopCounter]);
    }

    FreePool (ShellParameters->Argv);
  }

  ShellParameters->Argv = *OldArgv;
  *OldArgv              = NULL;
  ShellParameters->Argc = *OldArgc;
  *OldArgc              = 0;
}
