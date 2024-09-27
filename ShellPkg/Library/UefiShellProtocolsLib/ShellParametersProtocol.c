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
#include "ShellParametersProtocol.h"
#include "ConsoleWrappers.h"
#include "ShellEnvVar.h"

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
                  (VOID **)&ShellProtocolsInfoObject.OldShellParameters,
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
    if (FullCommandLine == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    Status = SHELL_GET_ENVIRONMENT_VARIABLE (L"ShellOpt", &Size, FullCommandLine);
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
    (*NewShellParameters)->StdIn  = ShellProtocolsInfoObject.OldShellParameters->StdIn;
    (*NewShellParameters)->StdOut = ShellProtocolsInfoObject.OldShellParameters->StdOut;
    (*NewShellParameters)->StdErr = ShellProtocolsInfoObject.OldShellParameters->StdErr;
    Status                        = gBS->ReinstallProtocolInterface (
                                           gImageHandle,
                                           &gEfiShellParametersProtocolGuid,
                                           (VOID *)ShellProtocolsInfoObject.OldShellParameters,
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
  if (ShellProtocolsInfoObject.OldShellParameters != NULL) {
    Status = gBS->ReinstallProtocolInterface (
                    gImageHandle,
                    &gEfiShellParametersProtocolGuid,
                    (VOID *)NewShellParameters,
                    (VOID *)ShellProtocolsInfoObject.OldShellParameters
                    );
    DEBUG_CODE (
      ShellProtocolsInfoObject.OldShellParameters = NULL;
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
