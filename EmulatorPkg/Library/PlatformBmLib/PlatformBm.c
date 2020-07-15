/*++ @file

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Portions copyright (c) 2011, Apple Inc. All rights reserved.
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "PlatformBm.h"

EFI_GUID mBootMenuFile = {
  0xEEC25BDC, 0x67F2, 0x4D95, { 0xB1, 0xD5, 0xF8, 0x1B, 0x20, 0x39, 0xD1, 0x1D }
};

/**
  Initialize the "Setup" variable.
**/
VOID
SetupVariableInit (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINTN                           Size;
  EMU_SYSTEM_CONFIGURATION        SystemConfigData;

  Size = sizeof (SystemConfigData);
  Status = gRT->GetVariable (
                  L"Setup",
                  &gEmuSystemConfigGuid,
                  NULL,
                  &Size,
                  (VOID *) &SystemConfigData
                  );

  if (EFI_ERROR (Status)) {
    //
    // SetupVariable is corrupt
    //
    SystemConfigData.ConOutRow = PcdGet32 (PcdConOutColumn);
    SystemConfigData.ConOutColumn = PcdGet32 (PcdConOutRow);

    Status = gRT->SetVariable (
                    L"Setup",
                    &gEmuSystemConfigGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (SystemConfigData),
                    (VOID *) &SystemConfigData
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((DEBUG_ERROR, "Failed to save Setup Variable to non-volatile storage, Status = %r\n", Status));
    }
  }
}

EFI_DEVICE_PATH *
FvFilePath (
  EFI_GUID                     *FileGuid
  )
{

  EFI_STATUS                         Status;
  EFI_LOADED_IMAGE_PROTOCOL          *LoadedImage;
  MEDIA_FW_VOL_FILEPATH_DEVICE_PATH  FileNode;

  EfiInitializeFwVolDevicepathNode (&FileNode, FileGuid);

  Status = gBS->HandleProtocol (
                  gImageHandle,
                  &gEfiLoadedImageProtocolGuid,
                  (VOID **) &LoadedImage
                  );
  ASSERT_EFI_ERROR (Status);
  return AppendDevicePathNode (
           DevicePathFromHandle (LoadedImage->DeviceHandle),
           (EFI_DEVICE_PATH_PROTOCOL *) &FileNode
           );
}

/**
  Create one boot option for BootManagerMenuApp.

  @param  FileGuid          Input file guid for the BootManagerMenuApp.
  @param  Description       Description of the BootManagerMenuApp boot option.
  @param  Position          Position of the new load option to put in the ****Order variable.
  @param  IsBootCategory    Whether this is a boot category.


  @retval OptionNumber      Return the option number info.

**/
UINTN
RegisterBootManagerMenuAppBootOption (
  EFI_GUID                         *FileGuid,
  CHAR16                           *Description,
  UINTN                            Position,
  BOOLEAN                          IsBootCategory
  )
{
  EFI_STATUS                       Status;
  EFI_BOOT_MANAGER_LOAD_OPTION     NewOption;
  EFI_DEVICE_PATH_PROTOCOL         *DevicePath;
  UINTN                            OptionNumber;

  DevicePath = FvFilePath (FileGuid);
  Status = EfiBootManagerInitializeLoadOption (
             &NewOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypeBoot,
             IsBootCategory ? LOAD_OPTION_ACTIVE : LOAD_OPTION_CATEGORY_APP,
             Description,
             DevicePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);
  FreePool (DevicePath);

  Status = EfiBootManagerAddLoadOptionVariable (&NewOption, Position);
  ASSERT_EFI_ERROR (Status);

  OptionNumber = NewOption.OptionNumber;

  EfiBootManagerFreeLoadOption (&NewOption);

  return OptionNumber;
}

/**
  Check if it's a Device Path pointing to BootManagerMenuApp.

  @param  DevicePath     Input device path.

  @retval TRUE   The device path is BootManagerMenuApp File Device Path.
  @retval FALSE  The device path is NOT BootManagerMenuApp File Device Path.
**/
BOOLEAN
IsBootManagerMenuAppFilePath (
  EFI_DEVICE_PATH_PROTOCOL     *DevicePath
)
{
  EFI_HANDLE                      FvHandle;
  VOID                            *NameGuid;
  EFI_STATUS                      Status;

  Status = gBS->LocateDevicePath (&gEfiFirmwareVolume2ProtocolGuid, &DevicePath, &FvHandle);
  if (!EFI_ERROR (Status)) {
    NameGuid = EfiGetNameGuidFromFwVolDevicePathNode ((CONST MEDIA_FW_VOL_FILEPATH_DEVICE_PATH *) DevicePath);
    if (NameGuid != NULL) {
      return CompareGuid (NameGuid, &mBootMenuFile);
    }
  }

  return FALSE;
}

/**
  Return the boot option number to the BootManagerMenuApp.

  If not found it in the current boot option, create a new one.

  @retval OptionNumber   Return the boot option number to the BootManagerMenuApp.

**/
UINTN
GetBootManagerMenuAppOption (
  VOID
  )
{
  UINTN                        BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  UINTN                        Index;
  UINTN                        OptionNumber;

  OptionNumber = 0;

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  for (Index = 0; Index < BootOptionCount; Index++) {
    if (IsBootManagerMenuAppFilePath (BootOptions[Index].FilePath)) {
      OptionNumber = BootOptions[Index].OptionNumber;
      break;
    }
  }

  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);

  if (Index >= BootOptionCount) {
    //
    // If not found the BootManagerMenuApp, create it.
    //
    OptionNumber = (UINT16) RegisterBootManagerMenuAppBootOption (&mBootMenuFile, L"UEFI BootManagerMenuApp", (UINTN) -1, FALSE);
  }

  return OptionNumber;
}

/**
  Platform Bds init. Include the platform firmware vendor, revision
  and so crc check.
**/
VOID
EFIAPI
PlatformBootManagerBeforeConsole (
  VOID
  )
{
  UINTN       Index;

  SetupVariableInit ();

  EfiEventGroupSignal (&gEfiEndOfDxeEventGroupGuid);

  Index   = 0;
  while (gPlatformConsole[Index].DevicePath != NULL) {
    //
    // Update the console variable with the connect type
    //
    if ((gPlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
      EfiBootManagerUpdateConsoleVariable (ConIn, gPlatformConsole[Index].DevicePath, NULL);
    }

    if ((gPlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
      EfiBootManagerUpdateConsoleVariable (ConOut, gPlatformConsole[Index].DevicePath, NULL);
    }

    if ((gPlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
      EfiBootManagerUpdateConsoleVariable (ErrOut, gPlatformConsole[Index].DevicePath, NULL);
    }

    Index++;
  }
}

/**
  Connect with predefined platform connect sequence,
  the OEM/IBV can customize with their own connect sequence.
**/
VOID
PlatformBdsConnectSequence (
  VOID
  )
{
  //
  // Just use the simple policy to connect all devices
  //
  EfiBootManagerConnectAll ();
}

/**
  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.

  @param MemoryTestLevel The memory test intensive level
  @param QuietBoot       Indicate if need to enable the quiet boot
**/
VOID
PlatformBdsDiagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot
  )
{
  EFI_STATUS  Status;

  //
  // Here we can decide if we need to show
  // the diagnostics screen
  //
  if (QuietBoot) {
    BootLogoEnableLogo ();

    //
    // Perform system diagnostic
    //
    Status = PlatformBootManagerMemoryTest (MemoryTestLevel);
    if (EFI_ERROR (Status)) {
      BootLogoDisableLogo ();
    }

    return;
  }

  //
  // Perform system diagnostic
  //
  PlatformBootManagerMemoryTest (MemoryTestLevel);
}

/**
  Register the static boot options.
**/
VOID
PlatformBdsRegisterStaticBootOptions (
  VOID
  )
{
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL  White;
  EFI_INPUT_KEY                  Enter;
  EFI_INPUT_KEY                  F2;
  EFI_INPUT_KEY                  F7;
  EFI_BOOT_MANAGER_LOAD_OPTION   BootOption;
  UINTN                          OptionNumber;

  Black.Blue = Black.Green = Black.Red = Black.Reserved = 0;
  White.Blue = White.Green = White.Red = White.Reserved = 0xFF;

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
  // 3. Boot Device List menu
  //
  F7.ScanCode     = SCAN_F7;
  F7.UnicodeChar  = CHAR_NULL;
  OptionNumber    = GetBootManagerMenuAppOption ();
  EfiBootManagerAddKeyOptionVariable (NULL, (UINT16)OptionNumber, 0, &F7, NULL);

  PrintXY (10, 10, &White, &Black, L"F2    to enter Setup.                              ");
  PrintXY (10, 30, &White, &Black, L"F7    to enter Boot Manager Menu.");
  PrintXY (10, 50, &White, &Black, L"Enter to boot directly.");
}

/**
  Returns the priority number.

  @param BootOption
**/
UINTN
BootOptionPriority (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION *BootOption
  )
{
  //
  // Make sure Shell is first
  //
  if (StrCmp (BootOption->Description, L"UEFI Shell") == 0) {
    return 0;
  }
  return 100;
}

INTN
EFIAPI
CompareBootOption (
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Left,
  CONST EFI_BOOT_MANAGER_LOAD_OPTION  *Right
  )
{
  return BootOptionPriority (Left) - BootOptionPriority (Right);
}

/**
  Do the platform specific action after the console is connected.

  Such as:
    Dynamically switch output mode;
    Signal console ready platform customized event;
    Run diagnostics like memory testing;
    Connect certain devices;
    Dispatch aditional option roms.
**/
VOID
EFIAPI
PlatformBootManagerAfterConsole (
  VOID
  )
{

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  switch (GetBootModeHob ()) {

  case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
  case BOOT_WITH_MINIMAL_CONFIGURATION:
    PlatformBdsDiagnostics (IGNORE, TRUE);

    //
    // Perform some platform specific connect sequence
    //
    PlatformBdsConnectSequence ();
    break;

  case BOOT_IN_RECOVERY_MODE:
    PlatformBdsDiagnostics (EXTENSIVE, FALSE);
    break;

  case BOOT_WITH_FULL_CONFIGURATION:
  case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
  case BOOT_WITH_DEFAULT_SETTINGS:
  default:
    PlatformBdsDiagnostics (IGNORE, TRUE);
    PlatformBdsRegisterStaticBootOptions ();
    PlatformBdsConnectSequence ();
    EfiBootManagerRefreshAllBootOption ();
    EfiBootManagerSortLoadOptionVariable (LoadOptionTypeBoot, (SORT_COMPARE)CompareBootOption);
    break;
  }
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
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION Black;
  EFI_GRAPHICS_OUTPUT_BLT_PIXEL_UNION White;
  UINT16                              Timeout;

  Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  Black.Raw = 0x00000000;
  White.Raw = 0x00FFFFFF;

  BootLogoUpdateProgress (
    White.Pixel,
    Black.Pixel,
    L"Start boot option",
    White.Pixel,
    (Timeout - TimeoutRemain) * 100 / Timeout,
    0
    );
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
  return;
}

