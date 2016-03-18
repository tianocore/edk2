/** @file
This file include all platform action which can be customized
by IBV/OEM.

Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PlatformBootManager.h"

EFI_GUID mUefiShellFileGuid = {0x7C04A583, 0x9E3E, 0x4f1c, {0xAD, 0x65, 0xE0, 0x52, 0x68, 0xD0, 0xB4, 0xD1 }};

/**
  Return the index of the load option in the load option array.

  The function consider two load options are equal when the
  OptionType, Attributes, Description, FilePath and OptionalData are equal.

  @param Key    Pointer to the load option to be found.
  @param Array  Pointer to the array of load options to be found.
  @param Count  Number of entries in the Array.

  @retval -1          Key wasn't found in the Array.
  @retval 0 ~ Count-1 The index of the Key in the Array.
**/
INTN
PlatformFindLoadOption (
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Key,
  IN CONST EFI_BOOT_MANAGER_LOAD_OPTION *Array,
  IN UINTN                              Count
  )
{
  UINTN                             Index;

  for (Index = 0; Index < Count; Index++) {
    if ((Key->OptionType == Array[Index].OptionType) &&
        (Key->Attributes == Array[Index].Attributes) &&
        (StrCmp (Key->Description, Array[Index].Description) == 0) &&
        (CompareMem (Key->FilePath, Array[Index].FilePath, GetDevicePathSize (Key->FilePath)) == 0) &&
        (Key->OptionalDataSize == Array[Index].OptionalDataSize) &&
        (CompareMem (Key->OptionalData, Array[Index].OptionalData, Key->OptionalDataSize) == 0)) {
      return (INTN) Index;
    }
  }

  return -1;
}

VOID
PlatformRegisterFvBootOption (
  EFI_GUID  *FileGuid,
  CHAR16    *Description,
  UINT32    Attributes
  )
{
  EFI_STATUS                        Status;
  EFI_HANDLE                        *HandleBuffer;
  UINTN                             HandleCount;
  UINTN                             IndexFv;
  EFI_FIRMWARE_VOLUME2_PROTOCOL     *Fv;
  CHAR16                            *UiSection;
  UINTN                             UiSectionLength;
  UINT32                            AuthenticationStatus;
  EFI_HANDLE                        FvHandle;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FileNode;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;
  EFI_BOOT_MANAGER_LOAD_OPTION      *BootOptions;
  UINTN                             BootOptionCount;
  UINTN                             OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION      NewOption;

  //
  // Locate all available FVs.
  //
  HandleBuffer = NULL;
  Status = gBS->LocateHandleBuffer (
                  ByProtocol,
                  &gEfiFirmwareVolume2ProtocolGuid,
                  NULL,
                  &HandleCount,
                  &HandleBuffer
                  );
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Go through FVs one by one to find the required FFS file
  //
  for (IndexFv = 0, FvHandle = NULL; IndexFv < HandleCount && FvHandle == NULL; IndexFv++) {
    Status = gBS->HandleProtocol (
                    HandleBuffer[IndexFv],
                    &gEfiFirmwareVolume2ProtocolGuid,
                    (VOID **)&Fv
                    );
    if (EFI_ERROR (Status)) {
      continue;
    }

    //
    // Attempt to read a EFI_SECTION_USER_INTERFACE section from the required FFS file
    //
    UiSection = NULL;
    Status = Fv->ReadSection (
                   Fv,
                   FileGuid,
                   EFI_SECTION_USER_INTERFACE,
                   0,
                   (VOID **) &UiSection,
                   &UiSectionLength,
                   &AuthenticationStatus
                   );
    if (EFI_ERROR (Status)) {
      continue;
    }
    FreePool (UiSection);

    //
    // Save the handle of the FV where the FFS file was found
    //
    FvHandle = HandleBuffer[IndexFv];
  }

  //
  // Free the buffer of FV handles
  //
  FreePool (HandleBuffer);

  //
  // If the FFS file was not found, then return
  //
  if (FvHandle == NULL) {
    return;
  }

  //
  // Create a device path for the FFS file that was found
  //
  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = AppendDevicePathNode (
                 DevicePathFromHandle (FvHandle),
                 (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
                 );

  //
  // Create and add a new load option for the FFS file that was found
  //
  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             Attributes,
             Description,
             DevicePath,
             NULL,
             0
             );
  if (!EFI_ERROR (Status)) {
    BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

    OptionIndex = PlatformFindLoadOption (&NewOption, BootOptions, BootOptionCount);

    if (OptionIndex == -1) {
      Status = EfiBootManagerAddLoadOptionVariable (&NewOption, (UINTN) -1);
      ASSERT_EFI_ERROR (Status);
    }
    EfiBootManagerFreeLoadOption (&NewOption);
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
  }
}

VOID
EFIAPI
InternalBdsEmptyCallbackFuntion (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  return;
}

/**
  Do the platform specific action before the console is connected.

  Such as:
    Update console variable;
    Register new Driver#### or Boot####;
    Signal ReadyToLock event.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  EFI_STATUS                    Status;
  UINTN                         Index;
  EFI_INPUT_KEY                 Enter;
  EFI_INPUT_KEY                 F2;
  EFI_BOOT_MANAGER_LOAD_OPTION  BootOption;
  EFI_ACPI_S3_SAVE_PROTOCOL     *AcpiS3Save;
  EFI_HANDLE                    Handle;
  EFI_EVENT                     EndOfDxeEvent;

  //
  // Update the console variables.
  //
  for (Index = 0; gPlatformConsole[Index].DevicePath != NULL; Index++) {
    if ((gPlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
      EfiBootManagerUpdateConsoleVariable (ConIn, gPlatformConsole[Index].DevicePath, NULL);
    }

    if ((gPlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
      EfiBootManagerUpdateConsoleVariable (ConOut, gPlatformConsole[Index].DevicePath, NULL);
    }

    if ((gPlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
      EfiBootManagerUpdateConsoleVariable (ErrOut, gPlatformConsole[Index].DevicePath, NULL);
    }
  }

  //
  // Register ENTER as CONTINUE key
  //
  Enter.ScanCode    = SCAN_NULL;
  Enter.UnicodeChar = CHAR_CARRIAGE_RETURN;
  EfiBootManagerRegisterContinueKeyOption (0, &Enter, NULL);

  //
  // Map F2 to Boot Manager Menu
  //
  F2.ScanCode    = SCAN_F2;
  F2.UnicodeChar = CHAR_NULL;
  EfiBootManagerGetBootManagerMenu (&BootOption);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16) BootOption.OptionNumber, 0, &F2, NULL);

  //
  // Register UEFI Shell
  //
  PlatformRegisterFvBootOption (&mUefiShellFileGuid, L"UEFI Shell", LOAD_OPTION_ACTIVE);

  //
  // Prepare for S3
  //
  Status = gBS->LocateProtocol (&gEfiAcpiS3SaveProtocolGuid, NULL, (VOID **)&AcpiS3Save);
  if (!EFI_ERROR (Status)) {
    AcpiS3Save->S3Save (AcpiS3Save, NULL);
  }

  //
  // Inform PI SMM drivers that BDS may run 3rd party code
  // Create and signal End of DXE event group
  //
  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_CALLBACK,
                  InternalBdsEmptyCallbackFuntion,
                  NULL,
                  &gEfiEndOfDxeEventGroupGuid,
                  &EndOfDxeEvent
                  );
  ASSERT_EFI_ERROR (Status);
  gBS->SignalEvent (EndOfDxeEvent);
  gBS->CloseEvent (EndOfDxeEvent);

  DEBUG((EFI_D_INFO,"All EndOfDxe callbacks have returned successfully\n"));

  //
  // Install SMM Ready To Lock protocol so all resources can be locked down
  // before BDS runs 3rd party code.  This action must be done last so all
  // other SMM driver signals are processed before this final lock down action.
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEfiDxeSmmReadyToLockProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch additional option ROMs
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{
  EFI_STATUS  Status;

  Print (
    L"\n"
    L"F2      to enter Boot Manager Menu.\n"
    L"ENTER   to boot directly.\n"
    L"\n"
    );

  //
  // Use a DynamicHii type pcd to save the boot status, which is used to
  // control configuration mode, such as FULL/MINIMAL/NO_CHANGES configuration.
  //
  if (PcdGetBool(PcdBootState)) {
    Status = PcdSetBoolS (PcdBootState, FALSE);
    ASSERT_EFI_ERROR (Status);
  }
}

/**
  This function is called each second during the boot manager waits the timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16  TimeoutRemain
  )
{
  Print (L"\r%-2d seconds remained...", TimeoutRemain);
}
