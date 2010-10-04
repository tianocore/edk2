/** @file
  Member functions of EFI_SHELL_PARAMETERS_PROTOCOL and functions for creation,
  manipulation, and initialization of EFI_SHELL_PARAMETERS_PROTOCOL.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "ShellParametersProtocol.h"

/**
  return the next parameter from a command line string;

  This function moves the next parameter from Walker into TempParameter and moves
  Walker up past that parameter for recursive calling.  When the final parameter
  is moved *Walker will be set to NULL;

  Temp Parameter must be large enough to hold the parameter before calling this
  function.

  @param[in,out] Walker        pointer to string of command line.  Adjusted to
                                reminaing command line on return
  @param[in,out] TempParameter pointer to string of command line item extracted.

**/
VOID
EFIAPI
GetNextParameter(
  CHAR16 **Walker,
  CHAR16 **TempParameter
  )
{
  CHAR16 *NextDelim;
  CHAR16 *TempLoc;

  ASSERT(Walker           != NULL);
  ASSERT(*Walker          != NULL);
  ASSERT(TempParameter    != NULL);
  ASSERT(*TempParameter   != NULL);

  //
  // make sure we dont have any leading spaces
  //
  while ((*Walker)[0] == L' ') {
      (*Walker)++;
  }

  //
  // make sure we still have some params now...
  //
  if (StrLen(*Walker) == 0) {
    ASSERT((*Walker)[0] == CHAR_NULL);
    *Walker = NULL;
    return;
  }

  //
  // we have a quoted parameter
  // could be the last parameter, but SHOULD have a trailing quote
  //
  if ((*Walker)[0] == L'\"') {
    NextDelim = NULL;
    for (TempLoc = *Walker + 1 ; TempLoc != NULL && *TempLoc != CHAR_NULL ; TempLoc++) {
      if (*TempLoc == L'^' && *(TempLoc+1) == L'^') {
        TempLoc++;
      } else if (*TempLoc == L'^' && *(TempLoc+1) == L'\"') {
        TempLoc++;
      } else if (*TempLoc == L'^' && *(TempLoc+1) == L'|') {
        TempLoc++;
      } else if (*TempLoc == L'^') {
        *TempLoc = L' ';
      } else if (*TempLoc == L'\"') {
        NextDelim = TempLoc;
        break;
      }
    }

    if (NextDelim - ((*Walker)+1) == 0) {
      //
      // found ""
      //
      StrCpy(*TempParameter, L"");
      *Walker = NextDelim + 1;
    } else if (NextDelim != NULL) {
      StrnCpy(*TempParameter, (*Walker)+1, NextDelim - ((*Walker)+1));
      *Walker = NextDelim + 1;
    } else {
      //
      // last one... someone forgot the training quote!
      //
      StrCpy(*TempParameter, *Walker);
      *Walker = NULL;
    }
    for (TempLoc = *TempParameter ; TempLoc != NULL && *TempLoc != CHAR_NULL ; TempLoc++) {
      if ((*TempLoc == L'^' && *(TempLoc+1) == L'^')
       || (*TempLoc == L'^' && *(TempLoc+1) == L'|')
       || (*TempLoc == L'^' && *(TempLoc+1) == L'\"')
      ){
        CopyMem(TempLoc, TempLoc+1, StrSize(TempLoc) - sizeof(TempLoc[0]));
      }
    }
  } else {
    //
    // we have a regular parameter (no quote) OR
    // we have the final parameter (no trailing space)
    //
    NextDelim = StrStr((*Walker), L" ");
    if (NextDelim != NULL) {
      StrnCpy(*TempParameter, *Walker, NextDelim - (*Walker));
      (*TempParameter)[NextDelim - (*Walker)] = CHAR_NULL;
      *Walker = NextDelim+1;
    } else {
      //
      // last one.
      //
      StrCpy(*TempParameter, *Walker);
      *Walker = NULL;
    }
    for (NextDelim = *TempParameter ; NextDelim != NULL && *NextDelim != CHAR_NULL ; NextDelim++) {
      if (*NextDelim == L'^' && *(NextDelim+1) == L'^') {
        CopyMem(NextDelim, NextDelim+1, StrSize(NextDelim) - sizeof(NextDelim[0]));
      } else if (*NextDelim == L'^') {
        *NextDelim = L' ';
      }
    }
    while ((*TempParameter)[StrLen(*TempParameter)-1] == L' ') {
      (*TempParameter)[StrLen(*TempParameter)-1] = CHAR_NULL;
    }
    while ((*TempParameter)[0] == L' ') {
      CopyMem(*TempParameter, (*TempParameter)+1, StrSize(*TempParameter) - sizeof((*TempParameter)[0]));
    }
  }
  return;
}

/**
  function to populate Argc and Argv.

  This function parses the CommandLine and divides it into standard C style Argc/Argv
  parameters for inclusion in EFI_SHELL_PARAMETERS_PROTOCOL.  this supports space
  delimited and quote surrounded parameter definition.

  @param[in] CommandLine        String of command line to parse
  @param[in,out] Argv           pointer to array of strings; one for each parameter
  @param[in,out] Argc           pointer to number of strings in Argv array

  @return EFI_SUCCESS           the operation was sucessful
  @return EFI_OUT_OF_RESOURCES  a memory allocation failed.
**/
EFI_STATUS
EFIAPI
ParseCommandLineToArgs(
  IN CONST CHAR16 *CommandLine,
  IN OUT CHAR16 ***Argv,
  IN OUT UINTN *Argc
  )
{
  UINTN       Count;
  CHAR16      *TempParameter;
  CHAR16      *Walker;
  CHAR16      *NewParam;
  UINTN       Size;

  ASSERT(Argc != NULL);
  ASSERT(Argv != NULL);

  if (CommandLine == NULL || StrLen(CommandLine)==0) {
    (*Argc) = 0;
    (*Argv) = NULL;
    return (EFI_SUCCESS);
  }

  Size = StrSize(CommandLine);
  TempParameter = AllocateZeroPool(Size);
  if (TempParameter == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  for ( Count = 0
      , Walker = (CHAR16*)CommandLine
      ; Walker != NULL && *Walker != CHAR_NULL
      ; GetNextParameter(&Walker, &TempParameter)
      , Count++
     );

/*  Count = 0;
  Walker = (CHAR16*)CommandLine;
  while(Walker != NULL) {
    GetNextParameter(&Walker, &TempParameter);
    Count++;
  }
*/
  //
  // lets allocate the pointer array
  //
  (*Argv) = AllocateZeroPool((Count)*sizeof(CHAR16*));
  if (*Argv == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  *Argc = 0;
  Walker = (CHAR16*)CommandLine;
  while(Walker != NULL && *Walker != CHAR_NULL) {
    SetMem16(TempParameter, Size, CHAR_NULL);
    GetNextParameter(&Walker, &TempParameter);
    NewParam = AllocateZeroPool(StrSize(TempParameter));
    ASSERT(NewParam != NULL);
    StrCpy(NewParam, TempParameter);
    ((CHAR16**)(*Argv))[(*Argc)] = NewParam;
    (*Argc)++;
  }
  ASSERT(Count >= (*Argc));
  return (EFI_SUCCESS);
}

/**
  creates a new EFI_SHELL_PARAMETERS_PROTOCOL instance and populates it and then
  installs it on our handle and if there is an existing version of the protocol
  that one is cached for removal later.

  @param[in,out] NewShellParameters on a successful return, a pointer to pointer
                                     to the newly installed interface.
  @param[in,out] RootShellInstance  on a successful return, pointer to boolean.
                                     TRUE if this is the root shell instance.

  @retval EFI_SUCCESS               the operation completed successfully.
  @return other                     the operation failed.
  @sa ReinstallProtocolInterface
  @sa InstallProtocolInterface
  @sa ParseCommandLineToArgs
**/
EFI_STATUS
EFIAPI
CreatePopulateInstallShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  **NewShellParameters,
  IN OUT BOOLEAN                        *RootShellInstance
  )
{
  EFI_STATUS Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  CHAR16                    *FullCommandLine;
  UINTN                     Size;

  Size = 0;
  FullCommandLine = NULL;
  LoadedImage = NULL;

  //
  // Assert for valid parameters
  //
  ASSERT(NewShellParameters != NULL);
  ASSERT(RootShellInstance  != NULL);

  //
  // See if we have a shell parameters placed on us
  //
  Status = gBS->OpenProtocol (
                gImageHandle,
                &gEfiShellParametersProtocolGuid,
                (VOID **) &ShellInfoObject.OldShellParameters,
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
  *NewShellParameters = AllocateZeroPool(sizeof(EFI_SHELL_PARAMETERS_PROTOCOL));
  if ((*NewShellParameters) == NULL) {
    return (EFI_OUT_OF_RESOURCES);
  }

  //
  // get loaded image protocol
  //
  Status = gBS->OpenProtocol (
                gImageHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID **) &LoadedImage,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
               );
  ASSERT_EFI_ERROR(Status);
  //
  // Build the full command line
  //
  Status = SHELL_GET_ENVIRONMENT_VARIABLE(L"ShellOpt", &Size, &FullCommandLine);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    FullCommandLine = AllocateZeroPool(Size + LoadedImage->LoadOptionsSize);
    Status = SHELL_GET_ENVIRONMENT_VARIABLE(L"ShellOpt", &Size, &FullCommandLine);
  }
  if (Status == EFI_NOT_FOUND) {
    //
    // no parameters via environment... ok
    //
  } else {
    if (EFI_ERROR(Status)) {
      return (Status);
    }
  }
  if (Size == 0 && LoadedImage->LoadOptionsSize != 0) {
    ASSERT(FullCommandLine == NULL);
    //
    // Now we need to include a NULL terminator in the size.
    //
    Size = LoadedImage->LoadOptionsSize + sizeof(FullCommandLine[0]);
    FullCommandLine = AllocateZeroPool(Size);
  }
  if (FullCommandLine != NULL) {
    if (LoadedImage->LoadOptionsSize != 0){
      StrCpy(FullCommandLine, LoadedImage->LoadOptions);
    }
    //
    // Populate Argc and Argv
    //
    Status = ParseCommandLineToArgs(FullCommandLine,
                                    &(*NewShellParameters)->Argv,
                                    &(*NewShellParameters)->Argc);

    FreePool(FullCommandLine);

    ASSERT_EFI_ERROR(Status);
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
    Status = gBS->InstallProtocolInterface(&gImageHandle,
                                           &gEfiShellParametersProtocolGuid,
                                           EFI_NATIVE_INTERFACE,
                                           (VOID*)(*NewShellParameters));
  } else {
    //
    // copy from the existing ones
    //
    (*NewShellParameters)->StdIn  = ShellInfoObject.OldShellParameters->StdIn;
    (*NewShellParameters)->StdOut = ShellInfoObject.OldShellParameters->StdOut;
    (*NewShellParameters)->StdErr = ShellInfoObject.OldShellParameters->StdErr;
    Status = gBS->ReinstallProtocolInterface(gImageHandle,
                                             &gEfiShellParametersProtocolGuid,
                                             (VOID*)ShellInfoObject.OldShellParameters,
                                             (VOID*)(*NewShellParameters));
  }

  return (Status);
}

/**
  frees all memory used by createion and installation of shell parameters protocol
  and if there was an old version installed it will restore that one.

  @param NewShellParameters the interface of EFI_SHELL_PARAMETERS_PROTOCOL that is
  being cleaned up.

  @retval EFI_SUCCESS     the cleanup was successful
  @return other           the cleanup failed
  @sa ReinstallProtocolInterface
  @sa UninstallProtocolInterface
**/
EFI_STATUS
EFIAPI
CleanUpShellParametersProtocol (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *NewShellParameters
  )
{
  EFI_STATUS Status;
  UINTN LoopCounter;

  //
  // If the old exists we need to restore it
  //
  if (ShellInfoObject.OldShellParameters != NULL) {
    Status = gBS->ReinstallProtocolInterface(gImageHandle,
                                             &gEfiShellParametersProtocolGuid,
                                             (VOID*)NewShellParameters,
                                             (VOID*)ShellInfoObject.OldShellParameters);
    DEBUG_CODE(ShellInfoObject.OldShellParameters = NULL;);
  } else {
    //
    // No old one, just uninstall us...
    //
    Status = gBS->UninstallProtocolInterface(gImageHandle,
                                             &gEfiShellParametersProtocolGuid,
                                             (VOID*)NewShellParameters);
  }
  if (NewShellParameters->Argv != NULL) {
    for ( LoopCounter = 0
        ; LoopCounter < NewShellParameters->Argc
        ; LoopCounter++
       ){
      FreePool(NewShellParameters->Argv[LoopCounter]);
    }
    FreePool(NewShellParameters->Argv);
  }
  FreePool(NewShellParameters);
  return (Status);
}

/**
  Funcion will replace the current StdIn and StdOut in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  If OldStdIn or OldStdOut is NULL then that value is not returned.

  @param[in,out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine             The new command line to parse and use.
  @param[out] OldStdIn                  Pointer to old StdIn.
  @param[out] OldStdOut                 Pointer to old StdOut.
  @param[out] OldStdErr                 Pointer to old StdErr.

  @retval   EFI_SUCCESS                 Operation was sucessful, Argv and Argc are valid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
EFIAPI
UpdateStdInStdOutStdErr(
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CONST CHAR16                       *NewCommandLine,
  OUT SHELL_FILE_HANDLE                 *OldStdIn,
  OUT SHELL_FILE_HANDLE                 *OldStdOut,
  OUT SHELL_FILE_HANDLE                 *OldStdErr
  )
{
  CHAR16            *CommandLineCopy;
  CHAR16            *CommandLineWalker;
  CHAR16            *StdErrFileName;
  CHAR16            *StdOutFileName;
  CHAR16            *StdInFileName;
  CHAR16            *StdInVarName;
  CHAR16            *StdOutVarName;
  CHAR16            *StdErrVarName;
  EFI_STATUS        Status;
  SHELL_FILE_HANDLE TempHandle;
  UINT64            FileSize;
  BOOLEAN           OutUnicode;
  BOOLEAN           InUnicode;
  BOOLEAN           ErrUnicode;
  BOOLEAN           OutAppend;
  BOOLEAN           ErrAppend;
  UINTN             Size;
  CHAR16            TagBuffer[2];
  SPLIT_LIST        *Split;

  ASSERT(ShellParameters != NULL);
  OutUnicode      = TRUE;
  InUnicode       = TRUE;
  ErrUnicode      = TRUE;
  StdInVarName    = NULL;
  StdOutVarName   = NULL;
  StdErrVarName   = NULL;
  StdErrFileName  = NULL;
  StdInFileName   = NULL;
  StdOutFileName  = NULL;
  ErrAppend       = FALSE;
  OutAppend       = FALSE;
  CommandLineCopy = NULL;

  if (OldStdIn != NULL) {
    *OldStdIn = ShellParameters->StdIn;
  }
  if (OldStdOut != NULL) {
    *OldStdOut = ShellParameters->StdOut;
  }
  if (OldStdErr != NULL) {
    *OldStdErr = ShellParameters->StdErr;
  }

  if (NewCommandLine == NULL) {
    return (EFI_SUCCESS);
  }

  CommandLineCopy = StrnCatGrow(&CommandLineCopy, NULL, NewCommandLine, 0);
  Status          = EFI_SUCCESS;
  Split           = NULL;

  if (!IsListEmpty(&ShellInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST*)GetFirstNode(&ShellInfoObject.SplitList.Link);
    if (Split != NULL && Split->SplitStdIn != NULL) {
      ShellParameters->StdIn  = Split->SplitStdIn;
    }
    if (Split != NULL && Split->SplitStdOut != NULL) {
      ShellParameters->StdOut = Split->SplitStdOut;
    }
  }

  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 2>>v ")) != NULL) {
    StdErrVarName   = CommandLineWalker += 6;
    ErrAppend       = TRUE;
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 1>>v ")) != NULL) {
    StdOutVarName   = CommandLineWalker += 6;
    OutAppend       = TRUE;
  } else if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" >>v ")) != NULL) {
    StdOutVarName   = CommandLineWalker += 5;
    OutAppend       = TRUE;
  } else if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" >v ")) != NULL) {
    StdOutVarName   = CommandLineWalker += 4;
    OutAppend       = FALSE;
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 1>>a ")) != NULL) {
    StdOutFileName  = CommandLineWalker += 6;
    OutAppend       = TRUE;
    OutUnicode      = FALSE;
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 1>> ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 5;
      OutAppend       = TRUE;
    }
  } 
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" >> ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 4;
      OutAppend       = TRUE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" >>a ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 5;
      OutAppend       = TRUE;
      OutUnicode      = FALSE;
    }
  } 
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 1>a ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 5;
      OutAppend       = FALSE;
      OutUnicode      = FALSE;
    }
  } 
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" >a ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 4;
      OutAppend       = FALSE;
      OutUnicode      = FALSE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 2>> ")) != NULL) {
    if (StdErrFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrFileName  = CommandLineWalker += 5;
      ErrAppend       = TRUE;
    }
  }

  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 2>v ")) != NULL) {
    if (StdErrVarName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrVarName   = CommandLineWalker += 5;
      ErrAppend       = FALSE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 1>v ")) != NULL) {
    if (StdOutVarName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutVarName   = CommandLineWalker += 5;
      OutAppend       = FALSE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 2>a ")) != NULL) {
    if (StdErrFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrFileName  = CommandLineWalker += 5;
      ErrAppend       = FALSE;
      ErrUnicode      = FALSE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 2> ")) != NULL) {
    if (StdErrFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdErrFileName  = CommandLineWalker += 4;
      ErrAppend       = FALSE;
    }
  }

  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" 1> ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 4;
      OutAppend       = FALSE;
    }
  }

  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" > ")) != NULL) {
    if (StdOutFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdOutFileName  = CommandLineWalker += 3;
      OutAppend       = FALSE;
    }
  }

  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" < ")) != NULL) {
    if (StdInFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdInFileName  = CommandLineWalker += 3;
      OutAppend       = FALSE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" <a ")) != NULL) {
    if (StdInFileName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdInFileName  = CommandLineWalker += 4;
      OutAppend       = FALSE;
    }
  }
  if (!EFI_ERROR(Status) && (CommandLineWalker = StrStr(CommandLineCopy, L" <v ")) != NULL) {
    if (StdInVarName != NULL) {
      Status = EFI_INVALID_PARAMETER;
    } else {
      StdInVarName  = CommandLineWalker += 4;
      OutAppend       = FALSE;
    }
  }

  if (!EFI_ERROR(Status)) {
    if (StdErrFileName != NULL && (CommandLineWalker = StrStr(StdErrFileName, L" ")) != NULL) {
      CommandLineWalker[0] = CHAR_NULL;
    }
    if (StdOutFileName != NULL && (CommandLineWalker = StrStr(StdOutFileName, L" ")) != NULL) {
      CommandLineWalker[0] = CHAR_NULL;
    }
    if (StdInFileName  != NULL && (CommandLineWalker = StrStr(StdInFileName , L" ")) != NULL) {
      CommandLineWalker[0] = CHAR_NULL;
    }
    if (StdErrVarName  != NULL && (CommandLineWalker = StrStr(StdErrVarName , L" ")) != NULL) {
      CommandLineWalker[0] = CHAR_NULL;
    }
    if (StdOutVarName  != NULL && (CommandLineWalker = StrStr(StdOutVarName , L" ")) != NULL) {
      CommandLineWalker[0] = CHAR_NULL;
    }
    if (StdInVarName   != NULL && (CommandLineWalker = StrStr(StdInVarName  , L" ")) != NULL) {
      CommandLineWalker[0] = CHAR_NULL;
    }

    //
    // Verify not the same and not duplicating something from a split
    //
    if ((StdErrFileName != NULL && StdOutFileName!= NULL && StringNoCaseCompare(&StdErrFileName, &StdOutFileName) == 0)
      ||(StdErrFileName != NULL && StdInFileName != NULL && StringNoCaseCompare(&StdErrFileName, &StdInFileName ) == 0)
      ||(StdOutFileName != NULL && StdInFileName != NULL && StringNoCaseCompare(&StdOutFileName, &StdInFileName ) == 0)
      ||(StdErrVarName  != NULL && StdInVarName  != NULL && StringNoCaseCompare(&StdErrVarName , &StdInVarName  ) == 0)
      ||(StdOutVarName  != NULL && StdInVarName != NULL && StringNoCaseCompare(&StdOutVarName , &StdInVarName  ) == 0)
      ||(StdErrVarName  != NULL && StdOutVarName != NULL && StringNoCaseCompare(&StdErrVarName , &StdOutVarName ) == 0)
      ||(Split != NULL && Split->SplitStdIn  != NULL && (StdInVarName  != NULL || StdInFileName  != NULL))
      ||(Split != NULL && Split->SplitStdOut != NULL && (StdOutVarName != NULL || StdOutFileName != NULL))
      ||(StdErrFileName != NULL && StdErrVarName != NULL)
      ||(StdOutFileName != NULL && StdOutVarName != NULL)
      ||(StdInFileName  != NULL && StdInVarName  != NULL)
      ||(StdErrVarName  != NULL && !IsVolatileEnv(StdErrVarName))
      ||(StdOutVarName  != NULL && !IsVolatileEnv(StdOutVarName))
      ||(StrStr(NewCommandLine, L"connect -r") != NULL 
         && (StdOutVarName != NULL || StdOutFileName != NULL || StdErrFileName != NULL || StdErrVarName != NULL))
      ){
      Status = EFI_INVALID_PARAMETER;
    } else {
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
          ShellInfoObject.NewEfiShellProtocol->DeleteFileByName(StdErrFileName);
        }
        Status = ShellOpenFileByName(StdErrFileName, &TempHandle, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE,0);
        ASSERT(TempHandle != NULL);
        if (!ErrAppend && ErrUnicode && !EFI_ERROR(Status)) {
          //
          // Write out the UnicodeFileTag
          //
          Size = sizeof(CHAR16);
          TagBuffer[0] = UnicodeFileTag;
          TagBuffer[1] = CHAR_NULL;
          ShellInfoObject.NewEfiShellProtocol->WriteFile(TempHandle, &Size, TagBuffer);
        }
        if (!ErrUnicode && !EFI_ERROR(Status)) {
          TempHandle = CreateFileInterfaceFile(TempHandle, FALSE);
          ASSERT(TempHandle != NULL);
        }
        if (!EFI_ERROR(Status)) {
          ShellParameters->StdErr = TempHandle;
        }
      }

      //
      // StdOut to a file
      //
      if (!EFI_ERROR(Status) && StdOutFileName != NULL) {
        if (!OutAppend) {
          //
          // delete existing file.
          //
          ShellInfoObject.NewEfiShellProtocol->DeleteFileByName(StdOutFileName);
        }
        Status = ShellOpenFileByName(StdOutFileName, &TempHandle, EFI_FILE_MODE_WRITE|EFI_FILE_MODE_READ|EFI_FILE_MODE_CREATE,0);
        if (TempHandle == NULL) {
          Status = EFI_INVALID_PARAMETER;
        } else {
          if (!OutAppend && OutUnicode && !EFI_ERROR(Status)) {
            //
            // Write out the UnicodeFileTag
            //
            Size = sizeof(CHAR16);
            TagBuffer[0] = UnicodeFileTag;
            TagBuffer[1] = CHAR_NULL;
            ShellInfoObject.NewEfiShellProtocol->WriteFile(TempHandle, &Size, TagBuffer);
          } else if (OutAppend) {
            //
            // Move to end of file
            //
            Status = ShellInfoObject.NewEfiShellProtocol->GetFileSize(TempHandle, &FileSize);
            if (!EFI_ERROR(Status)) {
              Status = ShellInfoObject.NewEfiShellProtocol->SetFilePosition(TempHandle, FileSize);
            }
          }
          if (!OutUnicode && !EFI_ERROR(Status)) {
            TempHandle = CreateFileInterfaceFile(TempHandle, FALSE);
            ASSERT(TempHandle != NULL);
          }
          if (!EFI_ERROR(Status)) {
            ShellParameters->StdOut = TempHandle;
          }
        }
      }

      //
      // StdOut to a var
      //
      if (!EFI_ERROR(Status) && StdOutVarName != NULL) {
        if (!OutAppend) {
          //
          // delete existing variable.
          //
          SHELL_SET_ENVIRONMENT_VARIABLE_V(StdOutVarName, 0, L"");
        }
        TempHandle = CreateFileInterfaceEnv(StdOutVarName);
        ASSERT(TempHandle != NULL);
        if (!OutUnicode) {
          TempHandle = CreateFileInterfaceFile(TempHandle, FALSE);
          ASSERT(TempHandle != NULL);
        }
        ShellParameters->StdOut = TempHandle;
      }

      //
      // StdErr to a var
      //
      if (!EFI_ERROR(Status) && StdErrVarName != NULL) {
        if (!ErrAppend) {
          //
          // delete existing variable.
          //
          SHELL_SET_ENVIRONMENT_VARIABLE_V(StdErrVarName, 0, L"");
        }
        TempHandle = CreateFileInterfaceEnv(StdErrVarName);
        ASSERT(TempHandle != NULL);
        if (!ErrUnicode) {
          TempHandle = CreateFileInterfaceFile(TempHandle, FALSE);
          ASSERT(TempHandle != NULL);
        }
        ShellParameters->StdErr = TempHandle;
      }

      //
      // StdIn from a var
      //
      if (!EFI_ERROR(Status) && StdInVarName != NULL) {
        TempHandle = CreateFileInterfaceEnv(StdInVarName);
        if (!InUnicode) {
          TempHandle = CreateFileInterfaceFile(TempHandle, FALSE);
        }
        Size = 0;
        ASSERT(TempHandle != NULL);
        if (((EFI_FILE_PROTOCOL*)TempHandle)->Read(TempHandle, &Size, NULL) != EFI_BUFFER_TOO_SMALL) {
          Status = EFI_INVALID_PARAMETER;
        } else {
          ShellParameters->StdIn = TempHandle;
        }
      }

      //
      // StdIn from a file
      //
      if (!EFI_ERROR(Status) && StdInFileName != NULL) {
        Status = ShellOpenFileByName(
          StdInFileName,
          &TempHandle,
          EFI_FILE_MODE_READ,
          0);
        if (!InUnicode && !EFI_ERROR(Status)) {
          TempHandle = CreateFileInterfaceFile(TempHandle, FALSE);
        }
        if (!EFI_ERROR(Status)) {
          ShellParameters->StdIn = TempHandle;
        }
      }
    }
  }
  FreePool(CommandLineCopy);
  return (Status);
}

/**
  Funcion will replace the current StdIn and StdOut in the ShellParameters protocol
  structure with StdIn and StdOut.  The current values are de-allocated.

  @param[in,out] ShellParameters       pointer to parameter structure to modify
  @param[out] OldStdIn                 Pointer to old StdIn.
  @param[out] OldStdOut                Pointer to old StdOut.
  @param[out] OldStdErr                Pointer to old StdErr.
**/
EFI_STATUS
EFIAPI
RestoreStdInStdOutStdErr (
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  OUT SHELL_FILE_HANDLE                 *OldStdIn OPTIONAL,
  OUT SHELL_FILE_HANDLE                 *OldStdOut OPTIONAL,
  OUT SHELL_FILE_HANDLE                 *OldStdErr OPTIONAL
  )
{
  SPLIT_LIST        *Split;
  if (!IsListEmpty(&ShellInfoObject.SplitList.Link)) {
    Split = (SPLIT_LIST*)GetFirstNode(&ShellInfoObject.SplitList.Link);
  } else {
    Split = NULL;
  }
  if (OldStdIn  != NULL && ShellParameters->StdIn  != *OldStdIn) {
    if ((Split != NULL && Split->SplitStdIn != ShellParameters->StdIn) || Split == NULL) {
      gEfiShellProtocol->CloseFile(ShellParameters->StdIn);
    }
    ShellParameters->StdIn = OldStdIn==NULL?NULL:*OldStdIn;
  }
  if (OldStdOut != NULL && ShellParameters->StdOut != *OldStdOut) {
    if ((Split != NULL && Split->SplitStdOut != ShellParameters->StdOut) || Split == NULL) {
      gEfiShellProtocol->CloseFile(ShellParameters->StdOut);
    }
    ShellParameters->StdOut = OldStdOut==NULL?NULL:*OldStdOut;
  }
  return (EFI_SUCCESS);
}
/**
  Funcion will replace the current Argc and Argv in the ShellParameters protocol
  structure by parsing NewCommandLine.  The current values are returned to the
  user.

  If OldArgv or OldArgc is NULL then that value is not returned.

  @param[in,out] ShellParameters        Pointer to parameter structure to modify.
  @param[in] NewCommandLine             The new command line to parse and use.
  @param[out] OldArgv                   Pointer to old list of parameters.
  @param[out] OldArgc                   Pointer to old number of items in Argv list.

  @retval   EFI_SUCCESS                 Operation was sucessful, Argv and Argc are valid.
  @retval   EFI_OUT_OF_RESOURCES        A memory allocation failed.
**/
EFI_STATUS
EFIAPI
UpdateArgcArgv(
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CONST CHAR16                       *NewCommandLine,
  OUT CHAR16                            ***OldArgv OPTIONAL,
  OUT UINTN                             *OldArgc OPTIONAL
  )
{
  ASSERT(ShellParameters != NULL);

  if (OldArgc != NULL) {
    *OldArgc = ShellParameters->Argc;
  }
  if (OldArgc != NULL) {
    *OldArgv = ShellParameters->Argv;
  }

  return (ParseCommandLineToArgs(NewCommandLine, &(ShellParameters->Argv), &(ShellParameters->Argc)));
}

/**
  Funcion will replace the current Argc and Argv in the ShellParameters protocol
  structure with Argv and Argc.  The current values are de-allocated and the
  OldArgv must not be deallocated by the caller.

  @param[in,out] ShellParameters       pointer to parameter structure to modify
  @param[in] OldArgv                   pointer to old list of parameters
  @param[in] OldArgc                   pointer to old number of items in Argv list
**/
VOID
EFIAPI
RestoreArgcArgv(
  IN OUT EFI_SHELL_PARAMETERS_PROTOCOL  *ShellParameters,
  IN CHAR16                             ***OldArgv,
  IN UINTN                              *OldArgc
  )
{
  UINTN LoopCounter;
  ASSERT(ShellParameters != NULL);
  ASSERT(OldArgv         != NULL);
  ASSERT(OldArgc         != NULL);

  if (ShellParameters->Argv != NULL) {
    for ( LoopCounter = 0
        ; LoopCounter < ShellParameters->Argc
        ; LoopCounter++
       ){
      FreePool(ShellParameters->Argv[LoopCounter]);
    }
    FreePool(ShellParameters->Argv);
  }
  ShellParameters->Argv = *OldArgv;
  *OldArgv = NULL;
  ShellParameters->Argc = *OldArgc;
  *OldArgc = 0;
}
