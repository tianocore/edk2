/*++

Copyright (c) 2006 - 2007, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
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

--*/

#include "Generic/Bds.h"
#include "BdsPlatform.h"
#include "Generic/BdsString.h"
#include "Generic/Language.h"
#include "Generic/FrontPage.h"

CHAR16  mFirmwareVendor[] = L"TianoCore.org";

EFI_EVENT  mReadyToBootEvent;

EFI_MEMORY_TYPE_INFORMATION mDefaultMemoryTypeInformation[] = {
  { EfiACPIReclaimMemory,   0 },
  { EfiACPIMemoryNVS,       0 },
  { EfiReservedMemoryType,  0 },
  { EfiRuntimeServicesData, 0 },
  { EfiRuntimeServicesCode, 0 },
  { EfiBootServicesCode,    0 },
  { EfiBootServicesData,    0 },
  { EfiLoaderCode,          0 },
  { EfiLoaderData,          0 },
  { EfiMaxMemoryType,       0 }
};

VOID 
BdsSetMemoryTypeInformationVariable (
 VOID
 )
{
  EFI_STATUS                   Status;
  EFI_MEMORY_TYPE_INFORMATION  *PreviousMemoryTypeInformation;
  EFI_MEMORY_TYPE_INFORMATION  *CurrentMemoryTypeInformation;
  UINTN                        VariableSize;
  BOOLEAN                      UpdateRequired;
  UINTN                        Index;
  UINTN                        Index1;
  UINT32                       Previous;
  UINT32                       Current;
  UINT32                       Next;
  EFI_HOB_GUID_TYPE            *Hob;

  UpdateRequired = FALSE;

  //
  // Retrieve the current memory usage statistics.  If they are not found, then
  // no adjustments can be made to the Memory Type Information variable.
  //
  Status = EfiGetSystemConfigurationTable (&gEfiMemoryTypeInformationGuid, (VOID **)&CurrentMemoryTypeInformation);
  if (EFI_ERROR (Status)) {
    return;
  }

  //
  // Get the Memory Type Information settings from a previous boot if they exist.
  //
  PreviousMemoryTypeInformation = BdsLibGetVariableAndSize (
                                    EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                                    &gEfiMemoryTypeInformationGuid,
                                    &VariableSize
                                    );

  //
  // If the previous Memory Type Information is not available, then set defaults
  //
  if (PreviousMemoryTypeInformation == NULL) {
    Hob = GetFirstGuidHob (&gEfiMemoryTypeInformationGuid);

    if (Hob != NULL) {
      PreviousMemoryTypeInformation = GET_GUID_HOB_DATA (Hob);
      VariableSize = GET_GUID_HOB_DATA_SIZE (Hob);
      ASSERT (VariableSize == sizeof (mDefaultMemoryTypeInformation));
      CopyMem (mDefaultMemoryTypeInformation, PreviousMemoryTypeInformation, VariableSize);
    } else {
      PreviousMemoryTypeInformation = mDefaultMemoryTypeInformation;
      VariableSize = sizeof (mDefaultMemoryTypeInformation);
    }
    UpdateRequired = TRUE;
  }

  //
  // Use a hueristic to adjust the Memory Type Information for the next boot
  //
  for (Index = 0; PreviousMemoryTypeInformation[Index].Type != EfiMaxMemoryType; Index++) {

    Current = 0;
    for (Index1 = 0; CurrentMemoryTypeInformation[Index1].Type != EfiMaxMemoryType; Index1++) {
      if (PreviousMemoryTypeInformation[Index].Type == CurrentMemoryTypeInformation[Index1].Type) {
        Current = CurrentMemoryTypeInformation[Index1].NumberOfPages;
        break;
      }
    }

    if (CurrentMemoryTypeInformation[Index1].Type == EfiMaxMemoryType) {
      continue;
    }

    Previous = PreviousMemoryTypeInformation[Index].NumberOfPages;

    if (Current > Previous) {
      Next = Current + (Current >> 2);
    } else if (Current < (Previous >> 1)) {
      Next = Previous - (Previous >> 2);
    } else {
      Next = Previous;
    }
    if (Next > 0 && Next <= 16) {
      Next = 16;
    }

    if (Next != Previous) {
      PreviousMemoryTypeInformation[Index].NumberOfPages = Next;
      UpdateRequired = TRUE;
    }
  }

  //
  // If any changes were made to the Memory Type Information settings, then set the new variable value
  //
  if (UpdateRequired) {
    Status = gRT->SetVariable (
                    EFI_MEMORY_TYPE_INFORMATION_VARIABLE_NAME,
                    &gEfiMemoryTypeInformationGuid,
                    EFI_VARIABLE_NON_VOLATILE  | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                    VariableSize,
                    PreviousMemoryTypeInformation
                    );
  }

  return;
}


/*++

Routine Description:

  This routine is a notification function for legayc boot or exit boot
  service event. It will adjust the memory information for different
  memory type and save them into the variables for next boot

Arguments:

  Event    - The event that triggered this notification function
  Context  - Pointer to the notification functions context

Returns:

  None.

--*/
VOID
EFIAPI
BdsReadyToBootEvent (
  EFI_EVENT  Event,
  VOID       *Context
  )
{
  BdsSetMemoryTypeInformationVariable ();
}

//
// BDS Platform Functions
//
VOID
PlatformBdsInit (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData
  )
/*++

Routine Description:

  Platform Bds init. Incude the platform firmware vendor, revision
  and so crc check.

Arguments:

  PrivateData  - The EFI_BDS_ARCH_PROTOCOL_INSTANCE instance

Returns:

  None.

--*/
{
  //
  // set firmwarevendor, here can be IBV/OEM customize
  //
  gST->FirmwareVendor = AllocateRuntimeCopyPool (
                          sizeof (mFirmwareVendor),
                          &mFirmwareVendor
                          );
  ASSERT (gST->FirmwareVendor != NULL);

  gST->FirmwareRevision = FIRMWARE_REVISION;

  //
  // Fixup Tasble CRC after we updated Firmware Vendor and Revision
  //
  gBS->CalculateCrc32 ((VOID *) gST, sizeof (EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);

  //
  // Create Ready To Boot event
  // 
  EfiCreateEventReadyToBootEx (
    TPL_CALLBACK,
    BdsReadyToBootEvent,
    NULL,
    &mReadyToBootEvent
    );

  //
  // Initialize the platform specific string and language
  //
  InitializeStringSupport ();
  InitializeLanguage (TRUE);
  InitializeFrontPage (FALSE);

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
  if (EFI_ERROR (Status)) {
    return Status;
  }

  return EFI_SUCCESS;
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
  IN BOOLEAN                     QuietBoot
  )
/*++

Routine Description:

  Perform the platform diagnostic, such like test memory. OEM/IBV also
  can customize this fuction to support specific platform diagnostic.
  
Arguments:

  MemoryTestLevel  - The memory test intensive level
  
  QuietBoot        - Indicate if need to enable the quiet boot
 
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
    EnableQuietBoot (&gEfiDefaultBmpLogoGuid);
    //
    // Perform system diagnostic
    //
    Status = BdsMemoryTest (MemoryTestLevel);
    if (EFI_ERROR (Status)) {
      DisableQuietBoot ();
    }

    return ;
  }
  //
  // Perform system diagnostic
  //
  Status = BdsMemoryTest (MemoryTestLevel);
}

VOID
PlatformBdsPolicyBehavior (
  IN EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData,
  IN OUT LIST_ENTRY                  *DriverOptionList,
  IN OUT LIST_ENTRY                  *BootOptionList
  )
/*++

Routine Description:

  The function will excute with as the platform policy, current policy
  is driven by boot mode. IBV/OEM can customize this code for their specific
  policy action.
  
Arguments:

  PrivateData      - The EFI_BDS_ARCH_PROTOCOL_INSTANCE instance
  
  DriverOptionList - The header of the driver option link list
  
  BootOptionList   - The header of the boot option link list
 
Returns:

  None.
  
--*/
{
  EFI_STATUS  Status;
  UINT16      Timeout;

  //
  // Init the time out value
  //
  Timeout = BdsLibGetTimeout ();

  //
  // Load the driver option as the driver option list
  //
  PlatformBdsGetDriverOption (DriverOptionList);

  //
  // Get current Boot Mode
  //
  PrivateData->BootMode = GetBootModeHob();

  //
  // Go the different platform policy with different boot mode
  // Notes: this part code can be change with the table policy
  //
  switch (PrivateData->BootMode) {

  case BOOT_ASSUMING_NO_CONFIGURATION_CHANGES:
  case BOOT_WITH_MINIMAL_CONFIGURATION:
    //
    // In no-configuration boot mode, we can connect the
    // console directly.
    //
    BdsLibConnectAllDefaultConsoles ();
    PlatformBdsDiagnostics (IGNORE, TRUE);

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
    PlatformBdsDiagnostics (EXTENSIVE, FALSE);
    BdsLibConnectAll ();
    ProcessCapsules (BOOT_ON_FLASH_UPDATE);
    break;

  case BOOT_IN_RECOVERY_MODE:
    //
    // In recovery mode, just connect platform console
    // and show up the front page
    //
    PlatformBdsConnectConsole (gPlatformConsole);
    PlatformBdsDiagnostics (EXTENSIVE, FALSE);

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

    PlatformBdsDiagnostics (IGNORE, TRUE);

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
PlatformBdsBootSuccess (
  IN  BDS_COMMON_OPTION *Option
  )
/*++

Routine Description:
  
  Hook point after a boot attempt succeeds. We don't expect a boot option to
  return, so the UEFI 2.0 specification defines that you will default to an
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
  TmpStr = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
  if (TmpStr != NULL) {
    BdsLibOutputStrings (gST->ConOut, TmpStr, Option->Description, L"\n\r", NULL);
    FreePool (TmpStr);
  }
}

VOID
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
  TmpStr = GetStringById (STRING_TOKEN (STR_BOOT_FAILED));
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
