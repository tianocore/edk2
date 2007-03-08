/*++

Copyright (c) 2006, Intel Corporation                                                         
All rights reserved. This program and the accompanying materials                          
are licensed and made available under the terms and conditions of the BSD License         
which accompanies this distribution.  The full text of the license may be found at        
http://opensource.org/licenses/bsd-license.php                                            
                                                                                          
THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,                     
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.             

Module Name:

  BdsEntry.c

Abstract:

  The entry of the bds

--*/

#include "Bds.h"
#include "BdsPlatform.h"
#include "FrontPage.h"

EFI_BDS_ARCH_PROTOCOL_INSTANCE  gBdsInstanceTemplate = {
  EFI_BDS_ARCH_PROTOCOL_INSTANCE_SIGNATURE,
  NULL,
  BdsEntry,
  0xFFFF,
  TRUE,
  EXTENSIVE
};

UINT16                          *mBootNext = NULL;

EFI_HANDLE                      mBdsImageHandle;

EFI_STATUS
EFIAPI
BdsInitialize (
  IN EFI_HANDLE                            ImageHandle,
  IN EFI_SYSTEM_TABLE                      *SystemTable
  )
/*++

Routine Description:

  Install Boot Device Selection Protocol

Arguments:
  
  (Standard EFI Image entry - EFI_IMAGE_ENTRY_POINT)

Returns:

  EFI_SUCEESS - BDS has finished initializing.
                Rerun the 
                dispatcher and recall BDS.Entry

  Other       - Return value from EfiLibAllocatePool()
                or gBS->InstallProtocolInterface

--*/
{
  EFI_STATUS  Status;

  mBdsImageHandle = ImageHandle;

  //
  // Install protocol interface
  //
  Status = gBS->InstallProtocolInterface (
                  &gBdsInstanceTemplate.Handle,
                  &gEfiBdsArchProtocolGuid,
                  EFI_NATIVE_INTERFACE,
                  &gBdsInstanceTemplate.Bds
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

VOID
BdsBootDeviceSelect (
  VOID
  )
/*++

Routine Description:

  In the loop of attempt to boot for the boot order

Arguments:
  
  None.

Returns:

  None.
  
--*/
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
  }
  //
  // Parse the boot order to get boot option
  //
  BdsLibBuildOptionFromVar (&BootLists, L"BootOrder");
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
      // skip the header of the link list, becuase it has no boot option
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
    // All the driver options should have been processed since
    // now boot will be performed.
    //
    Status = BdsLibBootViaBootOption (BootOption, BootOption->DevicePath, &ExitDataSize, &ExitData);
    if (EFI_ERROR (Status)) {
      //
      // Call platform action to indicate the boot fail
      //
      PlatformBdsBootFail (BootOption, Status, ExitData, ExitDataSize);

      //
      // Check the next boot option
      //
      Link = Link->ForwardLink;

    } else {
      //
      // Call platform action to indicate the boot success
      //
      PlatformBdsBootSuccess (BootOption);

      //
      // Boot success, then stop process the boot order, and
      // present the boot manager menu, front page
      //
      Timeout = 0xffff;
      PlatformBdsEnterFrontPage (Timeout, FALSE);

      //
      // Rescan the boot option list, avoid pertential risk of the boot
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

  return ;

}

EFI_STATUS
EFIAPI
BdsEntry (
  IN EFI_BDS_ARCH_PROTOCOL  *This
  )
/*++

Routine Description:

  Service routine for BdsInstance->Entry(). Devices are connected, the 
  consoles are initialized, and the boot options are tried. 

Arguments:

  This - Protocol Instance structure.

Returns:

  EFI_SUCEESS - BDS->Entry has finished executing. 
                
--*/
{
  EFI_BDS_ARCH_PROTOCOL_INSTANCE  *PrivateData;
  LIST_ENTRY                      DriverOptionList;
  LIST_ENTRY                      BootOptionList;
  UINTN                           BootNextSize;

  //
  // Insert the performance probe
  //
  PERF_END (0, DXE_TOK, NULL, 0);
  PERF_START (0, BDS_TOK, NULL, 0);

  //
  // Initialize the global system boot option and driver option
  //
  InitializeListHead (&DriverOptionList);
  InitializeListHead (&BootOptionList);

  //
  // Get the BDS private data
  //
  PrivateData = EFI_BDS_ARCH_PROTOCOL_INSTANCE_FROM_THIS (This);

  //
  // Do the platform init, can be customized by OEM/IBV
  //
  PERF_START (0, "PlatformBds", "BDS", 0);
  PlatformBdsInit (PrivateData);

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
  PlatformBdsPolicyBehavior (PrivateData, &DriverOptionList, &BootOptionList);
  PERF_END (0, "PlatformBds", "BDS", 0);

  //
  // BDS select the boot device to load OS
  //
  BdsBootDeviceSelect ();

  //
  // Only assert here since this is the right behavior, we should never
  // return back to DxeCore.
  //
  ASSERT (FALSE);

  return EFI_SUCCESS;
}
