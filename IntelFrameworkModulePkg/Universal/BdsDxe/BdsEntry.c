/** @file
  This module produce main entry for BDS phase - BdsEntry.
  When this module was dispatched by DxeCore, gEfiBdsArchProtocolGuid will be installed
  which contains interface of BdsEntry.
  After DxeCore finish DXE phase, gEfiBdsArchProtocolGuid->BdsEntry will be invoked
  to enter BDS phase.

Copyright (c) 2004 - 2018, Intel Corporation. All rights reserved.<BR>
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

///
/// The read-only variables defined in UEFI Spec.
///
CHAR16  *mReadOnlyVariables[] = {
  L"PlatformLangCodes",
  L"LangCodes",
  L"BootOptionSupport",
  L"HwErrRecSupport",
  L"OsIndicationsSupported"
  };

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
  EFI_EVENT         ConnectConInEvent;

  //
  // Got the latest boot option
  //
  BootNextExist = FALSE;
  LinkBootNext  = NULL;
  ConnectConInEvent = NULL;
  InitializeListHead (&BootLists);

  //
  // First check the boot next option
  //
  ZeroMem (Buffer, sizeof (Buffer));

  //
  // Create Event to signal ConIn connection request
  //
  if (PcdGetBool (PcdConInConnectOnDemand)) {
    Status = gBS->CreateEventEx (
                    EVT_NOTIFY_SIGNAL,
                    TPL_CALLBACK,
                    EfiEventEmptyFunction,
                    NULL,
                    &gConnectConInEventGuid,
                    &ConnectConInEvent
                    );
    if (EFI_ERROR(Status)) {
      ConnectConInEvent = NULL;
    }
  }

  if (mBootNext != NULL) {
    //
    // Indicate we have the boot next variable, so this time
    // boot will always have this boot option
    //
    BootNextExist = TRUE;

    //
    // Clear the this variable so it's only exist in this time boot
    //
    Status = gRT->SetVariable (
                    L"BootNext",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    0,
                    NULL
                    );
    //
    // Deleting variable with current variable implementation shouldn't fail.
    //
    ASSERT_EFI_ERROR (Status);

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
      // When LazyConIn enabled, signal connect ConIn event before enter UI
      //
      if (PcdGetBool (PcdConInConnectOnDemand) && ConnectConInEvent != NULL) {
        gBS->SignalEvent (ConnectConInEvent);
      }

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

      //
      // When LazyConIn enabled, signal connect ConIn Event before enter UI
      //
      if (PcdGetBool (PcdConInConnectOnDemand) && ConnectConInEvent != NULL) {
        gBS->SignalEvent (ConnectConInEvent);
      }

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

  DevicePath = BdsLibGetVariableAndSize (
                      VariableName,
                      &gEfiGlobalVariableGuid,
                      &VariableSize
                      );
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
}

/**

  Formalize Bds global variables. 

 1. For ConIn/ConOut/ConErr, if found the device path is not a valid device path, remove the variable.
 2. For OsIndicationsSupported, Create a BS/RT/UINT64 variable to report caps 
 3. Delete OsIndications variable if it is not NV/BS/RT UINT64
 Item 3 is used to solve case when OS corrupts OsIndications. Here simply delete this NV variable.
 
**/
VOID 
BdsFormalizeEfiGlobalVariable (
  VOID
  )
{
  EFI_STATUS Status;
  UINT64     OsIndicationSupport;
  UINT64     OsIndication;
  UINTN      DataSize;
  UINT32     Attributes;
  
  //
  // Validate Console variable.
  //
  BdsFormalizeConsoleVariable (L"ConIn");
  BdsFormalizeConsoleVariable (L"ConOut");
  BdsFormalizeConsoleVariable (L"ErrOut");

  //
  // OS indicater support variable
  //
  OsIndicationSupport = EFI_OS_INDICATIONS_BOOT_TO_FW_UI \
                      | EFI_OS_INDICATIONS_FMP_CAPSULE_SUPPORTED;

  BdsDxeSetVariableAndReportStatusCodeOnError (
    L"OsIndicationsSupported",
    &gEfiGlobalVariableGuid,
    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS,
    sizeof(UINT64),
    &OsIndicationSupport
    );

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

  if (!EFI_ERROR(Status)) {
    if (DataSize != sizeof(UINT64) ||
        (OsIndication & ~OsIndicationSupport) != 0 ||
        Attributes != (EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE)){

      DEBUG ((EFI_D_ERROR, "Unformalized OsIndications variable exists. Delete it\n"));
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
  LIST_ENTRY                      DriverOptionList;
  LIST_ENTRY                      BootOptionList;
  UINTN                           BootNextSize;
  CHAR16                          *FirmwareVendor;
  EFI_STATUS                      Status;
  UINT16                          BootTimeOut;
  UINTN                           Index;
  EDKII_VARIABLE_LOCK_PROTOCOL    *VariableLock;

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
  gST->Hdr.CRC32 = 0;
  gBS->CalculateCrc32 ((VOID *)gST, sizeof(EFI_SYSTEM_TABLE), &gST->Hdr.CRC32);

  //
  // Validate Variable.
  //
  BdsFormalizeEfiGlobalVariable();

  //
  // Mark the read-only variables if the Variable Lock protocol exists
  //
  Status = gBS->LocateProtocol (&gEdkiiVariableLockProtocolGuid, NULL, (VOID **) &VariableLock);
  DEBUG ((EFI_D_INFO, "[BdsDxe] Locate Variable Lock protocol - %r\n", Status));
  if (!EFI_ERROR (Status)) {
    for (Index = 0; Index < ARRAY_SIZE (mReadOnlyVariables); Index++) {
      Status = VariableLock->RequestToLock (VariableLock, mReadOnlyVariables[Index], &gEfiGlobalVariableGuid);
      ASSERT_EFI_ERROR (Status);
    }
  }

  //
  // Report Status Code to indicate connecting drivers will happen
  //
  REPORT_STATUS_CODE (
    EFI_PROGRESS_CODE,
    (EFI_SOFTWARE_DXE_BS_DRIVER | EFI_SW_DXE_BS_PC_BEGIN_CONNECTING_DRIVERS)
    );

  InitializeHwErrRecSupport();

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
                    L"Timeout",
                    &gEfiGlobalVariableGuid,
                    EFI_VARIABLE_BOOTSERVICE_ACCESS | EFI_VARIABLE_RUNTIME_ACCESS | EFI_VARIABLE_NON_VOLATILE,
                    sizeof (UINT16),
                    &BootTimeOut
                    );
  }

  //
  // bugbug: platform specific code
  // Initialize the platform specific string and language
  //
  InitializeStringSupport ();
  InitializeLanguage (TRUE);
  InitializeFrontPage (TRUE);

  //
  // Do the platform init, can be customized by OEM/IBV
  //
  PERF_START (NULL, "PlatformBds", "BDS", 0);
  PlatformBdsInit ();

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

