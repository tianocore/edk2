/**@file

Copyright (c) 2004 - 2006, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BdsPlatform.c

Abstract:

  This file include all platform action which can be customized
  by IBV/OEM.

**/

#include "BdsPlatform.h"

WIN_NT_SYSTEM_CONFIGURATION mSystemConfigData;

VOID
SetupVariableInit (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINTN                           Size;

  Size = sizeof (mSystemConfigData);
  Status = gRT->GetVariable (
                  L"Setup",
                  &gEfiWinNtSystemConfigGuid,
                  NULL,
                  &Size,
                  (VOID *) &mSystemConfigData
                  );

  if (EFI_ERROR (Status)) {
    //
    // SetupVariable is corrupt
    //
    mSystemConfigData.ConOutRow = PcdGet32 (PcdConOutColumn);
    mSystemConfigData.ConOutColumn = PcdGet32 (PcdConOutRow);

    Status = gRT->SetVariable (
                    L"Setup",
                    &gEfiWinNtSystemConfigGuid,
                    EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    sizeof (mSystemConfigData),
                    (VOID *) &mSystemConfigData
                    );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "Failed to save Setup Variable to non-volatile storage, Status = %r\n", Status));
    }
  }
}

//
// BDS Platform Functions
//
VOID
EFIAPI
PlatformBdsInit (
  VOID
  )
/*++

Routine Description:

  Platform Bds init. Include the platform firmware vendor, revision
  and so crc check.

Arguments:

Returns:

  None.

--*/
{
  BdsLibSaveMemoryTypeInformation ();
  SetupVariableInit ();
}

EFI_STATUS
PlatformBdsConnectConsole (
  IN BDS_CONSOLE_CONNECT_ENTRY   *PlatformConsole
  )
/*++

Routine Description:

  Connect the predefined platform default console device. Always try to find
  and enable the vga device if have.

Arguments:

  PlatformConsole         - Predfined platform default console device array.
 
Returns:

  EFI_SUCCESS             - Success connect at least one ConIn and ConOut 
                            device, there must have one ConOut device is 
                            active vga device.
  
  EFI_STATUS              - Return the status of 
                            BdsLibConnectAllDefaultConsoles ()

--*/
{
  EFI_STATUS  Status;
  UINTN       Index;

  Index   = 0;
  Status  = EFI_SUCCESS;

  //
  // Have chance to connect the platform default console,
  // the platform default console is the minimue device group
  // the platform should support
  //
  while (PlatformConsole[Index].DevicePath != NULL) {
    //
    // Update the console variable with the connect type
    //
    if ((PlatformConsole[Index].ConnectType & CONSOLE_IN) == CONSOLE_IN) {
      BdsLibUpdateConsoleVariable (L"ConIn", PlatformConsole[Index].DevicePath, NULL);
    }

    if ((PlatformConsole[Index].ConnectType & CONSOLE_OUT) == CONSOLE_OUT) {
      BdsLibUpdateConsoleVariable (L"ConOut", PlatformConsole[Index].DevicePath, NULL);
    }

    if ((PlatformConsole[Index].ConnectType & STD_ERROR) == STD_ERROR) {
      BdsLibUpdateConsoleVariable (L"ErrOut", PlatformConsole[Index].DevicePath, NULL);
    }

    Index++;
  }
  //
  // Connect the all the default console with current cosole variable
  //
  Status = BdsLibConnectAllDefaultConsoles ();
  
  return Status;
}

VOID
PlatformBdsConnectSequence (
  VOID
  )
/*++

Routine Description:

  Connect with predeined platform connect sequence, 
  the OEM/IBV can customize with their own connect sequence.
  
Arguments:

  None.
 
Returns:

  None.
  
--*/
{
  UINTN Index;

  Index = 0;

  //
  // Here we can get the customized platform connect sequence
  // Notes: we can connect with new variable which record the
  // last time boots connect device path sequence
  //
  while (gPlatformConnectSequence[Index] != NULL) {
    //
    // Build the platform boot option
    //
    BdsLibConnectDevicePath (gPlatformConnectSequence[Index]);
    Index++;
  }

  //
  // Jst use the simple policy to connect all devices
  //
  BdsLibConnectAll ();
}

VOID
PlatformBdsGetDriverOption (
  IN OUT LIST_ENTRY              *BdsDriverLists
  )
/*++

Routine Description:

  Load the predefined driver option, OEM/IBV can customize this
  to load their own drivers
  
Arguments:

  BdsDriverLists  - The header of the driver option link list.
 
Returns:

  None.
  
--*/
{
  UINTN Index;

  Index = 0;

  //
  // Here we can get the customized platform driver option
  //
  while (gPlatformDriverOption[Index] != NULL) {
    //
    // Build the platform boot option
    //
    BdsLibRegisterNewOption (BdsDriverLists, gPlatformDriverOption[Index], NULL, L"DriverOrder");
    Index++;
  }

}

VOID
PlatformBdsDiagnostics (
  IN EXTENDMEM_COVERAGE_LEVEL    MemoryTestLevel,
  IN BOOLEAN                     QuietBoot,
  IN BASEM_MEMORY_TEST           BaseMemoryTest  
  )
/*++

Routine Description:

  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.
  
Arguments:

  MemoryTestLevel  - The memory test intensive level
  
  QuietBoot        - Indicate if need to enable the quiet boot

  BaseMemoryTest   - A pointer to BdsMemoryTest() 

Returns:

  None.
  
--*/
{
  EFI_STATUS  Status;

  //
  // Here we can decide if we need to show
  // the diagnostics screen
  // Notes: this quiet boot code should be remove
  // from the graphic lib
  //
  if (QuietBoot) {
    EnableQuietBoot (PcdGetPtr(PcdLogoFile));
    //
    // Perform system diagnostic
    //
    Status = BaseMemoryTest (MemoryTestLevel);
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
    }

    return ;
  }
  //
  // Perform system diagnostic
  //
  Status = BaseMemoryTest (MemoryTestLevel);
}

/**
  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.

  @param  DriverOptionList        The header of the driver option link list
  @param  BootOptionList          The header of the boot option link list
  @param  ProcessCapsules         A pointer to ProcessCapsules()
  @param  BaseMemoryTest          A pointer to BaseMemoryTest()

**/
VOID
EFIAPI
PlatformBdsPolicyBehavior (
  IN OUT LIST_ENTRY                  *DriverOptionList,
  IN OUT LIST_ENTRY                  *BootOptionList,
  IN PROCESS_CAPSULES                ProcessCapsules,
  IN BASEM_MEMORY_TEST               BaseMemoryTest
  )
{
  EFI_STATUS     Status;
  UINT16         Timeout;
  EFI_BOOT_MODE  BootMode;

  //
  // Init the time out value
  //
  Timeout = PcdGet16 (PcdPlatformBootTimeOut);

  //
  // Load the driver option as the driver option list
  //
  PlatformBdsGetDriverOption (DriverOptionList);

  //
  // Get current Boot Mode
  //
  Status = BdsLibGetBootMode (&BootMode);

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  switch (BootMode) {

  case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
  case BOOT_WITH_MINIMAL_CONFIGURATION:
    //
    // In no-configuration boot mode, we can connect the
    // console directly.
    //
    BdsLibConnectAllDefaultConsoles ();
    PlatformBdsDiagnostics ((EXTENDMEM_COVERAGE_LEVEL)IGNORE, TRUE, BaseMemoryTest);

    //
    // Perform some platform specific connect sequence
    //
    PlatformBdsConnectSequence ();

    //
    // Notes: current time out = 0 can not enter the
    // front page
    //
    PlatformBdsEnterFrontPage (Timeout, FALSE);

    //
    // Check the boot option with the boot option list
    //
    BdsLibBuildOptionFromVar (BootOptionList, L"BootOrder");
    break;

  case BOOT_ON_FLASH_UPDATE:
    //
    // Boot with the specific configuration
    //
    PlatformBdsConnectConsole (gPlatformConsole);
    PlatformBdsDiagnostics (EXTENSIVE, FALSE, BaseMemoryTest);
    BdsLibConnectAll ();
    ProcessCapsules (BOOT_ON_FLASH_UPDATE);
    break;

  case BOOT_IN_RECOVERY_MODE:
    //
    // In recovery mode, just connect platform console
    // and show up the front page
    //
    PlatformBdsConnectConsole (gPlatformConsole);
    PlatformBdsDiagnostics (EXTENSIVE, FALSE, BaseMemoryTest);

    //
    // In recovery boot mode, we still enter to the
    // frong page now
    //
    PlatformBdsEnterFrontPage (Timeout, FALSE);
    break;

  case BOOT_WITH_FULL_CONFIGURATION:
  case BOOT_WITH_FULL_CONFIGURATION_PLUS_DIAGNOSTICS:
  case BOOT_WITH_DEFAULT_SETTINGS:
  default:
    //
    // Connect platform console
    //
    Status = PlatformBdsConnectConsole (gPlatformConsole);
    if (EFI_ERROR (Status)) {
      //
      // Here OEM/IBV can customize with defined action
      //
      PlatformBdsNoConsoleAction ();
    }

    PlatformBdsDiagnostics ((EXTENDMEM_COVERAGE_LEVEL)IGNORE, TRUE, BaseMemoryTest);

    //
    // Perform some platform specific connect sequence
    //
    PlatformBdsConnectSequence ();

    //
    // Give one chance to enter the setup if we
    // have the time out
    //
    PlatformBdsEnterFrontPage (Timeout, FALSE);

    //
    // Here we have enough time to do the enumeration of boot device
    //
    BdsLibEnumerateAllBootOption (BootOptionList);
    break;
  }

  return ;

}

VOID
EFIAPI
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION   *Option
  )
/*++

Routine Description:
  
  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the EFI 1.0 specification defines that you will default to an
  interactive mode and stop processing the BootOrder list in this case. This
  is alos a platform implementation and can be customized by IBV/OEM.

Arguments:

  Option - Pointer to Boot Option that succeeded to boot.

Returns:
  
  None.

--*/
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with EFI_SUCCESS and there is not in the boot device
  // select loop then we need to pop up a UI and wait for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool (TmpStr);
  }
}

VOID
EFIAPI
PlatformBdsBootFail (
  IN  BDS_COMMON_OPTION  *Option,
  IN  EFI_STATUS         Status,
  IN  CHAR16             *ExitData,
  IN  UINTN              ExitDataSize
  )
/*++

Routine Description:
  
  Hook point after a boot attempt fails.

Arguments:
  
  Option - Pointer to Boot Option that failed to boot.

  Status - Status returned from failed boot.

  ExitData - Exit data returned from failed boot.

  ExitDataSize - Exit data size returned from failed boot.

Returns:
  
  None.

--*/
{
  CHAR16  *TmpStr;

  //
  // If Boot returned with failed status then we need to pop up a UI and wait
  // for user input.
  //
  TmpStr = Option->StatusString;
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool (TmpStr);
  }

}

EFI_STATUS
PlatformBdsNoConsoleAction (
  VOID
  )
/*++

Routine Description:
  
  This function is remained for IBV/OEM to do some platform action,
  if there no console device can be connected.

Arguments:
  
  None.
  
Returns:
  
  EFI_SUCCESS      - Direct return success now.

--*/
{
  return EFI_SUCCESS;
}

/**
  This function locks platform flash that is not allowed to be updated during normal boot path.
  The flash layout is platform specific.

  **/
VOID
EFIAPI
PlatformBdsLockNonUpdatableFlash (
  VOID
  )
{
  return;
}

/**
  Lock the ConsoleIn device in system table. All key
  presses will be ignored until the Password is typed in. The only way to
  disable the password is to type it in to a ConIn device.

  @param  Password        Password used to lock ConIn device.

  @retval EFI_SUCCESS     lock the Console In Spliter virtual handle successfully.
  @retval EFI_UNSUPPORTED Password not found

**/
EFI_STATUS
EFIAPI
LockKeyboards (
  IN  CHAR16    *Password
  )
{
    return EFI_UNSUPPORTED;
}
