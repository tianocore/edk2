/** @file
  This module produce main entry for BDS phase - BdsEntry.
  When this module was dispatched by DxeCore, gEfiBdsArchProtocolGuid will be installed
  which contains interface of BdsEntry.
  After DxeCore finish DXE phase, gEfiBdsArchProtocolGuid->BdsEntry will be invoked
  to enter BDS phase.

Copyright (c) 2004 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Bds.h"
#include "Language.h"
#include "FrontPage.h"
#include "Hotkey.h"
#include "HwErrRecSupport.h"

///
/// BDS arch protocol instance initial value.
///
/// Note: Current BDS not directly get the BootMode, DefaultBoot,
/// TimeoutDefault, MemoryTestLevel value from the BDS arch protocol.
/// Please refer to the library useage of BdsLibGetBootMode, BdsLibGetTimeout
/// and PlatformBdsDiagnostics in BdsPlatform.c
///
EFI_HANDLE  gBdsHandle = NULL;

EFI_BDS_ARCH_PROTOCOL  gBds = {
  BdsEntry
};

UINT16                          *mBootNext = NULL;

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

  //
  // Install protocol interface
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &gBdsHandle,
                  &gEfiBdsArchProtocolGuid, &gBds,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**

  This function attempts to boot for the boot order specified
  by platform policy.

**/
VOID
BdsBootDeviceSelect (
  VOID
  )
{
  EFI_STATUS        Status;
  LIST_ENTRY        *Link;
  BDS_COMMON_OPTION *BootOption;
  UINTN             ExitDataSize;
  CHAR16            *ExitData;
  UINT16            Timeout;
  LIST_ENTRY        BootLists;
  CHAR16            Buffer[20];
  BOOLEAN           BootNextExist;
  LIST_ENTRY        *LinkBootNext;

  //
  // Got the latest boot option
  //
  BootNextExist = FALSE;
  LinkBootNext  = NULL;
  InitializeListHead (&BootLists);

  //
  // First check the boot next option
  //
  ZeroMem (Buffer, sizeof (Buffer));

  if (mBootNext != NULL) {
    //
    // Indicate we have the boot next variable, so this time
    // boot will always have this boot option
    //
    BootNextExist = TRUE;

    //
    // Clear the this variable so it's only exist in this time boot
    //
    gRT->SetVariable (
          L"BootNext",
          &gEfiGlobalVariableGuid,
          EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
          0,
          mBootNext
          );

    //
    // Add the boot next boot option
    //
    UnicodeSPrint (Buffer, sizeof (Buffer), L"Boot%04x", *mBootNext);
    BootOption = BdsLibVariableToOption (&BootLists, Buffer);

    //
    // If fail to get boot option from variable, just return and do nothing.
    //
    if (BootOption == NULL) {
      return;
    }

    BootOption->BootCurrent = *mBootNext;
  }
  //
  // Parse the boot order to get boot option
  //
  BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");

  //
  // When we didn't have chance to build boot option variables in the first 
  // full configuration boot (e.g.: Reset in the first page or in Device Manager),
  // we have no boot options in the following mini configuration boot.
  // Give the last chance to enumerate the boot options.
  //
  if (IsListEmpty (&BootLists)) {
    BdsLibEnumerateAllBootOption (&BootLists);
  }

  Link = BootLists.ForwardLink;

  //
  // Parameter check, make sure the loop will be valid
  //
  if (Link == NULL) {
    return ;
  }
  //
  // Here we make the boot in a loop, every boot success will
  // return to the front page
  //
  for (;;) {
    //
    // Check the boot option list first
    //
    if (Link == &BootLists) {
      //
      // There are two ways to enter here:
      // 1. There is no active boot option, give user chance to
      //    add new boot option
      // 2. All the active boot option processed, and there is no
      //    one is success to boot, then we back here to allow user
      //    add new active boot option
      //
      Timeout = 0xffff;
      PlatformBdsEnterFrontPage (Timeout, FALSE);
      InitializeListHead (&BootLists);
      BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");
      Link = BootLists.ForwardLink;
      continue;
    }
    //
    // Get the boot option from the link list
    //
    BootOption = CR (Link, BDS_COMMON_OPTION, Link, BDS_LOAD_OPTION_SIGNATURE);

    //
    // According to EFI Specification, if a load option is not marked
    // as LOAD_OPTION_ACTIVE, the boot manager will not automatically
    // load the option.
    //
    if (!IS_LOAD_OPTION_TYPE (BootOption->Attribute, LOAD_OPTION_ACTIVE)) {
      //
      // skip the header of the link list, because it has no boot option
      //
      Link = Link->ForwardLink;
      continue;
    }
    //
    // Make sure the boot option device path connected,
    // but ignore the BBS device path
    //
    if (DevicePathType (BootOption->DevicePath) != BBS_DEVICE_PATH) {
      //
      // Notes: the internal shell can not been connected with device path
      // so we do not check the status here
      //
      BdsLibConnectDevicePath (BootOption->DevicePath);
    }

    //
    // Restore to original mode before launching boot option.
    //
    BdsSetConsoleMode (FALSE);
    
    //
    // All the driver options should have been processed since
    // now boot will be performed.
    //
    Status = BdsLibBootViaBootOption (BootOption, BootOption->DevicePath, &ExitDataSize, &ExitData);
    if (Status != EFI_SUCCESS) {
      //
      // Call platform action to indicate the boot fail
      //
      BootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_FAILED));
      PlatformBdsBootFail (BootOption, Status, ExitData, ExitDataSize);

      //
      // Check the next boot option
      //
      Link = Link->ForwardLink;

    } else {
      //
      // Call platform action to indicate the boot success
      //
      BootOption->StatusString = GetStringById (STRING_TOKEN (STR_BOOT_SUCCEEDED));
      PlatformBdsBootSuccess (BootOption);

      //
      // Boot success, then stop process the boot order, and
      // present the boot manager menu, front page
      //
      Timeout = 0xffff;
      PlatformBdsEnterFrontPage (Timeout, FALSE);

      //
      // Rescan the boot option list, avoid potential risk of the boot
      // option change in front page
      //
      if (BootNextExist) {
        LinkBootNext = BootLists.ForwardLink;
      }

      InitializeListHead (&BootLists);
      if (LinkBootNext != NULL) {
        //
        // Reserve the boot next option
        //
        InsertTailList (&BootLists, LinkBootNext);
      }

      BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");
      Link = BootLists.ForwardLink;
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
  LIST_ENTRY                      DriverOptionList;
  LIST_ENTRY                      BootOptionList;
  UINTN                           BootNextSize;
  CHAR16                          *FirmwareVendor;

  //
  // Insert the performance probe
  //
  PERF_END (NULL, "DXE", NULL, 0);
  PERF_START (NULL, "BDS", NULL, 0);

  //
  // Initialize the global system boot option and driver option
  //
  InitializeListHead (&DriverOptionList);
  InitializeListHead (&BootOptionList);

  //
  // Initialize hotkey service
  //
  InitializeHotkeyService ();

  //
  // Fill in FirmwareVendor and FirmwareRevision from PCDs
  //
  FirmwareVendor = (CHAR16 *)PcdGetPtr (PcdFirmwareVendor);
  gST->FirmwareVendor = AllocateRuntimeCopyPool (StrSize (FirmwareVendor), FirmwareVendor);
  ASSERT (gST->FirmwareVendor != NULL);
  gST->FirmwareRevision = PcdGet32 (PcdFirmwareRevision);

  //
  // Fixup Tasble CRC after we updated Firmware Vendor and Revision
  //
  gBS->CalculateCrc32 ((VOID *)gST, sizeof(EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);

  //
  // Do the platform init, can be customized by OEM/IBV
  //
  PERF_START (NULL, "PlatformBds", "BDS", 0);
  PlatformBdsInit ();

  InitializeHwErrRecSupport();

  //
  // bugbug: platform specific code
  // Initialize the platform specific string and language
  //
  InitializeStringSupport ();
  InitializeLanguage (TRUE);
  InitializeFrontPage (TRUE);

  //
  // Set up the device list based on EFI 1.1 variables
  // process Driver#### and Load the driver's in the
  // driver option list
  //
  BdsLibBuildOptionFromVar (&DriverOptionList, L"DriverOrder");
  if (!IsListEmpty (&DriverOptionList)) {
    BdsLibLoadDrivers (&DriverOptionList);
  }
  //
  // Check if we have the boot next option
  //
  mBootNext = BdsLibGetVariableAndSize (
                L"BootNext",
                &gEfiGlobalVariableGuid,
                &BootNextSize
                );

  //
  // Setup some platform policy here
  //
  PlatformBdsPolicyBehavior (&DriverOptionList, &BootOptionList, BdsProcessCapsules, BdsMemoryTest);
  PERF_END (NULL, "PlatformBds", "BDS", 0);

  //
  // BDS select the boot device to load OS
  //
  BdsBootDeviceSelect ();

  //
  // Only assert here since this is the right behavior, we should never
  // return back to DxeCore.
  //
  ASSERT (FALSE);

  return ;
}
