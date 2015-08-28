/** @file
  This module produce main entry for BDS phase - BdsEntry.
  When this module was dispatched by DxeCore, gEfiBdsArchProtocolGuid will be installed
  which contains interface of BdsEntry.
  After DxeCore finish DXE phase, gEfiBdsArchProtocolGuid->BdsEntry will be invoked
  to enter BDS phase.

(C) Copyright 2015 Hewlett-Packard Development Company, L.P.<BR>
Copyright (c) 2004 - 2015, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Bds.h"
#include "Language.h"
#include "HwErrRecSupport.h"

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
  L"Boot"
};

CHAR16  mRecoveryBoot[] = L"Recovery Boot";
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

  return Status;
}

/**
  Emuerate all possible bootable medias in the following order:
  1. Removable BlockIo            - The boot option only points to the removable media
                                    device, like USB key, DVD, Floppy etc.
  2. Fixed BlockIo                - The boot option only points to a Fixed blockIo device,
                                    like HardDisk.
  3. Non-BlockIo SimpleFileSystem - The boot option points to a device supporting
                                    SimpleFileSystem Protocol, but not supporting BlockIo
                                    protocol.
  4. LoadFile                     - The boot option points to the media supporting 
                                    LoadFile protocol.
  Reference: UEFI Spec chapter 3.3 Boot Option Variables Default Boot Behavior

  @param BootOptionCount   Return the boot option count which has been found.

  @retval   Pointer to the boot option array.
**/
EFI_BOOT_MANAGER_LOAD_OPTION *
BdsEnumerateBootOptions (
  UINTN                                 *BootOptionCount
  )
{
  EFI_STATUS                            Status;
  EFI_BOOT_MANAGER_LOAD_OPTION          *BootOptions;
  UINTN                                 HandleCount;
  EFI_HANDLE                            *Handles;
  EFI_BLOCK_IO_PROTOCOL                 *BlkIo;
  UINTN                                 Removable;
  UINTN                                 Index;

  ASSERT (BootOptionCount != NULL);

  *BootOptionCount = 0;
  BootOptions      = NULL;

  //
  // Parse removable block io followed by fixed block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiBlockIoProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );

  for (Removable = 0; Removable < 2; Removable++) {
    for (Index = 0; Index < HandleCount; Index++) {
      Status = gBS->HandleProtocol (
                      Handles[Index],
                      &gEfiBlockIoProtocolGuid,
                      (VOID **) &BlkIo
                      );
      if (EFI_ERROR (Status)) {
        continue;
      }

      //
      // Skip the logical partitions
      //
      if (BlkIo->Media->LogicalPartition) {
        continue;
      }

      //
      // Skip the fixed block io then the removable block io
      //
      if (BlkIo->Media->RemovableMedia == ((Removable == 0) ? FALSE : TRUE)) {
        continue;
      }

      BootOptions = ReallocatePool (
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                      sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                      BootOptions
                      );
      ASSERT (BootOptions != NULL);

      Status = EfiBootManagerInitializeLoadOption (
                 &BootOptions[(*BootOptionCount)++],
                 LoadOptionNumberUnassigned,
                 LoadOptionTypeBoot,
                 LOAD_OPTION_ACTIVE,
                 mRecoveryBoot,
                 DevicePathFromHandle (Handles[Index]),
                 NULL,
                 0
                 );
      ASSERT_EFI_ERROR (Status);
    }
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse simple file system not based on block io
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiSimpleFileSystemProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {
    Status = gBS->HandleProtocol (
                    Handles[Index],
                    &gEfiBlockIoProtocolGuid,
                    (VOID **) &BlkIo
                    );
     if (!EFI_ERROR (Status)) {
      //
      //  Skip if the file system handle supports a BlkIo protocol, which we've handled in above
      //
      continue;
    }
    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               mRecoveryBoot,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  //
  // Parse load file, assuming UEFI Network boot option
  //
  gBS->LocateHandleBuffer (
         ByProtocol,
         &gEfiLoadFileProtocolGuid,
         NULL,
         &HandleCount,
         &Handles
         );
  for (Index = 0; Index < HandleCount; Index++) {

    BootOptions = ReallocatePool (
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount),
                    sizeof (EFI_BOOT_MANAGER_LOAD_OPTION) * (*BootOptionCount + 1),
                    BootOptions
                    );
    ASSERT (BootOptions != NULL);

    Status = EfiBootManagerInitializeLoadOption (
               &BootOptions[(*BootOptionCount)++],
               LoadOptionNumberUnassigned,
               LoadOptionTypeBoot,
               LOAD_OPTION_ACTIVE,
               mRecoveryBoot,
               DevicePathFromHandle (Handles[Index]),
               NULL,
               0
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (HandleCount != 0) {
    FreePool (Handles);
  }

  return BootOptions;
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
  DEBUG ((EFI_D_INFO, "[Bds]Exit the waiting!\n"));
}

/**
  Attempt to boot each boot option in the BootOptions array.

  @param BootOptions       Input boot option array.
  @param BootOptionCount   Input boot option count.

  @retval TRUE  Successfully boot one of the boot options.
  @retval FALSE Failed boot any of the boot options.
**/
BOOLEAN
BootAllBootOptions (
  IN EFI_BOOT_MANAGER_LOAD_OPTION    *BootOptions,
  IN UINTN                           BootOptionCount
  )
{
  UINTN                              Index;

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
    // Successful boot breaks the loop, otherwise tries next boot option
    //
    if (BootOptions[Index].Status == EFI_SUCCESS) {
      break;
    }
  }

  return (BOOLEAN) (Index < BootOptionCount);
}

/**
  This function attempts to boot per the boot order specified by platform policy.

  If the boot via Boot#### returns with a status of EFI_SUCCESS the boot manager will stop 
  processing the BootOrder variable and present a boot manager menu to the user. If a boot via 
  Boot#### returns a status other than EFI_SUCCESS, the boot has failed and the next Boot####
  in the BootOrder variable will be tried until all possibilities are exhausted.
                                  -- Chapter 3.1.1 Boot Manager Programming, the 4th paragraph
**/
VOID
DefaultBootBehavior (
  VOID
  )
{
  UINTN                        BootOptionCount;
  EFI_BOOT_MANAGER_LOAD_OPTION *BootOptions;
  EFI_BOOT_MANAGER_LOAD_OPTION BootManagerMenu;

  EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
  //
  // BootManagerMenu always contains the correct information even the above function returns failure.
  //

  BootOptions = EfiBootManagerGetLoadOptions (&BootOptionCount, LoadOptionTypeBoot);

  if (BootAllBootOptions (BootOptions, BootOptionCount)) {
    //
    // Follow generic rule, Call BdsDxeOnConnectConInCallBack to connect ConIn before enter UI
    //
    if (PcdGetBool (PcdConInConnectOnDemand)) {
      BdsDxeOnConnectConInCallBack (NULL, NULL);
    }

    //
    // Show the Boot Manager Menu after successful boot
    //
    EfiBootManagerBoot (&BootManagerMenu);
  } else {
    EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
    //
    // Re-scan all EFI boot options in case all the boot#### are deleted or failed to boot
    //
    // If no valid boot options exist, the boot manager will enumerate all removable media
    // devices followed by all fixed media devices. The order within each group is undefined.
    // These new default boot options are not saved to non volatile storage.The boot manger
    // will then attempt toboot from each boot option.
    //                            -- Chapter 3.3 Boot Manager Programming, the 2nd paragraph
    //
    EfiBootManagerConnectAll ();
    BootOptions = BdsEnumerateBootOptions (&BootOptionCount);

    if (!BootAllBootOptions (BootOptions, BootOptionCount)) {
      DEBUG ((EFI_D_ERROR, "[Bds]No bootable device!\n"));
      EfiBootManagerBoot (&BootManagerMenu);
    }
  }

  EfiBootManagerFreeLoadOption (&BootManagerMenu);
  EfiBootManagerFreeLoadOptions (BootOptions, BootOptionCount);
}

/**
  The function will load and start every Driver####/SysPrep####.

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

  ReconnectAll = FALSE;
  LoadOptionType = LoadOptionTypeMax;

  //
  // Process the driver option
  //
  for (Index = 0; Index < LoadOptionCount; Index++) {
    //
    // All the load options in the array should be of the same type.
    //
    if (LoadOptionType == LoadOptionTypeMax) {
      LoadOptionType = LoadOptions[Index].OptionType;
    }
    ASSERT (LoadOptionType == LoadOptions[Index].OptionType);
    ASSERT (LoadOptionType == LoadOptionTypeDriver || LoadOptionType == LoadOptionTypeSysPrep);

    Status = EfiBootManagerProcessLoadOption (&LoadOptions[Index]);

    if (!EFI_ERROR (Status) && ((LoadOptions[Index].Attributes & LOAD_OPTION_FORCE_RECONNECT) != 0)) {
      ReconnectAll = TRUE;
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

**/
VOID 
BdsFormalizeOSIndicationVariable (
  VOID
  )
{
  EFI_STATUS Status;
  UINT64     OsIndicationSupport;
  UINT64     OsIndication;
  UINTN      DataSize;
  UINT32     Attributes;

  //
  // OS indicater support variable
  //
  OsIndicationSupport = EFI_OS_INDICATIONS_BOOT_TO_FW_UI;
  Status = gRT->SetVariable (
                  L"OsIndicationsSupported",
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
                  L"OsIndications",
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
                    L"OsIndications",
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
  BdsFormalizeConsoleVariable (L"ConIn");
  BdsFormalizeConsoleVariable (L"ConOut");
  BdsFormalizeConsoleVariable (L"ErrOut");

  //
  // Validate OSIndication related variable.
  //
  BdsFormalizeOSIndicationVariable ();
}

/**

  Allocate a block of memory that will contain performance data to OS.

**/
VOID
BdsAllocateMemoryForPerformanceData (
  VOID
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          AcpiLowMemoryBase;
  EDKII_VARIABLE_LOCK_PROTOCOL  *VariableLock;

  AcpiLowMemoryBase = 0x0FFFFFFFFULL;

  //
  // Allocate a block of memory that will contain performance data to OS.
  //
  Status = gBS->AllocatePages (
                  AllocateMaxAddress,
                  EfiReservedMemoryType,
                  EFI_SIZE_TO_PAGES (PERF_DATA_MAX_LENGTH),
                  &AcpiLowMemoryBase
                  );
  if (!EFI_ERROR (Status)) {
    //
    // Save the pointer to variable for use in S3 resume.
    //
    Status = BdsDxeSetVariableAndReportStatusCodeOnError (
               L"PerfDataMemAddr",
               &gPerformanceProtocolGuid,
               EFI_VARIABLE_NON_VOLATILE | EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
               sizeof (EFI_PHYSICAL_ADDRESS),
               &AcpiLowMemoryBase
               );
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "[Bds] PerfDataMemAddr (%08x) cannot be saved to NV storage.\n", AcpiLowMemoryBase));
    }
    //
    // Mark L"PerfDataMemAddr" variable to read-only if the Variable Lock protocol exists
    // Still lock it even the variable cannot be saved to prevent it's set by 3rd party code.
    //
    Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
    if (!EFI_ERROR (Status)) {
      Status = VariableLock->RequestToLock (VariableLock, L"PerfDataMemAddr", &gPerformanceProtocolGuid);
      ASSERT_EFI_ERROR (Status);
    }
  }
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
  EDKII_VARIABLE_LOCK_PROTOCOL    *VariableLock;
  UINTN                           Index;
  EFI_BOOT_MANAGER_LOAD_OPTION    BootOption;
  UINT16                          *BootNext;
  CHAR16                          BootNextVariableName[sizeof ("Boot####")];
  EFI_BOOT_MANAGER_LOAD_OPTION    BootManagerMenu;
  BOOLEAN                         BootFwUi;

  HotkeyTriggered = NULL;
  Status          = EFI_SUCCESS;

  //
  // Insert the performance probe
  //
  PERF_END (NULL, "DXE", NULL, 0);
  PERF_START (NULL, "BDS", NULL, 0);
  DEBUG ((EFI_D_INFO, "[Bds] Entry...\n"));

  PERF_CODE (
    BdsAllocateMemoryForPerformanceData ();
  );

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
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
  DEBUG ((EFI_D_INFO, "[BdsDxe] Locate Variable Lock protocol - %r\n", Status));
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < sizeof (mReadOnlyVariables) / sizeof (mReadOnlyVariables[0]); Index++) {
      Status = VariableLock->RequestToLock (VariableLock, mReadOnlyVariables[Index], &gEfiGlobalVariableGuid);
      ASSERT_EFI_ERROR (Status);
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
  // Cache and remove the "BootNext" NV variable.
  //
  GetEfiGlobalVariable2 (EFI_BOOT_NEXT_VARIABLE_NAME, (VOID **) &BootNext, &DataSize);
  if (DataSize != sizeof (UINT16)) {
    if (BootNext != NULL) {
      FreePool (BootNext);
    }
    BootNext = NULL;
  }
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
  // Initialize the platform language variables
  //
  InitializeLanguage (TRUE);

  //
  // Report Status Code to indicate connecting drivers will happen
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
    );

  //
  // Do the platform init, can be customized by OEM/IBV
  // Possible things that can be done in PlatformBootManagerBeforeConsole:
  // > Update console variable: 1. include hot-plug devices; 2. Clear ConIn and add SOL for AMT
  // > Register new Driver#### or Boot####
  // > Register new Key####: e.g.: F12 
  // > Signal ReadyToLock event
  // > Authentication action: 1. connect Auth devices; 2. Identify auto logon user.
  //
  PERF_START (NULL, "PlatformBootManagerBeforeConsole", "BDS", 0);
  PlatformBootManagerBeforeConsole ();
  PERF_END   (NULL, "PlatformBootManagerBeforeConsole", "BDS", 0);

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
  PERF_START (NULL, "EfiBootManagerConnectAllDefaultConsoles", "BDS", 0);
  if (PcdGetBool (PcdConInConnectOnDemand)) {
    EfiBootManagerConnectConsoleVariable (ConOut);
    EfiBootManagerConnectConsoleVariable (ErrOut);

    //
    // Initialize ConnectConIn event
    //
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
  } else {
    EfiBootManagerConnectAllDefaultConsoles ();
  }
  PERF_END   (NULL, "EfiBootManagerConnectAllDefaultConsoles", "BDS", 0);

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
  PERF_START (NULL, "PlatformBootManagerAfterConsole", "BDS", 0);
  PlatformBootManagerAfterConsole ();
  PERF_END   (NULL, "PlatformBootManagerAfterConsole", "BDS", 0);

  DEBUG_CODE (
    EFI_BOOT_MANAGER_LOAD_OPTION_TYPE LoadOptionType;
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
  // Boot to Boot Manager Menu when EFI_OS_INDICATIONS_BOOT_TO_FW_UI is set. Skip HotkeyBoot
  //
  DataSize = sizeof (UINT64);
  Status = gRT->GetVariable (
                  L"OsIndications",
                  &gEfiGlobalVariableGuid,
                  NULL,
                  &DataSize,
                  &OsIndication
                  );
  if (EFI_ERROR (Status)) {
    OsIndication = 0;
  }

  BootFwUi = (BOOLEAN) ((OsIndication & EFI_OS_INDICATIONS_BOOT_TO_FW_UI) != 0);
  //
  // Clear EFI_OS_INDICATIONS_BOOT_TO_FW_UI to acknowledge OS
  // 
  if (BootFwUi) {
    OsIndication &= ~((UINT64) EFI_OS_INDICATIONS_BOOT_TO_FW_UI);
    Status = gRT->SetVariable (
               L"OsIndications",
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
  if (BootFwUi) {
    //
    // Follow generic rule, Call BdsDxeOnConnectConInCallBack to connect ConIn before enter UI
    //
    if (PcdGetBool (PcdConInConnectOnDemand)) {
      BdsDxeOnConnectConInCallBack (NULL, NULL);
    }

    //
    // Directly enter the setup page.
    // BootManagerMenu always contains the correct information even call fails.
    //
    EfiBootManagerGetBootManagerMenu (&BootManagerMenu);
    EfiBootManagerBoot (&BootManagerMenu);
    EfiBootManagerFreeLoadOption (&BootManagerMenu);
  }

  //
  // Execute SysPrep####
  //
  LoadOptions = EfiBootManagerGetLoadOptions (&LoadOptionCount, LoadOptionTypeSysPrep);
  ProcessLoadOptions (LoadOptions, LoadOptionCount);
  EfiBootManagerFreeLoadOptions (LoadOptions, LoadOptionCount);

  //
  // Execute Key####
  //
  PERF_START (NULL, "BdsWait", "BDS", 0);
  BdsWait (HotkeyTriggered);
  PERF_END   (NULL, "BdsWait", "BDS", 0);

  //
  // BdsReadKeys() be removed after all keyboard drivers invoke callback in timer callback.
  //
  BdsReadKeys ();

  EfiBootManagerHotkeyBoot ();

  //
  // Boot to "BootNext"
  //
  if (BootNext != NULL) {
    UnicodeSPrint (BootNextVariableName, sizeof (BootNextVariableName), L"Boot%04x", *BootNext);
    Status = EfiBootManagerVariableToLoadOption (BootNextVariableName, &BootOption);
    if (!EFI_ERROR (Status)) {
      EfiBootManagerBoot (&BootOption);
      EfiBootManagerFreeLoadOption (&BootOption);
      if (BootOption.Status == EFI_SUCCESS) {
        //
        // Boot to Boot Manager Menu upon EFI_SUCCESS
        //
        EfiBootManagerGetBootManagerMenu (&BootOption);
        EfiBootManagerBoot (&BootOption);
        EfiBootManagerFreeLoadOption (&BootOption);
      }
    }
  }

  while (TRUE) {
    //
    // BDS select the boot device to load OS
    // Try next upon boot failure
    // Show Boot Manager Menu upon boot success
    //
    DefaultBootBehavior ();
  }
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
                                 EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS, or 
                                 EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACCESS attribute is set, a size of zero 
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
  @retval EFI_SECURITY_VIOLATION The variable could not be written due to EFI_VARIABLE_AUTHENTICATED_WRITE_ACCESS 
                                 or EFI_VARIABLE_TIME_BASED_AUTHENTICATED_WRITE_ACESS being set, but the AuthInfo 
                                 does NOT pass the validation check carried out by the firmware.

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
