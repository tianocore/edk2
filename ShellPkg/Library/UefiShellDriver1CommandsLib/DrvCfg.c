/** @file
  Main file for DrvCfg shell Driver1 function.

  (C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
  Copyright (c) 2010 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "UefiShellDriver1CommandsLib.h"
#include <Protocol/HiiConfigAccess.h>
#include <Protocol/HiiDatabase.h>

STATIC CONST EFI_GUID  *CfgGuidList[] = { &gEfiDriverConfigurationProtocolGuid, &gEfiDriverConfiguration2ProtocolGuid, NULL };

/**
  Find the EFI_HII_HANDLE by device path.

  @param[in] DevPath1     The Device Path to match.
  @param[out] HiiHandle   The EFI_HII_HANDLE after the converstion.
  @param[in] HiiDb        The Hii database protocol

  @retval EFI_SUCCESS     The operation was successful.
  @retval EFI_NOT_FOUND   There was no EFI_HII_HANDLE found for that deviec path.
**/
EFI_STATUS
FindHiiHandleViaDevPath (
  IN CONST EFI_DEVICE_PATH_PROTOCOL  *DevPath1,
  OUT EFI_HII_HANDLE                 *HiiHandle,
  IN EFI_HII_DATABASE_PROTOCOL       *HiiDb
  )
{
  EFI_HII_HANDLE               *HandleBuffer;
  UINTN                        HandleBufferSize;
  VOID                         *MainBuffer;
  UINTN                        MainBufferSize;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageListHeader;
  EFI_HII_PACKAGE_HEADER       *PackageHeader;
  UINTN                        LoopVariable;
  EFI_DEVICE_PATH_PROTOCOL     *DevPath2;
  EFI_STATUS                   Status;

  ASSERT (DevPath1 != NULL);
  ASSERT (HiiHandle != NULL);
  ASSERT (*HiiHandle == NULL);
  ASSERT (HiiDb != NULL);

  HandleBufferSize = 0;
  HandleBuffer     = NULL;
  Status           = HiiDb->ListPackageLists (HiiDb, EFI_HII_PACKAGE_DEVICE_PATH, NULL, &HandleBufferSize, HandleBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    HandleBuffer = AllocateZeroPool (HandleBufferSize);
    if (HandleBuffer == NULL) {
      Status = EFI_OUT_OF_RESOURCES;
    } else {
      Status = HiiDb->ListPackageLists (HiiDb, EFI_HII_PACKAGE_DEVICE_PATH, NULL, &HandleBufferSize, HandleBuffer);
    }
  }

  if (EFI_ERROR (Status)) {
    SHELL_FREE_NON_NULL (HandleBuffer);
    return (Status);
  }

  if (HandleBuffer == NULL) {
    return EFI_NOT_FOUND;
  }

  for (LoopVariable = 0; LoopVariable < (HandleBufferSize/sizeof (HandleBuffer[0])) && *HiiHandle == NULL; LoopVariable++) {
    MainBufferSize = 0;
    MainBuffer     = NULL;
    Status         = HiiDb->ExportPackageLists (HiiDb, HandleBuffer[LoopVariable], &MainBufferSize, MainBuffer);
    if (Status == EFI_BUFFER_TOO_SMALL) {
      MainBuffer = AllocateZeroPool (MainBufferSize);
      if (MainBuffer != NULL) {
        Status = HiiDb->ExportPackageLists (HiiDb, HandleBuffer[LoopVariable], &MainBufferSize, MainBuffer);
      }
    }

    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Enumerate through the block of returned memory.
    // This should actually be a small block, but we need to be sure.
    //
    for (PackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *)MainBuffer
         ; PackageListHeader != NULL && ((CHAR8 *)PackageListHeader) < (((CHAR8 *)MainBuffer)+MainBufferSize) && *HiiHandle == NULL
         ; PackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *)(((CHAR8 *)(PackageListHeader)) + PackageListHeader->PackageLength))
    {
      for (PackageHeader = (EFI_HII_PACKAGE_HEADER *)(((CHAR8 *)(PackageListHeader))+sizeof (EFI_HII_PACKAGE_LIST_HEADER))
           ; PackageHeader != NULL && ((CHAR8 *)PackageHeader) < (((CHAR8 *)MainBuffer)+MainBufferSize) && PackageHeader->Type != EFI_HII_PACKAGE_END && *HiiHandle == NULL
           ; PackageHeader = (EFI_HII_PACKAGE_HEADER *)(((CHAR8 *)(PackageHeader))+PackageHeader->Length))
      {
        if (PackageHeader->Type == EFI_HII_PACKAGE_DEVICE_PATH) {
          DevPath2 = (EFI_DEVICE_PATH_PROTOCOL *)(((CHAR8 *)PackageHeader) + sizeof (EFI_HII_PACKAGE_HEADER));
          if (DevicePathCompare (&DevPath1, &DevPath2) == 0) {
            *HiiHandle = HandleBuffer[LoopVariable];
            break;
          }
        }
      }
    }

    SHELL_FREE_NON_NULL (MainBuffer);
  }

  SHELL_FREE_NON_NULL (HandleBuffer);

  if (*HiiHandle == NULL) {
    return (EFI_NOT_FOUND);
  }

  return (EFI_SUCCESS);
}

/**
  Convert a EFI_HANDLE to a EFI_HII_HANDLE.

  @param[in] Handle       The EFI_HANDLE to convert.
  @param[out] HiiHandle   The EFI_HII_HANDLE after the converstion.
  @param[in] HiiDb        The Hii database protocol

  @retval EFI_SUCCESS   The operation was successful.
**/
EFI_STATUS
ConvertHandleToHiiHandle (
  IN CONST EFI_HANDLE           Handle,
  OUT EFI_HII_HANDLE            *HiiHandle,
  IN EFI_HII_DATABASE_PROTOCOL  *HiiDb
  )
{
  EFI_STATUS                Status;
  EFI_DEVICE_PATH_PROTOCOL  *DevPath1;

  if ((HiiHandle == NULL) || (HiiDb == NULL)) {
    return (EFI_INVALID_PARAMETER);
  }

  *HiiHandle = NULL;

  if (Handle == NULL) {
    return (EFI_SUCCESS);
  }

  DevPath1 = NULL;
  Status   = gBS->OpenProtocol (Handle, &gEfiDevicePathProtocolGuid, (VOID **)&DevPath1, gImageHandle, NULL, EFI_OPEN_PROTOCOL_GET_PROTOCOL);
  if (EFI_ERROR (Status) || (DevPath1 == NULL)) {
    return (EFI_NOT_FOUND);
  }

  return (FindHiiHandleViaDevPath (DevPath1, HiiHandle, HiiDb));
}

/**
  Function to print out all HII configuration information to a file.

  @param[in] Handle           The handle to get info on.  NULL to do all handles.
  @param[in] FileName         The filename to rwite the info to.
**/
SHELL_STATUS
ConfigToFile (
  IN CONST EFI_HANDLE  Handle,
  IN CONST CHAR16      *FileName
  )
{
  EFI_HII_DATABASE_PROTOCOL  *HiiDatabase;
  EFI_STATUS                 Status;
  VOID                       *MainBuffer;
  UINTN                      MainBufferSize;
  EFI_HII_HANDLE             HiiHandle;
  SHELL_FILE_HANDLE          FileHandle;

  HiiDatabase    = NULL;
  MainBufferSize = 0;
  MainBuffer     = NULL;
  FileHandle     = NULL;

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ|EFI_FILE_MODE_WRITE|EFI_FILE_MODE_CREATE, 0);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL),
      gShellDriver1HiiHandle,
      L"drvcfg",
      FileName,
      Status
      );
    return (SHELL_DEVICE_ERROR);
  }

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );

  if (EFI_ERROR (Status) || (HiiDatabase == NULL)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_PROTOCOL_NF),
      gShellDriver1HiiHandle,
      L"drvcfg",
      L"EfiHiiDatabaseProtocol",
      &gEfiHiiDatabaseProtocolGuid
      );
    ShellCloseFile (&FileHandle);
    return (SHELL_NOT_FOUND);
  }

  HiiHandle = NULL;
  Status    = ConvertHandleToHiiHandle (Handle, &HiiHandle, HiiDatabase);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_HANDLE_NOT),
      gShellDriver1HiiHandle,
      L"drvcfg",
      ConvertHandleToHandleIndex (Handle),
      L"Device"
      );
    ShellCloseFile (&FileHandle);
    return (SHELL_DEVICE_ERROR);
  }

  Status = HiiDatabase->ExportPackageLists (HiiDatabase, HiiHandle, &MainBufferSize, MainBuffer);
  if (Status == EFI_BUFFER_TOO_SMALL) {
    MainBuffer = AllocateZeroPool (MainBufferSize);
    Status     = HiiDatabase->ExportPackageLists (HiiDatabase, HiiHandle, &MainBufferSize, MainBuffer);
  }

  Status = ShellWriteFile (FileHandle, &MainBufferSize, MainBuffer);

  ShellCloseFile (&FileHandle);
  SHELL_FREE_NON_NULL (MainBuffer);

  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_FILE_WRITE_FAIL),
      gShellDriver1HiiHandle,
      L"drvcfg",
      FileName
      );
    return (SHELL_DEVICE_ERROR);
  }

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_DRVCFG_COMP),
    gShellDriver1HiiHandle
    );

  return (SHELL_SUCCESS);
}

/**
  Function to read in HII configuration information from a file.

  @param[in] Handle           The handle to get info for.
  @param[in] FileName         The filename to read the info from.
**/
SHELL_STATUS
ConfigFromFile (
  IN       EFI_HANDLE  Handle,
  IN CONST CHAR16      *FileName
  )
{
  EFI_HII_DATABASE_PROTOCOL    *HiiDatabase;
  EFI_STATUS                   Status;
  VOID                         *MainBuffer;
  UINT64                       Temp;
  UINTN                        MainBufferSize;
  EFI_HII_HANDLE               HiiHandle;
  SHELL_FILE_HANDLE            FileHandle;
  CHAR16                       *TempDevPathString;
  EFI_HII_PACKAGE_LIST_HEADER  *PackageListHeader;
  EFI_HII_PACKAGE_HEADER       *PackageHeader;
  EFI_DEVICE_PATH_PROTOCOL     *DevPath;
  UINTN                        HandleIndex;

  HiiDatabase    = NULL;
  MainBufferSize = 0;
  MainBuffer     = NULL;
  FileHandle     = NULL;

  Status = ShellOpenFileByName (FileName, &FileHandle, EFI_FILE_MODE_READ, 0);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_FILE_OPEN_FAIL),
      gShellDriver1HiiHandle,
      L"drvcfg",
      FileName,
      Status
      );
    return (SHELL_DEVICE_ERROR);
  }

  //
  // Locate HII Database protocol
  //
  Status = gBS->LocateProtocol (
                  &gEfiHiiDatabaseProtocolGuid,
                  NULL,
                  (VOID **)&HiiDatabase
                  );

  if (EFI_ERROR (Status) || (HiiDatabase == NULL)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_PROTOCOL_NF),
      gShellDriver1HiiHandle,
      L"drvcfg",
      L"EfiHiiDatabaseProtocol",
      &gEfiHiiDatabaseProtocolGuid
      );
    ShellCloseFile (&FileHandle);
    return (SHELL_NOT_FOUND);
  }

  Status         = ShellGetFileSize (FileHandle, &Temp);
  MainBufferSize = (UINTN)Temp;
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_FILE_READ_FAIL),
      gShellDriver1HiiHandle,
      L"drvcfg",
      FileName
      );

    ShellCloseFile (&FileHandle);
    return (SHELL_DEVICE_ERROR);
  }

  MainBuffer = AllocateZeroPool ((UINTN)MainBufferSize);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_GEN_OUT_MEM),
      gShellDriver1HiiHandle,
      L"drvcfg"
      );
    ShellCloseFile (&FileHandle);
    return (SHELL_DEVICE_ERROR);
  }

  Status = ShellReadFile (FileHandle, &MainBufferSize, MainBuffer);
  if (EFI_ERROR (Status)) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_FILE_READ_FAIL),
      gShellDriver1HiiHandle,
      L"drvcfg",
      FileName
      );

    ShellCloseFile (&FileHandle);
    SHELL_FREE_NON_NULL (MainBuffer);
    return (SHELL_DEVICE_ERROR);
  }

  ShellCloseFile (&FileHandle);

  if (Handle != NULL) {
    //
    // User override in place.  Just do it.
    //
    HiiHandle = NULL;
    Status    = ConvertHandleToHiiHandle (Handle, &HiiHandle, HiiDatabase);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_HANDLE_NOT),
        gShellDriver1HiiHandle,
        L"drvcfg",
        ConvertHandleToHandleIndex (Handle),
        L"Device"
        );
      ShellCloseFile (&FileHandle);
      return (SHELL_DEVICE_ERROR);
    }

    Status = HiiDatabase->UpdatePackageList (HiiDatabase, HiiHandle, MainBuffer);
    if (EFI_ERROR (Status)) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_UEFI_FUNC_WARN),
        gShellDriver1HiiHandle,
        L"drvcfg",
        L"HiiDatabase->UpdatePackageList",
        Status
        );
      return (SHELL_DEVICE_ERROR);
    }
  } else {
    //
    // we need to parse the buffer and try to match the device paths for each item to try to find it's device path.
    //

    for (PackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *)MainBuffer
         ; PackageListHeader != NULL && ((CHAR8 *)PackageListHeader) < (((CHAR8 *)MainBuffer)+MainBufferSize)
         ; PackageListHeader = (EFI_HII_PACKAGE_LIST_HEADER *)(((CHAR8 *)(PackageListHeader)) + PackageListHeader->PackageLength))
    {
      for (PackageHeader = (EFI_HII_PACKAGE_HEADER *)(((CHAR8 *)(PackageListHeader))+sizeof (EFI_HII_PACKAGE_LIST_HEADER))
           ; PackageHeader != NULL && ((CHAR8 *)PackageHeader) < (((CHAR8 *)MainBuffer)+MainBufferSize) && PackageHeader->Type != EFI_HII_PACKAGE_END
           ; PackageHeader = (EFI_HII_PACKAGE_HEADER *)(((CHAR8 *)(PackageHeader))+PackageHeader->Length))
      {
        if (PackageHeader->Type == EFI_HII_PACKAGE_DEVICE_PATH) {
          HiiHandle = NULL;
          Status    = FindHiiHandleViaDevPath ((EFI_DEVICE_PATH_PROTOCOL *)(((CHAR8 *)PackageHeader) + sizeof (EFI_HII_PACKAGE_HEADER)), &HiiHandle, HiiDatabase);
          if (EFI_ERROR (Status)) {
            //
            // print out an error.
            //
            TempDevPathString = ConvertDevicePathToText ((EFI_DEVICE_PATH_PROTOCOL *)(((CHAR8 *)PackageHeader) + sizeof (EFI_HII_PACKAGE_HEADER)), TRUE, TRUE);
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_IN_FILE_NF),
              gShellDriver1HiiHandle,
              TempDevPathString
              );
            SHELL_FREE_NON_NULL (TempDevPathString);
          } else {
            Status = HiiDatabase->UpdatePackageList (HiiDatabase, HiiHandle, PackageListHeader);
            if (EFI_ERROR (Status)) {
              ShellPrintHiiEx (
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_GEN_UEFI_FUNC_WARN),
                gShellDriver1HiiHandle,
                L"drvcfg",
                L"HiiDatabase->UpdatePackageList",
                Status
                );
              return (SHELL_DEVICE_ERROR);
            } else {
              DevPath = (EFI_DEVICE_PATH_PROTOCOL *)(((CHAR8 *)PackageHeader) + sizeof (EFI_HII_PACKAGE_HEADER));
              gBS->LocateDevicePath (&gEfiHiiConfigAccessProtocolGuid, &DevPath, &Handle);
              HandleIndex = ConvertHandleToHandleIndex (Handle);
              ShellPrintHiiEx (
                -1,
                -1,
                NULL,
                STRING_TOKEN (STR_DRVCFG_DONE_HII),
                gShellDriver1HiiHandle,
                HandleIndex
                );
            }
          }
        }
      }
    }
  }

  SHELL_FREE_NON_NULL (MainBuffer);

  ShellPrintHiiEx (
    -1,
    -1,
    NULL,
    STRING_TOKEN (STR_DRVCFG_COMP),
    gShellDriver1HiiHandle
    );
  return (SHELL_SUCCESS);
}

/**
  Present a requested action to the user.

  @param[in] DriverImageHandle  The handle for the driver to configure.
  @param[in] ControllerHandle   The handle of the device being managed by the Driver specified.
  @param[in] ChildHandle        The handle of a child device of the specified device.
  @param[in] ActionRequired     The required HII action.

  @retval SHELL_INVALID_PARAMETER   A parameter has a invalid value.
**/
EFI_STATUS
ShellCmdDriverConfigurationProcessActionRequired (
  EFI_HANDLE                                DriverImageHandle,
  EFI_HANDLE                                ControllerHandle,
  EFI_HANDLE                                ChildHandle,
  EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED  ActionRequired
  )
{
  EFI_HANDLE  ConnectControllerContextOverride[2];

  switch (ActionRequired) {
    case EfiDriverConfigurationActionNone:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_NONE), gShellDriver1HiiHandle);
      break;

    case EfiDriverConfigurationActionStopController:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_STOP), gShellDriver1HiiHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_ENTER_S), gShellDriver1HiiHandle, L"stop controller");
      ShellPromptForResponse (ShellPromptResponseTypeEnterContinue, NULL, NULL);

      gBS->DisconnectController (ControllerHandle, DriverImageHandle, ChildHandle);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_CTLR_S), gShellDriver1HiiHandle, L"stopped");
      break;

    case EfiDriverConfigurationActionRestartController:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_RESTART_S), gShellDriver1HiiHandle, L"controller");
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_ENTER_S), gShellDriver1HiiHandle, L"restart controller");
      ShellPromptForResponse (ShellPromptResponseTypeEnterContinue, NULL, NULL);

      gBS->DisconnectController (ControllerHandle, DriverImageHandle, ChildHandle);
      ConnectControllerContextOverride[0] = DriverImageHandle;
      ConnectControllerContextOverride[1] = NULL;
      gBS->ConnectController (ControllerHandle, ConnectControllerContextOverride, NULL, TRUE);
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_CTLR_S), gShellDriver1HiiHandle, L"restarted");
      break;

    case EfiDriverConfigurationActionRestartPlatform:
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_RESTART_S), gShellDriver1HiiHandle, L"platform");
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_ENTER_S), gShellDriver1HiiHandle, L"restart platform");
      ShellPromptForResponse (ShellPromptResponseTypeEnterContinue, NULL, NULL);

      gRT->ResetSystem (EfiResetCold, EFI_SUCCESS, 0, NULL);
      break;

    default:
      return (EFI_INVALID_PARAMETER);
  }

  return EFI_SUCCESS;
}

/**
  Do the configuration in an environment without HII.

  @param[in] Language           The language code.
  @param[in] ForceDefaults      TRUE to force defaults, FALSE otherwise.
  @param[in] DefaultType        If ForceDefaults is TRUE, specifies the default type.
  @param[in] AllChildren        TRUE to configure all children, FALSE otherwise.
  @param[in] ValidateOptions    TRUE to validate existing options, FALSE otherwise.
  @param[in] SetOptions         TRUE to set options, FALSE otherwise.
  @param[in] DriverImageHandle  The handle for the driver to configure.
  @param[in] DeviceHandle       The handle of the device being managed by the Driver specified.
  @param[in] ChildHandle        The handle of a child device of the specified device.

  @retval SHELL_NOT_FOUND           A specified handle could not be found.
  @retval SHELL_INVALID_PARAMETER   A parameter has a invalid value.
**/
SHELL_STATUS
PreHiiDrvCfg (
  IN CONST CHAR8  *Language,
  IN BOOLEAN      ForceDefaults,
  IN UINT32       DefaultType,
  IN BOOLEAN      AllChildren,
  IN BOOLEAN      ValidateOptions,
  IN BOOLEAN      SetOptions,
  IN EFI_HANDLE   DriverImageHandle,
  IN EFI_HANDLE   DeviceHandle,
  IN EFI_HANDLE   ChildHandle
  )
{
  EFI_STATUS                                Status;
  SHELL_STATUS                              ShellStatus;
  UINTN                                     OuterLoopCounter;
  CHAR8                                     *BestLanguage;
  UINTN                                     DriverImageHandleCount;
  EFI_HANDLE                                *DriverImageHandleBuffer;
  UINTN                                     HandleCount;
  EFI_HANDLE                                *HandleBuffer;
  UINTN                                     *HandleType;
  UINTN                                     LoopCounter;
  UINTN                                     ChildIndex;
  UINTN                                     ChildHandleCount;
  EFI_HANDLE                                *ChildHandleBuffer;
  UINTN                                     *ChildHandleType;
  EFI_DRIVER_CONFIGURATION_ACTION_REQUIRED  ActionRequired;
  EFI_DRIVER_CONFIGURATION_PROTOCOL         *DriverConfiguration;
  BOOLEAN                                   Iso639Language;
  UINTN                                     HandleIndex1;
  UINTN                                     HandleIndex2;
  UINTN                                     HandleIndex3;

  ShellStatus = SHELL_SUCCESS;

  if ((ChildHandle == NULL) && AllChildren) {
    SetOptions = FALSE;
  }

  if (ForceDefaults) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DRVCFG_FORCE_D),
      gShellDriver1HiiHandle,
      DefaultType
      );
  } else if (ValidateOptions) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DRVCFG_VALIDATE),
      gShellDriver1HiiHandle
      );
  } else if (SetOptions) {
    ShellPrintHiiEx (
      -1,
      -1,
      NULL,
      STRING_TOKEN (STR_DRVCFG_SET),
      gShellDriver1HiiHandle
      );
  }

  if (DriverImageHandle == 0) {
    DriverImageHandleBuffer = GetHandleListByProtocolList (CfgGuidList);
    if (DriverImageHandleBuffer == NULL) {
      ShellStatus = SHELL_NOT_FOUND;
      goto Done;
    }

    for (
         HandleBuffer = DriverImageHandleBuffer, DriverImageHandleCount = 0
         ; HandleBuffer != NULL && *HandleBuffer != NULL
         ; HandleBuffer++, DriverImageHandleCount++)
    {
    }
  } else {
    DriverImageHandleCount = 1;
    //
    // Allocate buffer to hold the image handle so as to
    // keep consistent with the above clause
    //
    DriverImageHandleBuffer = AllocatePool (sizeof (EFI_HANDLE));
    ASSERT (DriverImageHandleBuffer);
    DriverImageHandleBuffer[0] = DriverImageHandle;
  }

  for (OuterLoopCounter = 0; OuterLoopCounter < DriverImageHandleCount; OuterLoopCounter++) {
    Iso639Language = FALSE;
    Status         = gBS->OpenProtocol (
                            DriverImageHandleBuffer[OuterLoopCounter],
                            &gEfiDriverConfiguration2ProtocolGuid,
                            (VOID **)&DriverConfiguration,
                            NULL,
                            NULL,
                            EFI_OPEN_PROTOCOL_GET_PROTOCOL
                            );
    if (EFI_ERROR (Status)) {
      Iso639Language = TRUE;
      Status         = gBS->OpenProtocol (
                              DriverImageHandleBuffer[OuterLoopCounter],
                              &gEfiDriverConfigurationProtocolGuid,
                              (VOID **)&DriverConfiguration,
                              NULL,
                              NULL,
                              EFI_OPEN_PROTOCOL_GET_PROTOCOL
                              );
    }

    if (EFI_ERROR (Status)) {
      //      ShellPrintHiiEx(
      //        -1,
      //        -1,
      //        NULL,
      //        STRING_TOKEN (STR_DRVCFG_NOT_SUPPORT),
      //        gShellDriver1HiiHandle,
      //        ConvertHandleToHandleIndex (DriverImageHandleBuffer[OuterLoopCounter])
      //        );
      ShellStatus = SHELL_UNSUPPORTED;
      continue;
    }

    BestLanguage = GetBestLanguage (
                     DriverConfiguration->SupportedLanguages,
                     Iso639Language,
                     Language != NULL ? Language : "",
                     DriverConfiguration->SupportedLanguages,
                     NULL
                     );
    if (BestLanguage == NULL) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_GEN_NO_VALUE),
        gShellDriver1HiiHandle,
        L"drvcfg",
        L"-l"
        );
      ShellStatus = SHELL_INVALID_PARAMETER;
      continue;
    }

    Status = ParseHandleDatabaseByRelationshipWithType (
               DriverImageHandleBuffer[OuterLoopCounter],
               NULL,
               &HandleCount,
               &HandleBuffer,
               &HandleType
               );
    if (EFI_ERROR (Status)) {
      continue;
    }

    if (SetOptions && (DeviceHandle == NULL)) {
      gST->ConOut->ClearScreen (gST->ConOut);
      Status = DriverConfiguration->SetOptions (
                                      DriverConfiguration,
                                      NULL,
                                      NULL,
                                      BestLanguage,
                                      &ActionRequired
                                      );
      gST->ConOut->ClearScreen (gST->ConOut);

      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DRVCFG_ALL_LANG),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex (DriverImageHandleBuffer[OuterLoopCounter]),
        DriverConfiguration->SupportedLanguages
        );
      if (!EFI_ERROR (Status)) {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DRVCFG_OPTIONS_SET),
          gShellDriver1HiiHandle
          );
        for (LoopCounter = 0; LoopCounter < HandleCount; LoopCounter++) {
          if ((HandleType[LoopCounter] & HR_CONTROLLER_HANDLE) == HR_CONTROLLER_HANDLE) {
            ShellCmdDriverConfigurationProcessActionRequired (
              DriverImageHandleBuffer[OuterLoopCounter],
              HandleBuffer[LoopCounter],
              NULL,
              ActionRequired
              );
          }
        }
      } else {
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DRVCFG_NOT_SET),
          gShellDriver1HiiHandle,
          Status
          );
      }

      continue;
    }

    for (LoopCounter = 0; LoopCounter < HandleCount; LoopCounter++) {
      if ((HandleType[LoopCounter] & HR_CONTROLLER_HANDLE) != HR_CONTROLLER_HANDLE) {
        continue;
      }

      if ((DeviceHandle != NULL) && (DeviceHandle != HandleBuffer[LoopCounter])) {
        continue;
      }

      if (ChildHandle == NULL) {
        HandleIndex1 = ConvertHandleToHandleIndex (DriverImageHandleBuffer[OuterLoopCounter]);
        HandleIndex2 = ConvertHandleToHandleIndex (HandleBuffer[LoopCounter]);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DRVCFG_CTRL_LANG),
          gShellDriver1HiiHandle,
          HandleIndex1,
          HandleIndex2,
          DriverConfiguration->SupportedLanguages
          );

        if (ForceDefaults) {
          Status = DriverConfiguration->ForceDefaults (
                                          DriverConfiguration,
                                          HandleBuffer[LoopCounter],
                                          NULL,
                                          DefaultType,
                                          &ActionRequired
                                          );

          if (!EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_DEF_FORCED),
              gShellDriver1HiiHandle
              );
            ShellCmdDriverConfigurationProcessActionRequired (
              DriverImageHandleBuffer[OuterLoopCounter],
              HandleBuffer[LoopCounter],
              NULL,
              ActionRequired
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_FORCE_FAILED),
              gShellDriver1HiiHandle,
              Status
              );
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        } else if (ValidateOptions) {
          Status = DriverConfiguration->OptionsValid (
                                          DriverConfiguration,
                                          HandleBuffer[LoopCounter],
                                          NULL
                                          );

          if (!EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_OPTIONS_VALID),
              gShellDriver1HiiHandle
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_OPTIONS_INV),
              gShellDriver1HiiHandle,
              Status
              );
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        } else if (SetOptions) {
          gST->ConOut->ClearScreen (gST->ConOut);
          Status = DriverConfiguration->SetOptions (
                                          DriverConfiguration,
                                          HandleBuffer[LoopCounter],
                                          NULL,
                                          BestLanguage,
                                          &ActionRequired
                                          );
          gST->ConOut->ClearScreen (gST->ConOut);
          HandleIndex1 = ConvertHandleToHandleIndex (DriverImageHandleBuffer[OuterLoopCounter]);
          HandleIndex2 = ConvertHandleToHandleIndex (HandleBuffer[LoopCounter]);
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DRVCFG_CTRL_LANG),
            gShellDriver1HiiHandle,
            HandleIndex1,
            HandleIndex2,
            DriverConfiguration->SupportedLanguages
            );
          if (!EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_OPTIONS_SET),
              gShellDriver1HiiHandle
              );

            ShellCmdDriverConfigurationProcessActionRequired (
              DriverImageHandleBuffer[OuterLoopCounter],
              HandleBuffer[LoopCounter],
              NULL,
              ActionRequired
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_NOT_SET),
              gShellDriver1HiiHandle,
              Status
              );
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        } else {
          Print (L"\n");
        }
      }

      if ((ChildHandle == NULL) && !AllChildren) {
        continue;
      }

      Status = ParseHandleDatabaseByRelationshipWithType (
                 DriverImageHandleBuffer[OuterLoopCounter],
                 HandleBuffer[LoopCounter],
                 &ChildHandleCount,
                 &ChildHandleBuffer,
                 &ChildHandleType
                 );
      if (EFI_ERROR (Status)) {
        continue;
      }

      for (ChildIndex = 0; ChildIndex < ChildHandleCount; ChildIndex++) {
        if ((ChildHandleType[ChildIndex] & HR_CHILD_HANDLE) != HR_CHILD_HANDLE) {
          continue;
        }

        if ((ChildHandle != NULL) && (ChildHandle != ChildHandleBuffer[ChildIndex])) {
          continue;
        }

        HandleIndex1 = ConvertHandleToHandleIndex (DriverImageHandleBuffer[OuterLoopCounter]);
        HandleIndex2 = ConvertHandleToHandleIndex (HandleBuffer[LoopCounter]);
        HandleIndex3 = ConvertHandleToHandleIndex (ChildHandleBuffer[ChildIndex]);
        ShellPrintHiiEx (
          -1,
          -1,
          NULL,
          STRING_TOKEN (STR_DRVCFG_CHILD_LANG),
          gShellDriver1HiiHandle,
          HandleIndex1,
          HandleIndex2,
          HandleIndex3,
          DriverConfiguration->SupportedLanguages
          );

        if (ForceDefaults) {
          Status = DriverConfiguration->ForceDefaults (
                                          DriverConfiguration,
                                          HandleBuffer[LoopCounter],
                                          ChildHandleBuffer[ChildIndex],
                                          DefaultType,
                                          &ActionRequired
                                          );

          if (!EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_DEF_FORCED),
              gShellDriver1HiiHandle
              );

            ShellCmdDriverConfigurationProcessActionRequired (
              DriverImageHandleBuffer[OuterLoopCounter],
              HandleBuffer[LoopCounter],
              ChildHandleBuffer[ChildIndex],
              ActionRequired
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_FORCE_FAILED),
              gShellDriver1HiiHandle,
              Status
              );
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        } else if (ValidateOptions) {
          Status = DriverConfiguration->OptionsValid (
                                          DriverConfiguration,
                                          HandleBuffer[LoopCounter],
                                          ChildHandleBuffer[ChildIndex]
                                          );

          if (!EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_OPTIONS_VALID),
              gShellDriver1HiiHandle
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_OPTIONS_INV),
              gShellDriver1HiiHandle,
              Status
              );
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        } else if (SetOptions) {
          gST->ConOut->ClearScreen (gST->ConOut);
          Status = DriverConfiguration->SetOptions (
                                          DriverConfiguration,
                                          HandleBuffer[LoopCounter],
                                          ChildHandleBuffer[ChildIndex],
                                          BestLanguage,
                                          &ActionRequired
                                          );
          gST->ConOut->ClearScreen (gST->ConOut);
          HandleIndex1 = ConvertHandleToHandleIndex (DriverImageHandleBuffer[OuterLoopCounter]);
          HandleIndex2 = ConvertHandleToHandleIndex (HandleBuffer[LoopCounter]);
          HandleIndex3 = ConvertHandleToHandleIndex (ChildHandleBuffer[ChildIndex]);
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DRVCFG_CHILD_LANG),
            gShellDriver1HiiHandle,
            HandleIndex1,
            HandleIndex2,
            HandleIndex3,
            DriverConfiguration->SupportedLanguages
            );
          if (!EFI_ERROR (Status)) {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_OPTIONS_SET),
              gShellDriver1HiiHandle
              );

            ShellCmdDriverConfigurationProcessActionRequired (
              DriverImageHandleBuffer[OuterLoopCounter],
              HandleBuffer[LoopCounter],
              ChildHandleBuffer[ChildIndex],
              ActionRequired
              );
          } else {
            ShellPrintHiiEx (
              -1,
              -1,
              NULL,
              STRING_TOKEN (STR_DRVCFG_NOT_SET),
              gShellDriver1HiiHandle,
              Status
              );
            ShellStatus = SHELL_DEVICE_ERROR;
          }
        } else {
          Print (L"\n");
        }
      }

      FreePool (ChildHandleBuffer);
      FreePool (ChildHandleType);
    }

    FreePool (BestLanguage);
    FreePool (HandleBuffer);
    FreePool (HandleType);
  }

  if ((DriverImageHandle != NULL) && (DriverImageHandleCount != 0)) {
    FreePool (DriverImageHandleBuffer);
  }

Done:
  return ShellStatus;
}

/**
  Function to print out configuration information on all configurable handles.

  @param[in] ChildrenToo    TRUE to tewst for children.
  @param[in] Language       ASCII string for language code.
  @param[in] UseHii         TRUE to check for Hii and DPC, FALSE for DCP only.

  @retval SHELL_SUCCESS     The operation was successful.
**/
SHELL_STATUS
PrintConfigInfoOnAll (
  IN CONST BOOLEAN  ChildrenToo,
  IN CONST CHAR8    *Language,
  IN CONST BOOLEAN  UseHii
  )
{
  EFI_HANDLE  *HandleList;
  EFI_HANDLE  *CurrentHandle;
  BOOLEAN     Found;
  UINTN       Index2;

  Found         = FALSE;
  HandleList    = NULL;
  CurrentHandle = NULL;

  if (UseHii) {
    //
    // HII method
    //
    HandleList = GetHandleListByProtocol (&gEfiHiiConfigAccessProtocolGuid);
    for (CurrentHandle = HandleList; CurrentHandle != NULL && *CurrentHandle != NULL; CurrentHandle++) {
      Found  = TRUE;
      Index2 = *CurrentHandle == NULL ? 0 : ConvertHandleToHandleIndex (*CurrentHandle);
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DRVCFG_LINE_HII),
        gShellDriver1HiiHandle,
        Index2
        );
    }

    SHELL_FREE_NON_NULL (HandleList);
  }

  if (PreHiiDrvCfg (
        Language,
        FALSE,
        0,
        ChildrenToo,
        FALSE,
        FALSE,
        0,
        0,
        0
        ) == SHELL_SUCCESS)
  {
    Found = TRUE;
  }

  if (!Found) {
    ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_DRVCFG_NONE_FOUND), gShellDriver1HiiHandle);
    return (SHELL_SUCCESS);
  }

  return (SHELL_SUCCESS);
}

STATIC CONST SHELL_PARAM_ITEM  ParamListHii[] = {
  { L"-s", TypeFlag  },
  { L"-l", TypeValue },
  { L"-f", TypeValue },
  { L"-o", TypeValue },
  { L"-i", TypeValue },
  { NULL,  TypeMax   }
};
STATIC CONST SHELL_PARAM_ITEM  ParamListPreHii[] = {
  { L"-c", TypeFlag  },
  { L"-s", TypeFlag  },
  { L"-v", TypeFlag  },
  { L"-l", TypeValue },
  { L"-f", TypeValue },
  { NULL,  TypeMax   }
};

/**
  Function for 'drvcfg' command.

  @param[in] ImageHandle  Handle to the Image (NULL if Internal).
  @param[in] SystemTable  Pointer to the System Table (NULL if Internal).
**/
SHELL_STATUS
EFIAPI
ShellCommandRunDrvCfg (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS    Status;
  LIST_ENTRY    *Package;
  CHAR16        *ProblemParam;
  SHELL_STATUS  ShellStatus;
  CHAR8         *Language;
  CONST CHAR16  *Lang;
  CONST CHAR16  *HandleIndex1;
  CONST CHAR16  *HandleIndex2;
  CONST CHAR16  *HandleIndex3;
  CONST CHAR16  *ForceTypeString;
  BOOLEAN       Force;
  BOOLEAN       Set;
  BOOLEAN       Validate;
  BOOLEAN       InFromFile;
  BOOLEAN       OutToFile;
  BOOLEAN       AllChildren;
  BOOLEAN       UseHii;
  UINT32        ForceType;
  UINT64        Intermediate;
  EFI_HANDLE    Handle1;
  EFI_HANDLE    Handle2;
  EFI_HANDLE    Handle3;
  CONST CHAR16  *FileName;

  ShellStatus  = SHELL_SUCCESS;
  Status       = EFI_SUCCESS;
  Language     = NULL;
  UseHii       = TRUE;
  ProblemParam = NULL;

  //
  // initialize the shell lib (we must be in non-auto-init...)
  //
  Status = ShellInitialize ();
  ASSERT_EFI_ERROR (Status);

  Status = CommandInit ();
  ASSERT_EFI_ERROR (Status);

  //
  // parse the command line
  //
  Status = ShellCommandLineParse (ParamListHii, &Package, &ProblemParam, TRUE);
  if (EFI_ERROR (Status) || (ShellCommandLineGetCount (Package) > 2)) {
    UseHii = FALSE;
    if (Package != NULL) {
      ShellCommandLineFreeVarList (Package);
    }

    SHELL_FREE_NON_NULL (ProblemParam);
    Status = ShellCommandLineParse (ParamListPreHii, &Package, &ProblemParam, TRUE);
    if (EFI_ERROR (Status)) {
      if ((Status == EFI_VOLUME_CORRUPTED) && (ProblemParam != NULL)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM), gShellDriver1HiiHandle, L"drvcfg", ProblemParam);
        FreePool (ProblemParam);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      } else {
        ASSERT (FALSE);
      }
    }
  }

  if (ShellStatus == SHELL_SUCCESS) {
    if (ShellCommandLineGetCount (Package) > 4) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_TOO_MANY), gShellDriver1HiiHandle, L"drvcfg");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    Lang = ShellCommandLineGetValue (Package, L"-l");
    if (Lang != NULL) {
      Language = AllocateZeroPool (StrSize (Lang));
      AsciiSPrint (Language, StrSize (Lang), "%S", Lang);
    } else if (ShellCommandLineGetFlag (Package, L"-l")) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"drvcfg", L"-l");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    Set             = ShellCommandLineGetFlag (Package, L"-s");
    Validate        = ShellCommandLineGetFlag (Package, L"-v");
    InFromFile      = ShellCommandLineGetFlag (Package, L"-i");
    OutToFile       = ShellCommandLineGetFlag (Package, L"-o");
    AllChildren     = ShellCommandLineGetFlag (Package, L"-c");
    Force           = ShellCommandLineGetFlag (Package, L"-f");
    ForceTypeString = ShellCommandLineGetValue (Package, L"-f");

    if (OutToFile) {
      FileName = ShellCommandLineGetValue (Package, L"-o");
    } else if (InFromFile) {
      FileName = ShellCommandLineGetValue (Package, L"-i");
    } else {
      FileName = NULL;
    }

    if (InFromFile && EFI_ERROR (ShellFileExists (FileName))) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FIND_FAIL), gShellDriver1HiiHandle, L"drvcfg", FileName);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (OutToFile && !EFI_ERROR (ShellFileExists (FileName))) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_FILE_EXIST), gShellDriver1HiiHandle, L"drvcfg", FileName);
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (Force && (ForceTypeString == NULL)) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"drvcfg", L"-f");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (Force) {
      Status = ShellConvertStringToUint64 (ForceTypeString, &Intermediate, FALSE, FALSE);
      if (EFI_ERROR (Status)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PROBLEM_VAL), gShellDriver1HiiHandle, L"drvcfg", ForceTypeString, L"-f");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      ForceType = (UINT32)Intermediate;
    } else {
      ForceType = 0;
    }

    HandleIndex1 = ShellCommandLineGetRawValue (Package, 1);
    Handle1      = NULL;
    if ((HandleIndex1 != NULL) && !EFI_ERROR (ShellConvertStringToUint64 (HandleIndex1, &Intermediate, TRUE, FALSE))) {
      Handle1 = ConvertHandleIndexToHandle ((UINTN)Intermediate);
      if ((Handle1 == NULL) || ((UINT64)(UINTN)Intermediate != Intermediate)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"drvcfg", HandleIndex1);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    HandleIndex2 = ShellCommandLineGetRawValue (Package, 2);
    Handle2      = NULL;
    if ((HandleIndex2 != NULL) && !EFI_ERROR (ShellConvertStringToUint64 (HandleIndex2, &Intermediate, TRUE, FALSE))) {
      Handle2 = ConvertHandleIndexToHandle ((UINTN)Intermediate);
      if ((Handle2 == NULL) || ((UINT64)(UINTN)Intermediate != Intermediate)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"drvcfg", HandleIndex2);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    HandleIndex3 = ShellCommandLineGetRawValue (Package, 3);
    Handle3      = NULL;
    if ((HandleIndex3 != NULL) && !EFI_ERROR (ShellConvertStringToUint64 (HandleIndex3, &Intermediate, TRUE, FALSE))) {
      Handle3 = ConvertHandleIndexToHandle ((UINTN)Intermediate);
      if ((Handle3 == NULL) || ((UINT64)(UINTN)Intermediate != Intermediate)) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_INV_HANDLE), gShellDriver1HiiHandle, L"drvcfg", HandleIndex3);
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    if ((InFromFile || OutToFile) && (FileName == NULL)) {
      if (FileName == NULL) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_NO_VALUE), gShellDriver1HiiHandle, L"drvcfg", InFromFile ? L"-i" : L"-o");
      } else {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_HANDLE_REQ), gShellDriver1HiiHandle, L"drvcfg");
      }

      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (!UseHii && (InFromFile || OutToFile)) {
      if (InFromFile) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDriver1HiiHandle, L"drvcfg", L"-i");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }

      if (OutToFile) {
        ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_INV), gShellDriver1HiiHandle, L"drvcfg", L"-o");
        ShellStatus = SHELL_INVALID_PARAMETER;
        goto Done;
      }
    }

    if (Validate && Force) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDriver1HiiHandle, L"drvcfg", L"-v", L"-f");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (Validate && Set) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDriver1HiiHandle, L"drvcfg", L"-v", L"-s");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (Set && Force) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDriver1HiiHandle, L"drvcfg", L"-s", L"-f");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    if (OutToFile && InFromFile) {
      ShellPrintHiiEx (-1, -1, NULL, STRING_TOKEN (STR_GEN_PARAM_CONFLICT), gShellDriver1HiiHandle, L"drvcfg", L"-i", L"-o");
      ShellStatus = SHELL_INVALID_PARAMETER;
      goto Done;
    }

    //
    // We do HII first.
    //
    if (UseHii) {
      if ((Handle1 != NULL) && EFI_ERROR (gBS->OpenProtocol (Handle1, &gEfiHiiConfigAccessProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
        //
        // no HII on this handle.
        //
        ShellStatus = SHELL_UNSUPPORTED;
      } else if (Validate) {
      } else if (Force) {
      } else if (Set) {
      } else if (InFromFile) {
        ShellStatus = ConfigFromFile (Handle1, FileName);
        if ((Handle1 != NULL) && (ShellStatus == SHELL_SUCCESS)) {
          goto Done;
        }
      } else if (OutToFile) {
        ShellStatus = ConfigToFile (Handle1, FileName);
        if ((Handle1 != NULL) && (ShellStatus == SHELL_SUCCESS)) {
          goto Done;
        }
      } else if (HandleIndex1 == NULL) {
        //
        // display all that are configurable
        //
        ShellStatus = PrintConfigInfoOnAll (AllChildren, Language, UseHii);
        goto Done;
      } else {
        if (!EFI_ERROR (gBS->OpenProtocol (Handle1, &gEfiHiiConfigAccessProtocolGuid, NULL, gImageHandle, NULL, EFI_OPEN_PROTOCOL_TEST_PROTOCOL))) {
          ShellPrintHiiEx (
            -1,
            -1,
            NULL,
            STRING_TOKEN (STR_DRVCFG_LINE_HII),
            gShellDriver1HiiHandle,
            ConvertHandleToHandleIndex (Handle1)
            );
          goto Done;
        }
      }
    }

    //
    // We allways need to do this one since it does both by default.
    //
    if (!InFromFile && !OutToFile) {
      ShellStatus = PreHiiDrvCfg (
                      Language,
                      Force,
                      ForceType,
                      AllChildren,
                      Validate,
                      Set,
                      Handle1,
                      Handle2,
                      Handle3
                      );
    }

    if (ShellStatus == SHELL_UNSUPPORTED) {
      ShellPrintHiiEx (
        -1,
        -1,
        NULL,
        STRING_TOKEN (STR_DRVCFG_NOT_SUPPORT),
        gShellDriver1HiiHandle,
        ConvertHandleToHandleIndex (Handle1)
        );
    }
  }

Done:
  ShellCommandLineFreeVarList (Package);
  SHELL_FREE_NON_NULL (Language);
  return (ShellStatus);
}
