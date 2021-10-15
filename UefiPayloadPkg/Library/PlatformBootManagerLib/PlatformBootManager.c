/** @file
  This file include all platform action which can be customized
  by IBV/OEM.

Copyright (c) 2015 - 2021, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBootManager.h"
#include "PlatformConsole.h"
#include <Protocol/PlatformBootManagerOverride.h>
#include <Guid/BootManagerMenu.h>
#include <Library/HobLib.h>

UNIVERSAL_PAYLOAD_PLATFORM_BOOT_MANAGER_OVERRIDE_PROTOCOL  *mUniversalPayloadPlatformBootManagerOverrideInstance = NULL;

/**
  Signal EndOfDxe event and install SMM Ready to lock protocol.

**/
VOID
InstallReadyToLock (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_HANDLE                            Handle;
  EFI_SMM_ACCESS2_PROTOCOL              *SmmAccess;

  DEBUG((DEBUG_INFO,"InstallReadyToLock  entering......\n"));
  //
  // Inform the SMM infrastructure that we're entering BDS and may run 3rd party code hereafter
  // Since PI1.2.1, we need signal EndOfDxe as ExitPmAuth
  //
  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);
  DEBUG((DEBUG_INFO,"All EndOfDxe callbacks have returned successfully\n"));

  //
  // Install DxeSmmReadyToLock protocol in order to lock SMM
  //
  Status = gBS->LocateProtocol (&gEfiSmmAccess2ProtocolGuid, NULL, (VOID **) &SmmAccess);
  if (!EFI_ERROR (Status)) {
    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gEfiDxeSmmReadyToLockProtocolGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
  }

  DEBUG((DEBUG_INFO,"InstallReadyToLock  end\n"));
  return;
}

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

/**
  Register a boot option using a file GUID in the FV.

  @param FileGuid     The file GUID name in FV.
  @param Description  The boot option description.
  @param Attributes   The attributes used for the boot option loading.
**/
VOID
PlatformRegisterFvBootOption (
  EFI_GUID                         *FileGuid,
  CHAR16                           *Description,
  UINT32                           Attributes
)
{
  EFI_STATUS                        Status;
  UINTN                             OptionIndex;
  EFI_BOOT_MANAGER_LOAD_OPTION      NewOption;
  EFI_BOOT_MANAGER_LOAD_OPTION      *BootOptions;
  UINTN                             BootOptionCount;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH FileNode;
  EFI_LOADED_IMAGE_PROTOCOL         *LoadedImage;
  EFI_DEVICE_PATH_PROTOCOL          *DevicePath;

  Status = gBS->HandleProtocol (gImageHandle, &gEfiLoadedImageProtocolGuid, (VOID **) &LoadedImage);
  ASSERT_EFI_ERROR (Status);

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);
  DevicePath = AppendDevicePathNode (
                 DevicePathFromHandle (LoadedImage->DeviceHandle),
                 (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
               );

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
  EFI_INPUT_KEY                Enter;
  EFI_INPUT_KEY                F2;
  EFI_INPUT_KEY                Down;
  EFI_BOOT_MANAGER_LOAD_OPTION BootOption;
  EFI_STATUS                   Status;

  Status = gBS->LocateProtocol (&gUniversalPayloadPlatformBootManagerOverrideProtocolGuid, NULL, (VOID **) &mUniversalPayloadPlatformBootManagerOverrideInstance);
  if (EFI_ERROR (Status)) {
    mUniversalPayloadPlatformBootManagerOverrideInstance = NULL;
  }
  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL){
    mUniversalPayloadPlatformBootManagerOverrideInstance->BeforeConsole();
    return;
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
  // Also add Down key to Boot Manager Menu since some serial terminals don't support F2 key.
  //
  Down.ScanCode    = SCAN_DOWN;
  Down.UnicodeChar = CHAR_NULL;
  EfiBootManagerGetBootManagerMenu (&BootOption);
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16) BootOption.OptionNumber, 0, &Down, NULL);

  //
  // Install ready to lock.
  // This needs to be done before option rom dispatched.
  //
  InstallReadyToLock ();

  //
  // Dispatch deferred images after EndOfDxe event and ReadyToLock installation.
  //
  EfiBootManagerDispatchDeferredImages ();

  PlatformConsoleInit ();
}

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch additional option roms.
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
)
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  White;

  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL){
    mUniversalPayloadPlatformBootManagerOverrideInstance->AfterConsole();
    return;
  }
  Black.Blue = Black.Green = Black.Red = Black.Reserved = 0;
  White.Blue = White.Green = White.Red = White.Reserved = 0xFF;

  EfiBootManagerConnectAll ();
  EfiBootManagerRefreshAllBootOption ();

  //
  // Register UEFI Shell
  //
  PlatformRegisterFvBootOption (PcdGetPtr (PcdShellFile), L"UEFI Shell", LOAD_OPTION_ACTIVE);

  Print (
    L"\n"
    L"F2 or Down      to enter Boot Manager Menu.\n"
    L"ENTER           to boot directly.\n"
    L"\n"
  );

}

/**
  This function is called each second during the boot manager waits the timeout.

  @param TimeoutRemain  The remaining timeout.
**/
VOID
EFIAPI
PlatformBootManagerWaitCallback (
  UINT16          TimeoutRemain
)
{
  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL){
    mUniversalPayloadPlatformBootManagerOverrideInstance->WaitCallback (TimeoutRemain);
  }
  return;
}

/**
  The function is called when no boot option could be launched,
  including platform recovery options and options pointing to applications
  built into firmware volumes.

  If this function returns, BDS attempts to enter an infinite loop.
**/
VOID
EFIAPI
PlatformBootManagerUnableToBoot (
  VOID
  )
{
  if (mUniversalPayloadPlatformBootManagerOverrideInstance != NULL){
    mUniversalPayloadPlatformBootManagerOverrideInstance->UnableToBoot();
  }
  return;
}

/**
  Get/update PcdBootManagerMenuFile from GUID HOB which will be assigned in bootloader.

  @param  ImageHandle   The firmware allocated handle for the EFI image.
  @param  SystemTable   A pointer to the EFI System Table.

  @retval EFI_SUCCESS       The entry point is executed successfully.
  @retval other             Some error occurs.

**/
EFI_STATUS
EFIAPI
PlatformBootManagerLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
)
{
  EFI_STATUS                           Status;
  UINTN                                Size;
  VOID                                 *GuidHob;
  UNIVERSAL_PAYLOAD_GENERIC_HEADER     *GenericHeader;
  UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU  *BootManagerMenuFile;

  GuidHob = GetFirstGuidHob (&gEdkiiBootManagerMenuFileGuid);

  if (GuidHob == NULL) {
    //
    // If the HOB is not create, the default value of PcdBootManagerMenuFile will be used.
    //
    return EFI_SUCCESS;
  }

  GenericHeader = (UNIVERSAL_PAYLOAD_GENERIC_HEADER *) GET_GUID_HOB_DATA (GuidHob);
  if ((sizeof (UNIVERSAL_PAYLOAD_GENERIC_HEADER) > GET_GUID_HOB_DATA_SIZE (GuidHob)) || (GenericHeader->Length > GET_GUID_HOB_DATA_SIZE (GuidHob))) {
    return EFI_NOT_FOUND;
  }
  if (GenericHeader->Revision == UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU_REVISION) {
    BootManagerMenuFile = (UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU *) GET_GUID_HOB_DATA (GuidHob);
    if (BootManagerMenuFile->Header.Length < UNIVERSAL_PAYLOAD_SIZEOF_THROUGH_FIELD (UNIVERSAL_PAYLOAD_BOOT_MANAGER_MENU, FileName)) {
      return EFI_NOT_FOUND;
    }
    Size = sizeof (BootManagerMenuFile->FileName);
    Status = PcdSetPtrS (PcdBootManagerMenuFile, &Size, &BootManagerMenuFile->FileName);
    ASSERT_EFI_ERROR (Status);
  } else {
    return EFI_NOT_FOUND;
  }

  return EFI_SUCCESS;
}
