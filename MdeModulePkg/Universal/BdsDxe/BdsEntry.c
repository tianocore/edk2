/** @file
  This module produce main entry for BDS phase - BdsEntry.
  When this module was dispatched by DxeCore, gEfiBdsArchProtocolGuid will be installed
  which contains interface of BdsEntry.
  After DxeCore finish DXE phase, gEfiBdsArchProtocolGuid->BdsEntry will be invoked
  to enter BDS phase.

Copyright (c) 2004 - 2019, Intel Corporation. All rights reserved.<BR>
(C) Copyright 2016-2019 Hewlett Packard Enterprise Development LP<BR>
(C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Bds.h"
#include "Language.h"
#include "HwErrRecSupport.h"
#include <Library/VariablePolicyHelperLib.h>

#define SET_BOOT_OPTION_SUPPORT_KEY_COUNT(a, c) {  \
      (a) = ((a) & ~EFI_BOOT_OPTION_SUPPORT_COUNT) | (((c) << LowBitSet32 (EFI_BOOT_OPTION_SUPPORT_COUNT)) & EFI_BOOT_OPTION_SUPPORT_COUNT); \
      }

///
/// BDS arch protocol instance initial value.
///
EFI_BDS_ARCH_PROTOCOL  gBds = {
  BdsEntry
};

//
// gConnectConInEvent - Event which is signaled when ConIn connection is required
//
EFI_EVENT      gConnectConInEvent = NULL;

///
/// The read-only variables defined in UEFI Spec.
///
CHAR16  *mReadOnlyVariables[] = {
  EFI_PLATFORM_LANG_CODES_VARIABLE_NAME,
  EFI_LANG_CODES_VARIABLE_NAME,
  EFI_BOOT_OPTION_SUPPORT_VARIABLE_NAME,
  EFI_HW_ERR_REC_SUPPORT_VARIABLE_NAME,
  EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME
  };

CHAR16 *mBdsLoadOptionName[] = {
  L"Driver",
  L"SysPrep",
  L"Boot",
  L"PlatformRecovery"
};

/**
  Event to Connect ConIn.

  @param  Event                 Event whose notification function is being invoked.
  @param  Context               Pointer to the notification function's context,
                                which is implementation-dependent.

**/
VOID
EFIAPI
BdsDxeOnConnectConInCallBack (
  IN EFI_EVENT                Event,
  IN VOID                     *Context
  )
{
  EFI_STATUS Status;

  //
  // When Osloader call ReadKeyStroke to signal this event
  // no driver dependency is assumed existing. So use a non-dispatch version
  //
  Status = EfiBootManagerConnectConsoleVariable (ConIn);
  if (EFI_ERROR (Status)) {
    //
    // Should not enter this case, if enter, the keyboard will not work.
    // May need platfrom policy to connect keyboard.
    //
    DEBUG ((EFI_D_WARN, "[Bds] Connect ConIn failed - %r!!!\n", Status));
  }
}
/**
  Notify function for event group EFI_EVENT_GROUP_READY_TO_BOOT. This is used to
  check whether there is remaining deferred load images.

  @param[in]  Event   The Event that is being processed.
  @param[in]  Context The Event Context.

**/
VOID
EFIAPI
CheckDeferredLoadImageOnReadyToBoot (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  EFI_STATUS                         Status;
  EFI_DEFERRED_IMAGE_LOAD_PROTOCOL   *DeferredImage;
  UINTN                              HandleCount;
  EFI_HANDLE                         *Handles;
  UINTN                              Index;
  UINTN                              ImageIndex;
  EFI_DEVICE_PATH_PROTOCOL           *ImageDevicePath;
  VOID                               *Image;
  UINTN                              ImageSize;
  BOOLEAN                            BootOption;
  CHAR16                             *DevicePathStr;

  //
  // Find all the deferred image load protocols.
  //
  HandleCount = 0;
  Handles = NULL;
  Status = gBS->LocateHandleBuffer (
    ByProtocol,
    &gEfiDeferredImageLoadProtocolGuid,
    NULL,
    &HandleCount,
    &Handles
  );
  if (EFI_ERROR (Status)) {
    return;
  }

  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (Handles[Index], &gEfiDeferredImageLoadProtocolGuid, (VOID **) &DeferredImage);
    if (EFI_ERROR (Status)) {
      continue;
    }

    for (ImageIndex = 0; ; ImageIndex++) {
      //
      // Load all the deferred images in this protocol instance.
      //
      Status = DeferredImage->GetImageInfo (
        DeferredImage,
        ImageIndex,
        &ImageDevicePath,
        (VOID **) &Image,
        &ImageSize,
        &BootOption
      );
      if (EFI_ERROR (Status)) {
        break;
      }
      DevicePathStr = ConvertDevicePathToText (ImageDevicePath, FALSE, FALSE);
      DEBUG ((DEBUG_LOAD, "[Bds] Image was deferred but not loaded: %s.\n", DevicePathStr));
      if (DevicePathStr != NULL) {
        FreePool (DevicePathStr);
      }
    }
  }
  if (Handles != NULL) {
    FreePool (Handles);
  }
}

/**

  Install Boot Device Selection Protocol

  @param ImageHandle     The image handle.
  @param SystemTable     The system table.

  @retval  EFI_SUCEESS  BDS has finished initializing.
                        Return the dispatcher and recall BDS.Entry
  @retval  Other        Return status from AllocatePool() or gBS->InstallProtocolInterface

**/
EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;
  //
  // Install protocol interface
  //
  Handle = NULL;
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
                  &gEfiBdsArchProtocolGuid, &gBds,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  DEBUG_CODE (
    EFI_EVENT   Event;
    //
    // Register notify function to check deferred images on ReadyToBoot Event.
    //
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    CheckDeferredLoadImageOnReadyToBoot,
                    NULL,
                    &gEfiEventReadyToBootGuid,
                    &Event
                    );
    ASSERT_EFI_ERROR (Status);
  );
  return Status;
}

/**
  Function waits for a given event to fire, or for an optional timeout to expire.

  @param   Event              The event to wait for
  @param   Timeout            An optional timeout value in 100 ns units.

  @retval  EFI_SUCCESS      Event fired before Timeout expired.
  @retval  EFI_TIME_OUT     Timout expired before Event fired..

**/
EFI_STATUS
BdsWaitForSingleEvent (
  IN  EFI_EVENT                  Event,
  IN  UINT64                     Timeout       OPTIONAL
  )
{
  UINTN       Index;
  EFI_STATUS  Status;
  EFI_EVENT   TimerEvent;
  EFI_EVENT   WaitList[2];

  if (Timeout != 0) {
    //
    // Create a timer event
    //
    Status = gBS->CreateEvent (EVT_TIMER, 0, NULL, NULL, &TimerEvent);
    if (!EFI_ERROR (Status)) {
      //
      // Set the timer event
      //
      gBS->SetTimer (
             TimerEvent,
             TimerRelative,
             Timeout
             );

      //
      // Wait for the original event or the timer
      //
      WaitList[0] = Event;
      WaitList[1] = TimerEvent;
      Status      = gBS->WaitForEvent (2, WaitList, &Index);
      ASSERT_EFI_ERROR (Status);
      gBS->CloseEvent (TimerEvent);

      //
      // If the timer expired, change the return to timed out
      //
      if (Index == 1) {
        Status = EFI_TIMEOUT;
      }
    }
  } else {
    //
    // No timeout... just wait on the event
    //
    Status = gBS->WaitForEvent (1, &Event, &Index);
    ASSERT (!EFI_ERROR (Status));
    ASSERT (Index == 0);
  }

  return Status;
}

/**
  The function reads user inputs.

**/
VOID
BdsReadKeys (
  VOID
  )
{
  EFI_STATUS         Status;
  EFI_INPUT_KEY      Key;

  if (PcdGetBool (PcdConInConnectOnDemand)) {
    return;
  }

  while (gST->ConIn != NULL) {

    Status = gST->ConIn->ReadKeyStroke (gST->ConIn, &Key);

    if (EFI_ERROR (Status)) {
      //
      // No more keys.
      //
      break;
    }
  }
}

/**
  The function waits for the boot manager timeout expires or hotkey is pressed.

  It calls PlatformBootManagerWaitCallback each second.

  @param     HotkeyTriggered   Input hotkey event.
**/
VOID
BdsWait (
  IN EFI_EVENT      HotkeyTriggered
  )
{
  EFI_STATUS            Status;
  UINT16                TimeoutRemain;

  DEBUG ((EFI_D_INFO, "[Bds]BdsWait ...Zzzzzzzzzzzz...\n"));

  TimeoutRemain = PcdGet16 (PcdPlatformBootTimeOut);
  while (TimeoutRemain != 0) {
    DEBUG ((EFI_D_INFO, "[Bds]BdsWait(%d)..Zzzz...\n", (UINTN) TimeoutRemain));
    PlatformBootManagerWaitCallback (TimeoutRemain);

    BdsReadKeys (); // BUGBUG: Only reading can signal HotkeyTriggered
                    //         Can be removed after all keyboard drivers invoke callback in timer callback.

    if (HotkeyTriggered != NULL) {
      Status = BdsWaitForSingleEvent (HotkeyTriggered, EFI_TIMER_PERIOD_SECONDS (1));
      if (!EFI_ERROR (Status)) {
        break;
      }
    } else {
      gBS->Stall (1000000);
    }

    //
    // 0xffff means waiting forever
    // BDS with no hotkey provided and 0xffff as timeout will "hang" in the loop
    //
    if (TimeoutRemain != 0xffff) {
      TimeoutRemain--;
    }
  }

  //
  // If the platform configured a nonzero and finite time-out, and we have
  // actually reached that, report 100% completion to the platform.
  //
  // Note that the (TimeoutRemain == 0) condition excludes
  // PcdPlatformBootTimeOut=0xFFFF, and that's deliberate.
  //
  if (PcdGet16 (PcdPlatformBootTimeOut) != 0 && TimeoutRemain == 0) {
    PlatformBootManagerWaitCallback (0);
  }
  DEBUG ((EFI_D_INFO, "[Bds]Exit the waiting!\n"));
}

/**
  Attempt to boot each boot option in the BootOptions array.

  @param BootOptions       Input boot option array.
  @param BootOptionCount   Input boot option count.
  @param BootManagerMenu   Input boot manager menu.

  @retval TRUE  Successfully boot one of the boot options.
  @retval FALSE Failed boot any of the boot options.
**/
BOOLEAN
BootBootOptions (
  IN EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions,
  IN UINTN                           BootOptionCount,
  IN EFI_BOOT_MANAGER_LOAD_OPTION    *BootManagerMenu OPTIONAL
  )
{
  UINTN                              Index;

  //
  // Report Status Code to indicate BDS starts attempting booting from the UEFI BootOrder list.
  //
  REPORT_STATUS_CODE (EFI_PROGRESS_CODE, (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_ATTEMPT_BOOT_ORDER_EVENT));

  //
  // Attempt boot each boot option
  //
  for (Index = 0; Index < BootOptionCount; Index++) {
    //
    // According to EFI Specification, if a load option is not marked
    // as LOAD_OPTION_ACTIVE, the boot manager will not automatically
    // load the option.
    //
    if ((BootOptions[Index].Attributes & LOAD_OPTION_ACTIVE) == 0) {
      continue;
    }

    //
    // Boot#### load options with LOAD_OPTION_CATEGORY_APP are executables which are not
    // part of the normal boot processing. Boot options with reserved category values will be
    // ignored by the boot manager.
    //
    if ((BootOptions[Index].Attributes & LOAD_OPTION_CATEGORY) != LOAD_OPTION_CATEGORY_BOOT) {
      continue;
    }

    //
    // All the driver options should have been processed since
    // now boot will be performed.
    //
    EfiBootManagerBoot (&BootOptions[Index]);

    //
    // If the boot via Boot#### returns with a status of EFI_SUCCESS, platform firmware
    // supports boot manager menu, and if firmware is configured to boot in an
    // interactive mode, the boot manager will stop processing the BootOrder variable and
    // present a boot manager menu to the user.
    //
    if ((BootManagerMenu != NULL) && (BootOptions[Index].Status == EFI_SUCCESS)) {
      EfiBootManagerBoot (BootManagerMenu);
      break;
    }
  }

  return (BOOLEAN) (Index < BootOptionCount);
}

/**
  The function will load and start every Driver####, SysPrep#### or PlatformRecovery####.

  @param  LoadOptions        Load option array.
  @param  LoadOptionCount    Load option count.
**/
VOID
ProcessLoadOptions (
  IN EFI_BOOT_MANAGER_LOAD_OPTION       *LoadOptions,
  IN UINTN                              LoadOptionCount
  )
{
  EFI_STATUS                        Status;
  UINTN                             Index;
  BOOLEAN                           ReconnectAll;
  EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType;

  ReconnectAll   = FALSE;
  LoadOptionType = LoadOptionTypeMax;

  //
  // Process the driver option
  //
  for (Index = 0; Index < LoadOptionCount; Index++) {
    //
    // All the load options in the array should be of the same type.
    //
    if (Index == 0) {
      LoadOptionType = LoadOptions[Index].OptionType;
    }
    ASSERT (LoadOptionType == LoadOptions[Index].OptionType);
    ASSERT (LoadOptionType != LoadOptionTypeBoot);

    Status = EfiBootManagerProcessLoadOption (&LoadOptions[Index]);

    //
    // Status indicates whether the load option is loaded and executed
    // LoadOptions[Index].Status is what the load option returns
    //
    if (!EFI_ERROR (Status)) {
      //
      // Stop processing if any PlatformRecovery#### returns success.
      //
      if ((LoadOptions[Index].Status == EFI_SUCCESS) &&
          (LoadOptionType == LoadOptionTypePlatformRecovery)) {
        break;
      }

      //
      // Only set ReconnectAll flag when the load option executes successfully.
      //
      if (!EFI_ERROR (LoadOptions[Index].Status) &&
          (LoadOptions[Index].Attributes & LOAD_OPTION_FORCE_RECONNECT) != 0) {
        ReconnectAll = TRUE;
      }
    }
  }

  //
  // If a driver load option is marked as LOAD_OPTION_FORCE_RECONNECT,
  // then all of the EFI drivers in the system will be disconnected and
  // reconnected after the last driver load option is processed.
  //
  if (ReconnectAll && LoadOptionType == LoadOptionTypeDriver) {
    EfiBootManagerDisconnectAll ();
    EfiBootManagerConnectAll ();
  }
}

/**

  Validate input console variable data.

  If found the device path is not a valid device path, remove the variable.

  @param VariableName             Input console variable name.

**/
VOID
BdsFormalizeConsoleVariable (
  IN  CHAR16          *VariableName
  )
{
  EFI_DEVICE_PATH_PROTOCOL  *DevicePath;
  UINTN                     VariableSize;
  EFI_STATUS                Status;

  GetEfiGlobalVariable2 (VariableName, (VOID **) &DevicePath, &VariableSize);
  if ((DevicePath != NULL) && !IsDevicePathValid (DevicePath, VariableSize)) {
    Status = gRT->SetVariable (
                    VariableName,
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    //
    // Deleting variable with current variable implementation shouldn't fail.
    //
    ASSERT_EFI_ERROR (Status);
  }

  if (DevicePath != NULL) {
    FreePool (DevicePath);
  }
}

/**
  Formalize OsIndication related variables.

  For OsIndicationsSupported, Create a BS/RT/UINT64 variable to report caps
  Delete OsIndications variable if it is not NV/BS/RT UINT64.

  Item 3 is used to solve case when OS corrupts OsIndications. Here simply delete this NV variable.

  Create a boot option for BootManagerMenu if it hasn't been created yet

**/
VOID
BdsFormalizeOSIndicationVariable (
  VOID
  )
{
  EFI_STATUS                      Status;
  UINT64                          OsIndicationSupport;
  UINT64                          OsIndication;
  UINTN                           DataSize;
  UINT32                          Attributes;
  EFI_BOOT_MANAGER_LOAD_OPTION    BootManagerMenu;

  //
  // OS indicater support variable
  //
  Status = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
  if (Status != EFI_NOT_FOUND) {
    OsIndicationSupport = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
    EfiBootManagerFreeLoadOption (&BootManagerMenu);
  } else {
    OsIndicationSupport = 0;
  }

  if (PcdGetBool (PcdPlatformRecoverySupport)) {
    OsIndicationSupport |= EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY;
  }

  if (PcdGetBool(PcdCapsuleOnDiskSupport)) {
    OsIndicationSupport |= EFI_OS_INDICATIONS_FILE_CAPSULE_DELIVERY_SUPPORTED;
  }

  Status = gRT->SetVariable (
                  EFI_OS_INDICATIONS_SUPPORT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof(UINT64),
                  &OsIndicationSupport
                  );
  //
  // Platform needs to make sure setting volatile variable before calling 3rd party code shouldn't fail.
  //
  ASSERT_EFI_ERROR (Status);

  //
  // If OsIndications is invalid, remove it.
  // Invalid case
  //   1. Data size != UINT64
  //   2. OsIndication value inconsistence
  //   3. OsIndication attribute inconsistence
  //
  OsIndication = 0;
  Attributes = 0;
  DataSize = sizeof(UINT64);
  Status = gRT->GetVariable (
                  EFI_OS_INDICATIONS_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  &Attributes,
                  &DataSize,
                  &OsIndication
                  );
  if (Status == EFI_NOT_FOUND) {
    return;
  }

  if ((DataSize != sizeof (OsIndication)) ||
      ((OsIndication & ~OsIndicationSupport) != 0) ||
      (Attributes != (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE))
     ){

    DEBUG ((EFI_D_ERROR, "[Bds] Unformalized OsIndications variable exists. Delete it\n"));
    Status = gRT->SetVariable (
                    EFI_OS_INDICATIONS_VARIABLE_NAME,
                    &gEfiGlobalVariableGuid,
                    0,
                    0,
                    NULL
                    );
    //
    // Deleting variable with current variable implementation shouldn't fail.
    //
    ASSERT_EFI_ERROR(Status);
  }
}

/**

  Validate variables.

**/
VOID
BdsFormalizeEfiGlobalVariable (
  VOID
  )
{
  //
  // Validate Console variable.
  //
  BdsFormalizeConsoleVariable (EFI_CON_IN_VARIABLE_NAME);
  BdsFormalizeConsoleVariable (EFI_CON_OUT_VARIABLE_NAME);
  BdsFormalizeConsoleVariable (EFI_ERR_OUT_VARIABLE_NAME);

  //
  // Validate OSIndication related variable.
  //
  BdsFormalizeOSIndicationVariable ();
}

/**

  Service routine for BdsInstance->Entry(). Devices are connected, the
  consoles are initialized, and the boot options are tried.

  @param This             Protocol Instance structure.

**/
VOID
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
{
  EFI_BOOT_MANAGER_LOAD_OPTION    *LoadOptions;
  UINTN                           LoadOptionCount;
  CHAR16                          *FirmwareVendor;
  EFI_EVENT                       HotkeyTriggered;
  UINT64                          OsIndication;
  UINTN                           DataSize;
  EFI_STATUS                      Status;
  UINT32                          BootOptionSupport;
  UINT16                          BootTimeOut;
  EDKII_VARIABLE_POLICY_PROTOCOL  *VariablePolicy;
  UINTN                           Index;
  EFI_BOOT_MANAGER_LOAD_OPTION    LoadOption;
  UINT16                          *BootNext;
  CHAR16                          BootNextVariableName[sizeof ("Boot####")];
  EFI_BOOT_MANAGER_LOAD_OPTION    BootManagerMenu;
  BOOLEAN                         BootFwUi;
  BOOLEAN                         PlatformRecovery;
  BOOLEAN                         BootSuccess;
  EFI_DEVICE_PATH_PROTOCOL        *FilePath;
  EFI_STATUS                      BootManagerMenuStatus;
  EFI_BOOT_MANAGER_LOAD_OPTION    PlatformDefaultBootOption;

  HotkeyTriggered = NULL;
  Status          = EFI_SUCCESS;
  BootSuccess     = FALSE;

  //
  // Insert the performance probe
  //
  PERF_CROSSMODULE_END("DXE");
  PERF_CROSSMODULE_BEGIN("BDS");
  DEBUG ((EFI_D_INFO, "[Bds] Entry...\n"));

  //
  // Fill in FirmwareVendor and FirmwareRevision from PCDs
  //
  FirmwareVendor = (CHAR16 *) PcdGetPtr (PcdFirmwareVendor);
  gST->FirmwareVendor = AllocateRuntimeCopyPool (StrSize (FirmwareVendor), FirmwareVendor);
  ASSERT (gST->FirmwareVendor != NULL);
  gST->FirmwareRevision = PcdGet32 (PcdFirmwareRevision);

  //
  // Fixup Tasble CRC after we updated Firmware Vendor and Revision
  //
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 ((VOID *) gST, sizeof (EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);

  //
  // Validate Variable.
  //
  BdsFormalizeEfiGlobalVariable ();

  //
  // Mark the read-only variables if the Variable Lock protocol exists
  //
  Status = gBS->LocateProtocol(&gEdkiiVariablePolicyProtocolGuid, NULL, (VOID**)&VariablePolicy);
  DEBUG((DEBUG_INFO, "[BdsDxe] Locate Variable Policy protocol - %r\n", Status));
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < ARRAY_SIZE (mReadOnlyVariables); Index++) {
      Status = RegisterBasicVariablePolicy(
                 VariablePolicy,
                 &gEfiGlobalVariableGuid,
                 mReadOnlyVariables[Index],
                 VARIABLE_POLICY_NO_MIN_SIZE,
                 VARIABLE_POLICY_NO_MAX_SIZE,
                 VARIABLE_POLICY_NO_MUST_ATTR,
                 VARIABLE_POLICY_NO_CANT_ATTR,
                 VARIABLE_POLICY_TYPE_LOCK_NOW
                 );
      ASSERT_EFI_ERROR(Status);
    }
  }

  InitializeHwErrRecSupport ();

  //
  // Initialize L"Timeout" EFI global variable.
  //
  BootTimeOut = PcdGet16 (PcdPlatformBootTimeOut);
  if (BootTimeOut != 0xFFFF) {
    //
    // If time out value equal 0xFFFF, no need set to 0xFFFF to variable area because UEFI specification
    // define same behavior between no value or 0xFFFF value for L"Timeout".
    //
    BdsDxeSetVariableAndReportStatusCodeOnError (
      EFI_TIME_OUT_VARIABLE_NAME,
      &gEfiGlobalVariableGuid,
      EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
      sizeof (UINT16),
      &BootTimeOut
      );
  }

  //
  // Initialize L"BootOptionSupport" EFI global variable.
  // Lazy-ConIn implictly disables BDS hotkey.
  //
  BootOptionSupport = EFI_BOOT_OPTION_SUPPORT_APP | EFI_BOOT_OPTION_SUPPORT_SYSPREP;
  if (!PcdGetBool (PcdConInConnectOnDemand)) {
    BootOptionSupport |= EFI_BOOT_OPTION_SUPPORT_KEY;
    SET_BOOT_OPTION_SUPPORT_KEY_COUNT (BootOptionSupport, 3);
  }
  Status = gRT->SetVariable (
                  EFI_BOOT_OPTION_SUPPORT_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
                  sizeof (BootOptionSupport),
                  &BootOptionSupport
                  );
  //
  // Platform needs to make sure setting volatile variable before calling 3rd party code shouldn't fail.
  //
  ASSERT_EFI_ERROR (Status);

  //
  // Cache the "BootNext" NV variable before calling any PlatformBootManagerLib APIs
  // This could avoid the "BootNext" set by PlatformBootManagerLib be consumed in this boot.
  //
  GetEfiGlobalVariable2 (EFI_BOOT_NEXT_VARIABLE_NAME, (VOID **) &BootNext, &DataSize);
  if (DataSize != sizeof (UINT16)) {
    if (BootNext != NULL) {
      FreePool (BootNext);
    }
    BootNext = NULL;
  }

  //
  // Initialize the platform language variables
  //
  InitializeLanguage (TRUE);

  FilePath = FileDevicePath (NULL, EFI_REMOVABLE_MEDIA_FILE_NAME);
  if (FilePath == NULL) {
    DEBUG ((DEBUG_ERROR, "Fail to allocate memory for default boot file path. Unable to boot.\n"));
    CpuDeadLoop ();
  }
  Status = EfiBootManagerInitializeLoadOption (
             &PlatformDefaultBootOption,
             LoadOptionNumberUnassigned,
             LoadOptionTypePlatformRecovery,
             LOAD_OPTION_ACTIVE,
             L"Default PlatformRecovery",
             FilePath,
             NULL,
             0
             );
  ASSERT_EFI_ERROR (Status);

  //
  // System firmware must include a PlatformRecovery#### variable specifying
  // a short-form File Path Media Device Path containing the platform default
  // file path for removable media if the platform supports Platform Recovery.
  //
  if (PcdGetBool (PcdPlatformRecoverySupport)) {
    LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionTypePlatformRecovery);
    if (EfiBootManagerFindLoadOption (&PlatformDefaultBootOption, LoadOptions, LoadOptionCount) == -1) {
      for (Index = 0; Index < LoadOptionCount; Index++) {
        //
        // The PlatformRecovery#### options are sorted by OptionNumber.
        // Find the the smallest unused number as the new OptionNumber.
        //
        if (LoadOptions[Index].OptionNumber != Index) {
          break;
        }
      }
      PlatformDefaultBootOption.OptionNumber = Index;
      Status = EfiBootManagerLoadOptionToVariable (&PlatformDefaultBootOption);
      ASSERT_EFI_ERROR (Status);
    }
    EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);
  }
  FreePool (FilePath);

  //
  // Report Status Code to indicate connecting drivers will happen
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
    );

  //
  // Initialize ConnectConIn event before calling platform code.
  //
  if (PcdGetBool (PcdConInConnectOnDemand)) {
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    BdsDxeOnConnectConInCallBack,
                    NULL,
                    &gConnectConInEventGuid,
                    &gConnectConInEvent
                    );
    if (EFI_ERROR (Status)) {
      gConnectConInEvent = NULL;
    }
  }

  //
  // Do the platform init, can be customized by OEM/IBV
  // Possible things that can be done in PlatformBootManagerBeforeConsole:
  // > Update console variable: 1. include hot-plug devices; 2. Clear ConIn and add SOL for AMT
  // > Register new Driver#### or Boot####
  // > Register new Key####: e.g.: F12
  // > Signal ReadyToLock event
  // > Authentication action: 1. connect Auth devices; 2. Identify auto logon user.
  //
  PERF_INMODULE_BEGIN("PlatformBootManagerBeforeConsole");
  PlatformBootManagerBeforeConsole ();
  PERF_INMODULE_END("PlatformBootManagerBeforeConsole");

  //
  // Initialize hotkey service
  //
  EfiBootManagerStartHotkeyService (&HotkeyTriggered);

  //
  // Execute Driver Options
  //
  LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionTypeDriver);
  ProcessLoadOptions (LoadOptions, LoadOptionCount);
  EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);

  //
  // Connect consoles
  //
  PERF_INMODULE_BEGIN("EfiBootManagerConnectAllDefaultConsoles");
  if (PcdGetBool (PcdConInConnectOnDemand)) {
    EfiBootManagerConnectConsoleVariable (ConOut);
    EfiBootManagerConnectConsoleVariable (ErrOut);
    //
    // Do not connect ConIn devices when lazy ConIn feature is ON.
    //
  } else {
    EfiBootManagerConnectAllDefaultConsoles ();
  }
  PERF_INMODULE_END("EfiBootManagerConnectAllDefaultConsoles");

  //
  // Do the platform specific action after the console is ready
  // Possible things that can be done in PlatformBootManagerAfterConsole:
  // > Console post action:
  //   > Dynamically switch output mode from 100x31 to 80x25 for certain senarino
  //   > Signal console ready platform customized event
  // > Run diagnostics like memory testing
  // > Connect certain devices
  // > Dispatch aditional option roms
  // > Special boot: e.g.: USB boot, enter UI
  //
  PERF_INMODULE_BEGIN("PlatformBootManagerAfterConsole");
  PlatformBootManagerAfterConsole ();
  PERF_INMODULE_END("PlatformBootManagerAfterConsole");

  //
  // If any component set PcdTestKeyUsed to TRUE because use of a test key
  // was detected, then display a warning message on the debug log and the console
  //
  if (PcdGetBool (PcdTestKeyUsed)) {
    DEBUG ((DEBUG_ERROR, "**********************************\n"));
    DEBUG ((DEBUG_ERROR, "**  WARNING: Test Key is used.  **\n"));
    DEBUG ((DEBUG_ERROR, "**********************************\n"));
    Print (L"**  WARNING: Test Key is used.  **\n");
  }

  //
  // Boot to Boot Manager Menu when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set. Skip HotkeyBoot
  //
  DataSize = sizeof (UINT64);
  Status = gRT->GetVariable (
                  EFI_OS_INDICATIONS_VARIABLE_NAME,
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &OsIndication
                  );
  if (EFI_ERROR (Status)) {
    OsIndication = 0;
  }

  DEBUG_CODE (
    EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType;
    DEBUG ((EFI_D_INFO, "[Bds]OsIndication: %016x\n", OsIndication));
    DEBUG ((EFI_D_INFO, "[Bds]=============Begin Load Options Dumping ...=============\n"));
    for (LoadOptionType = 0; LoadOptionType < LoadOptionTypeMax; LoadOptionType++) {
      DEBUG ((
        EFI_D_INFO, "  %s Options:\n",
        mBdsLoadOptionName[LoadOptionType]
        ));
      LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionType);
      for (Index = 0; Index < LoadOptionCount; Index++) {
        DEBUG ((
          EFI_D_INFO, "    %s%04x: %s \t\t 0x%04x\n",
          mBdsLoadOptionName[LoadOptionType],
          LoadOptions[Index].OptionNumber,
          LoadOptions[Index].Description,
          LoadOptions[Index].Attributes
          ));
      }
      EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);
    }
    DEBUG ((EFI_D_INFO, "[Bds]=============End Load Options Dumping=============\n"));
  );

  //
  // BootManagerMenu doesn't contain the correct information when return status is EFI_NOT_FOUND.
  //
  BootManagerMenuStatus = EfiBootManagerGetBootManagerMenu (&BootManagerMenu);

  BootFwUi         = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) != 0);
  PlatformRecovery = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY) != 0);
  //
  // Clear EFI_OS_INDICATIONS_BOOT_TO_FW_UI to acknowledge OS
  //
  if (BootFwUi || PlatformRecovery) {
    OsIndication &= ~((UINT64) (EFI_OS_INDICATIONS_BOOT_TO_FW_UI | EFI_OS_INDICATIONS_START_PLATFORM_RECOVERY));
    Status = gRT->SetVariable (
               EFI_OS_INDICATIONS_VARIABLE_NAME,
               &gEfiGlobalVariableGuid,
               EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
               sizeof(UINT64),
               &OsIndication
               );
    //
    // Changing the content without increasing its size with current variable implementation shouldn't fail.
    //
    ASSERT_EFI_ERROR (Status);
  }

  //
  // Launch Boot Manager Menu directly when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set. Skip HotkeyBoot
  //
  if (BootFwUi && (BootManagerMenuStatus != EFI_NOT_FOUND)) {
    //
    // Follow generic rule, Call BdsDxeOnConnectConInCallBack to connect ConIn before enter UI
    //
    if (PcdGetBool (PcdConInConnectOnDemand)) {
      BdsDxeOnConnectConInCallBack (NULL, NULL);
    }

    //
    // Directly enter the setup page.
    //
    EfiBootManagerBoot (&BootManagerMenu);
  }

  if (!PlatformRecovery) {
    //
    // Execute SysPrep####
    //
    LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionTypeSysPrep);
    ProcessLoadOptions (LoadOptions, LoadOptionCount);
    EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);

    //
    // Execute Key####
    //
    PERF_INMODULE_BEGIN ("BdsWait");
    BdsWait (HotkeyTriggered);
    PERF_INMODULE_END ("BdsWait");
    //
    // BdsReadKeys() can be removed after all keyboard drivers invoke callback in timer callback.
    //
    BdsReadKeys ();

    EfiBootManagerHotkeyBoot ();

    if (BootNext != NULL) {
      //
      // Delete "BootNext" NV variable before transferring control to it to prevent loops.
      //
      Status = gRT->SetVariable (
                      EFI_BOOT_NEXT_VARIABLE_NAME,
                      &gEfiGlobalVariableGuid,
                      0,
                      0,
                      NULL
                      );
      //
      // Deleting NV variable shouldn't fail unless it doesn't exist.
      //
      ASSERT (Status == EFI_SUCCESS || Status == EFI_NOT_FOUND);

      //
      // Boot to "BootNext"
      //
      UnicodeSPrint (BootNextVariableName, sizeof (BootNextVariableName), L"Boot%04x", *BootNext);
      Status = EfiBootManagerVariableToLoadOption (BootNextVariableName, &LoadOption);
      if (!EFI_ERROR (Status)) {
        EfiBootManagerBoot (&LoadOption);
        EfiBootManagerFreeLoadOption (&LoadOption);
        if ((LoadOption.Status == EFI_SUCCESS) &&
            (BootManagerMenuStatus != EFI_NOT_FOUND) &&
            (LoadOption.OptionNumber != BootManagerMenu.OptionNumber)) {
          //
          // Boot to Boot Manager Menu upon EFI_SUCCESS
          // Exception: Do not boot again when the BootNext points to Boot Manager Menu.
          //
          EfiBootManagerBoot (&BootManagerMenu);
        }
      }
    }

    do {
      //
      // Retry to boot if any of the boot succeeds
      //
      LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionTypeBoot);
      BootSuccess = BootBootOptions (LoadOptions, LoadOptionCount, (BootManagerMenuStatus != EFI_NOT_FOUND) ? &BootManagerMenu : NULL);
      EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);
    } while (BootSuccess);
  }

  if (BootManagerMenuStatus != EFI_NOT_FOUND) {
    EfiBootManagerFreeLoadOption (&BootManagerMenu);
  }

  if (!BootSuccess) {
    if (PcdGetBool (PcdPlatformRecoverySupport)) {
      LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionTypePlatformRecovery);
      ProcessLoadOptions (LoadOptions, LoadOptionCount);
      EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);
    } else {
      //
      // When platform recovery is not enabled, still boot to platform default file path.
      //
      EfiBootManagerProcessLoadOption (&PlatformDefaultBootOption);
    }
  }
  EfiBootManagerFreeLoadOption (&PlatformDefaultBootOption);

  DEBUG ((EFI_D_ERROR, "[Bds] Unable to boot!\n"));
  PlatformBootManagerUnableToBoot ();
  CpuDeadLoop ();
}

/**
  Set the variable and report the error through status code upon failure.

  @param  VariableName           A Null-terminated string that is the name of the vendor's variable.
                                 Each VariableName is unique for each VendorGuid. VariableName must
                                 contain 1 or more characters. If VariableName is an empty string,
                                 then EFI_INVALID_PARAMETER is returned.
  @param  VendorGuid             A unique identifier for the vendor.
  @param  Attributes             Attributes bitmask to set for the variable.
  @param  DataSize               The size in bytes of the Data buffer. Unless the EFI_VARIABLE_APPEND_WRITE,
                                 or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero
                                 causes the variable to be deleted. When the EFI_VARIABLE_APPEND_WRITE attribute is
                                 set, then a SetVariable() call with a DataSize of zero will not cause any change to
                                 the variable value (the timestamp associated with the variable may be updated however
                                 even if no new data value is provided,see the description of the
                                 EFI_VARIABLE_AUTHENTICATION_2 descriptor below. In this case the DataSize will not
                                 be zero since the EFI_VARIABLE_AUTHENTICATION_2 descriptor will be populated).
  @param  Data                   The contents for the variable.

  @retval EFI_SUCCESS            The firmware has successfully stored the variable and its data as
                                 defined by the Attributes.
  @retval EFI_INVALID_PARAMETER  An invalid combination of attribute bits, name, and GUID was supplied, or the
                                 DataSize exceeds the maximum allowed.
  @retval EFI_INVALID_PARAMETER  VariableName is an empty string.
  @retval EFI_OUT_OF_RESOURCES   Not enough storage is available to hold the variable and its data.
  @retval EFI_DEVICE_ERROR       The variable could not be retrieved due to a hardware error.
  @retval EFI_WRITE_PROTECTED    The variable in question is read-only.
  @retval EFI_WRITE_PROTECTED    The variable in question cannot be deleted.
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS
                                 being set, but the AuthInfo does NOT pass the validation check carried out by the firmware.

  @retval EFI_NOT_FOUND          The variable trying to be updated or deleted was not found.
**/
EFI_STATUS
BdsDxeSetVariableAndReportStatusCodeOnError (
  IN CHAR16     *VariableName,
  IN EFI_GUID   *VendorGuid,
  IN UINT32     Attributes,
  IN UINTN      DataSize,
  IN VOID       *Data
  )
{
  EFI_STATUS                 Status;
  EDKII_SET_VARIABLE_STATUS  *SetVariableStatus;
  UINTN                      NameSize;

  Status = gRT->SetVariable (
                  VariableName,
                  VendorGuid,
                  Attributes,
                  DataSize,
                  Data
                  );
  if (EFI_ERROR (Status)) {
    NameSize = StrSize (VariableName);
    SetVariableStatus = AllocatePool (sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + DataSize);
    if (SetVariableStatus != NULL) {
      CopyGuid (&SetVariableStatus->Guid, VendorGuid);
      SetVariableStatus->NameSize   = NameSize;
      SetVariableStatus->DataSize   = DataSize;
      SetVariableStatus->SetStatus  = Status;
      SetVariableStatus->Attributes = Attributes;
      CopyMem (SetVariableStatus + 1,                          VariableName, NameSize);
      CopyMem (((UINT8 *) (SetVariableStatus + 1)) + NameSize, Data,         DataSize);

      REPORT_STATUS_CODE_EX (
        EFI_ERROR_CODE,
        PcdGet32 (PcdErrorCodeSetVariable),
        0,
        NULL,
        &gEdkiiStatusCodeDataTypeVariableGuid,
        SetVariableStatus,
        sizeof (EDKII_SET_VARIABLE_STATUS) + NameSize + DataSize
        );

      FreePool (SetVariableStatus);
    }
  }

  return Status;
}
