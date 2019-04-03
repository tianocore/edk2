/** @file
  Main file for attrib shell level 2 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellLevel2CommandsLib.h"

// This function was from from the BdsLib implementation in
// IntelFrameworkModulePkg\Library\GenericBdsLib\BdsConnect.c
// function name: BdsLibConnectAllEfi
/**
  This function will connect all current system handles recursively. The
  connection will finish until every handle's child handle created if it have.

  @retval EFI_SUCCESS           All handles and it's child handle have been
                                connected
  @retval EFI_STATUS            Return the status of gBS->LocateHandleBuffer().

**/
EFI_STATUS
ConnectAllEfi (
  VOID
  )
{
  EFI_STATUS  Status;
  UINTN       HandleCount;
  EFI_HANDLE  *HandleBuffer;
  UINTN       Index;

  Status = gBS->LocateHandleBuffer (
                  AllHandles,
                  NULL,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                 );
  if (EFI_ERROR (Status)) {
    return Status;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->ConnectController (HandleBuffer[Index], NULL, NULL, TRUE);
  }

  if (HandleBuffer != NULL) {
    FreePool (HandleBuffer);
  }

  return EFI_SUCCESS;
}

/**
  function to load a .EFI driver into memory and possible connect the driver.

  if FileName is NULL then ASSERT.

  @param[in] FileName           FileName of the driver to load
  @param[in] Connect            Whether to connect or not

  @retval EFI_SUCCESS           the driver was loaded and if Connect was
                                true then connect was attempted. Connection may
                                have failed.
  @retval EFI_OUT_OF_RESOURCES  there was insufficient memory
**/
EFI_STATUS
LoadDriver(
  IN CONST CHAR16   *FileName,
  IN CONST BOOLEAN  Connect
  )
{
  EFI_HANDLE                    LoadedDriverHandle;
  EFI_STATUS                    Status;
  EFI_DEVICE_PATH_PROTOCOL      *FilePath;
  EFI_LOADED_IMAGE_PROTOCOL     *LoadedDriverImage;

  LoadedDriverImage   = NULL;
  FilePath            = NULL;
  LoadedDriverHandle  = NULL;
  Status              = EFI_SUCCESS;

  ASSERT (FileName != NULL);

  //
  // Fix local copies of the protocol pointers
  //
  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // Convert to DEVICE_PATH
  //
  FilePath = gEfiShellProtocol->GetDevicePathFromFilePath(FileName);

  if (FilePath == NULL) {
    ASSERT(FALSE);
    return (EFI_INVALID_PARAMETER);
  }

  //
  // Use LoadImage to get it into memory
  //
  Status = gBS->LoadImage(
    FALSE,
    gImageHandle,
    FilePath,
    NULL,
    0,
    &LoadedDriverHandle);

  if (EFI_ERROR(Status)) {
    ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOAD_NOT_IMAGE), gShellLevel2HiiHandle, FileName, Status);
  } else {
    //
    // Make sure it is a driver image
    //
    Status = gBS->HandleProtocol (LoadedDriverHandle, &gEfiLoadedImageProtocolGuid, (VOID *) &LoadedDriverImage);

    ASSERT (LoadedDriverImage != NULL);

    if ( EFI_ERROR(Status)
      || ( LoadedDriverImage->ImageCodeType != EfiBootServicesCode
        && LoadedDriverImage->ImageCodeType != EfiRuntimeServicesCode)
     ){
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOAD_NOT_DRIVER), gShellLevel2HiiHandle, FileName);

      //
      // Exit and unload the non-driver image
      //
      gBS->Exit(LoadedDriverHandle, EFI_INVALID_PARAMETER, 0, NULL);
      Status = EFI_INVALID_PARAMETER;
    }
  }

  if (!EFI_ERROR(Status)) {
    //
    // Start the image
    //
    Status = gBS->StartImage(LoadedDriverHandle, NULL, NULL);
    if (EFI_ERROR(Status)) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOAD_ERROR), gShellLevel2HiiHandle, FileName, Status);
    } else {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_LOAD_LOADED), gShellLevel2HiiHandle, FileName, LoadedDriverImage->ImageBase, Status);
    }
  }

  if (!EFI_ERROR(Status) && Connect) {
    //
    // Connect it...
    //
    Status = ConnectAllEfi();
  }

  //
  // clean up memory...
  //
  if (FilePath != NULL) {
    FreePool(FilePath);
  }

  return (Status);
}

STATIC CONST SHELL_PARAM_ITEM LoadParamList[] = {
  {L"-nc", TypeFlag},
  {NULL, TypeMax}
  };

/**
  Function for 'load' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunLoad (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  UINTN               ParamCount;
  EFI_SHELL_FILE_INFO *ListHead;
  EFI_SHELL_FILE_INFO *Node;

  ListHead            = NULL;
  ProblemParam        = NULL;
  ShellStatus         = SHELL_SUCCESS;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (LoadParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellLevel2HiiHandle, L"load", ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    //
    // check for "-?"
    //
    if (ShellCommandLineGetFlag(Package, L"-?")) {
      ASSERT(FALSE);
    } else if (ShellCommandLineGetRawValue(Package, 1) == NULL) {
      //
      // we didnt get a single file to load parameter
      //
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_FEW), gShellLevel2HiiHandle, L"load");
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      for ( ParamCount = 1
          ; ShellCommandLineGetRawValue(Package, ParamCount) != NULL
          ; ParamCount++
         ){
        Status = ShellOpenFileMetaArg((CHAR16*)ShellCommandLineGetRawValue(Package, ParamCount), EFI_FILE_MODE_READ, &ListHead);
        if (!EFI_ERROR(Status)) {
          for ( Node = (EFI_SHELL_FILE_INFO *)GetFirstNode(&ListHead->Link)
              ; !IsNull(&ListHead->Link, &Node->Link)
              ; Node = (EFI_SHELL_FILE_INFO *)GetNextNode(&ListHead->Link, &Node->Link)
             ){
            //
            // once we have an error preserve that value, but finish the loop.
            //
            if (EFI_ERROR(Status)) {
              LoadDriver(Node->FullName, (BOOLEAN)(ShellCommandLineGetFlag(Package, L"-nc")==FALSE));
            } else {
              Status = LoadDriver(Node->FullName, (BOOLEAN)(ShellCommandLineGetFlag(Package, L"-nc")==FALSE));
            }
          } // for loop for multi-open
          if (EFI_ERROR(Status)) {
            ShellCloseFileMetaArg(&ListHead);
          } else {
            Status = ShellCloseFileMetaArg(&ListHead);;
          }
        } else {
          //
          // no files found.
          //
          ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_NF), gShellLevel2HiiHandle, L"load", (CHAR16*)ShellCommandLineGetRawValue(Package, ParamCount));
          ShellStatus = SHELL_NOT_FOUND;
        }
      } // for loop for params
    }

    //
    // free the command line package
    //
    ShellCommandLineFreeVarList (Package);
  }

  if (EFI_ERROR(Status) && ShellStatus == SHELL_SUCCESS) {
    ShellStatus = SHELL_DEVICE_ERROR;
  }

  return (ShellStatus);
}
