/** @file
  Main file for Drivers shell Driver1 function.

  Copyright (c) 2010 - 2011, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "UefiShellDriver1CommandsLib.h"

STATIC CONST SHELL_PARAM_ITEM ParamList[] = {
  {L"-sfo", TypeFlag},
  {L"-l", TypeValue},
  {NULL, TypeMax}
  };

/**
  Get a device path (in text format) for a given handle.

  @param[in] TheHandle      The handle to get the device path for.

  @retval NULL    An error occured.
  @return         A pointer to the driver path as a string.  The callee must 
                  free this memory.
**/
CHAR16*
EFIAPI
GetDevicePathTextForHandle(
  IN EFI_HANDLE TheHandle
  )
{
  EFI_STATUS                Status;
  EFI_LOADED_IMAGE_PROTOCOL *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL  *ImageDevicePath;
  EFI_DEVICE_PATH_PROTOCOL  *FinalPath;
  CHAR16                    *RetVal;

  FinalPath = NULL;

  Status = gBS->OpenProtocol (
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                (VOID**)&LoadedImage,
                gImageHandle,
                NULL,
                EFI_OPEN_PROTOCOL_GET_PROTOCOL
               );
  if (!EFI_ERROR (Status)) {
    Status = gBS->OpenProtocol (
                  LoadedImage->DeviceHandle,
                  &gEfiDevicePathProtocolGuid,
                  (VOID**)&ImageDevicePath,
                  gImageHandle,
                  NULL,
                  EFI_OPEN_PROTOCOL_GET_PROTOCOL
                 );
    if (!EFI_ERROR (Status)) {
      FinalPath = AppendDevicePath (ImageDevicePath, LoadedImage->FilePath);
      gBS->CloseProtocol(
        LoadedImage->DeviceHandle,
        &gEfiDevicePathProtocolGuid,
        gImageHandle,
        NULL);
    }
    gBS->CloseProtocol(
                TheHandle,
                &gEfiLoadedImageProtocolGuid,
                gImageHandle,
                NULL);
  }

  if (FinalPath == NULL) {
    return (NULL);
  }
  RetVal = gEfiShellProtocol->GetFilePathFromDevicePath(FinalPath);
  if (RetVal == NULL) {
    RetVal = gDevPathToText->ConvertDevicePathToText(FinalPath, TRUE, TRUE);
  }
  FreePool(FinalPath);
  return (RetVal);
}

/**
  Determine if the given handle has Driver Configuration protocol.

  @param[in] TheHandle      The handle to the driver to test.

  @retval TRUE              The driver does have Driver Configuration.
  @retval FALSE             The driver does not have Driver Configuration.
**/
BOOLEAN
EFIAPI
ReturnDriverConfig(
  IN CONST EFI_HANDLE TheHandle
  )
{
  EFI_STATUS                  Status;
  Status = gBS->OpenProtocol((EFI_HANDLE)TheHandle, &gEfiDriverConfigurationProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
  if (EFI_ERROR(Status)) {
    return (FALSE);
  }
  return (TRUE);
}

/**
  Determine if the given handle has DriverDiagnostics protocol.

  @param[in] TheHandle      The handle to the driver to test.

  @retval TRUE              The driver does have Driver Diasgnostics.
  @retval FALSE             The driver does not have Driver Diagnostics.
**/
BOOLEAN
EFIAPI
ReturnDriverDiag(
  IN CONST EFI_HANDLE TheHandle
  )
{
  EFI_STATUS                  Status;
  Status = gBS->OpenProtocol((EFI_HANDLE)TheHandle, &gEfiDriverDiagnostics2ProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
  if (EFI_ERROR(Status)) {
    Status = gBS->OpenProtocol((EFI_HANDLE)TheHandle, &gEfiDriverDiagnosticsProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL);
    if (EFI_ERROR(Status)) {
      return (FALSE);
    }
  }
  return (TRUE);
}

/**
  Finds and returns the version of the driver specified by TheHandle.

  @param[in] TheHandle      The driver handle to get the version of.

  @return             The version of the driver.
  @retval 0xFFFFFFFF  An error ocurred.
**/
UINT32
EFIAPI
ReturnDriverVersion(
  IN CONST EFI_HANDLE TheHandle
  )
{
  EFI_DRIVER_BINDING_PROTOCOL *DriverBinding;
  EFI_STATUS                  Status;
  UINT32                      RetVal;

  RetVal = (UINT32)-1;

  Status = gBS->OpenProtocol((EFI_HANDLE)TheHandle, &gEfiDriverBindingProtocolGuid, (VOID**)&DriverBinding, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (!EFI_ERROR(Status)) {
    RetVal = DriverBinding->Version;
    gBS->CloseProtocol(TheHandle, &gEfiDriverBindingProtocolGuid, gImageHandle, NULL);
  }
  return (RetVal);
}

/**
  Function for 'drivers' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDrivers (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS          Status;
  LIST_ENTRY          *Package;
  CHAR16              *ProblemParam;
  SHELL_STATUS        ShellStatus;
  CHAR8               *Language;
  CONST CHAR16        *Lang;
  EFI_HANDLE          *HandleList;
  EFI_HANDLE          *HandleWalker;
  UINTN               ChildCount;
  UINTN               DeviceCount;
  CHAR16              *Temp2;
  CHAR16              *FormatString;
  UINT32              DriverVersion;
  BOOLEAN             DriverConfig;
  BOOLEAN             DriverDiag;

  ShellStatus         = SHELL_SUCCESS;
  Status              = EFI_SUCCESS;
  Language            = NULL;
  FormatString        = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize();
  ASSERT_EFI_ERROR(Status);

  Status = CommandInit();
  ASSERT_EFI_ERROR(Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamList, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR(Status)) {
    if (Status == EFI_VOLUME_CORRUPTED && ProblemParam != NULL) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, ProblemParam);
      FreePool(ProblemParam);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      ASSERT(FALSE);
    }
  } else {
    if (ShellCommandLineGetCount(Package) > 1) {
      ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle);
      ShellStatus = SHELL_INVALID_PARAMETER;
    } else {
      Lang = ShellCommandLineGetValue(Package, L"-l");
      if (Lang != NULL) {
        Language = AllocateZeroPool(StrSize(Lang));
        AsciiSPrint(Language, StrSize(Lang), "%S", Lang);
      } else if (!ShellCommandLineGetFlag(Package, L"-l")){
        ASSERT(Language == NULL);
  //      Language = AllocateZeroPool(10);
  //      AsciiSPrint(Language, 10, "en-us");
      } else {
        ASSERT(Language == NULL);
        ShellPrintHiiEx(-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"-l");
        ShellCommandLineFreeVarList (Package);
        return (SHELL_INVALID_PARAMETER);
      }

      if (ShellCommandLineGetFlag(Package, L"-sfo")) {
        FormatString = HiiGetString(gShellDriver1HiiHandle, STRING_TOKEN(STR_DRIVERS_ITEM_LINE_SFO), Language);
      } else {
        FormatString = HiiGetString(gShellDriver1HiiHandle, STRING_TOKEN(STR_DRIVERS_ITEM_LINE), Language);
        //
        // print the header row
        //
        ShellPrintHiiEx(
          -1,
          -1,
          Language,
          STRING_TOKEN(STR_DRIVERS_HEADER_LINES),
          gShellDriver1HiiHandle);
      }

      HandleList = GetHandleListByProtocol(&gEfiDriverBindingProtocolGuid);
      for (HandleWalker = HandleList ; HandleWalker != NULL && *HandleWalker != NULL ; HandleWalker++){
        ChildCount    = 0;
        DeviceCount   = 0;
        Status        = ParseHandleDatabaseForChildDevices (*HandleWalker, &ChildCount , NULL);
        Status        = PARSE_HANDLE_DATABASE_DEVICES      (*HandleWalker, &DeviceCount, NULL);
        Temp2         = GetDevicePathTextForHandle(*HandleWalker);
        DriverVersion = ReturnDriverVersion(*HandleWalker);
        DriverConfig  = ReturnDriverConfig(*HandleWalker);
        DriverDiag    = ReturnDriverDiag  (*HandleWalker);
        Lang          = GetStringNameFromHandle(*HandleWalker, Language==NULL?"en":Language);

        ShellPrintEx(
          -1,
          -1,
          FormatString,
          ConvertHandleToHandleIndex(*HandleWalker),
          DriverVersion,
          ChildCount > 0?L'B':(DeviceCount > 0?L'D':L'?'),
          DriverConfig?L'Y':L'N',
          DriverDiag?L'Y':L'N',
          DeviceCount,
          ChildCount,
          Lang,
          Temp2==NULL?L"":Temp2
         );
        if (Temp2 != NULL) {
          FreePool(Temp2);
        }
      }
    }
    SHELL_FREE_NON_NULL(Language);
    ShellCommandLineFreeVarList (Package);
    SHELL_FREE_NON_NULL(FormatString);
  }

  return (ShellStatus);
}
