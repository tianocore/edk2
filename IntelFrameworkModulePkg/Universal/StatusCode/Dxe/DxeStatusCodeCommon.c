/** @file
  Status code driver for IA32/X64/EBC architecture.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "DxeStatusCode.h"

EFI_EVENT    mVirtualAddressChangeEvent = NULL;
EFI_HANDLE   mHandle = NULL;

//
// Declaration of status code protocol.
//
EFI_STATUS_CODE_PROTOCOL  mEfiStatusCodeProtocol  = {
  ReportDispatcher
};

//
// Report operation nest status.
// If it is set, then the report operation has nested.
//
UINT32  mStatusCodeNestStatus = 0;

/**
  Entry point of DXE Status Code Driver.

  This function is the entry point of this DXE Status Code Driver.
  It installs Status Code Runtime Protocol, and registers event for EVT_SIGNAL_VIRTUAL_ADDRESS_CHANGE.

  @param  ImageHandle       The firmware allocated handle for the EFI image.
  @param  SystemTable       A pointer to the EFI System Table.
  
  @retval EFI_SUCCESS       The entry point is executed successfully.

**/
EFI_STATUS
EFIAPI
DxeStatusCodeDriverEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Dispatch initialization request to supported devices
  //
  InitializationDispatcherWorker ();

  //
  // Install Status Code Runtime Protocol implementation as defined in PI Specification.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mHandle,
                  &gEfiStatusCodeRuntimeProtocolGuid,
                  &mEfiStatusCodeProtocol,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  Status = gBS->CreateEventEx (
                  EVT_NOTIFY_SIGNAL,
                  TPL_NOTIFY,
                  VirtualAddressChangeCallBack,
                  NULL,
                  &gEfiEventVirtualAddressChangeGuid,
                  &mVirtualAddressChangeEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Report status code to all supported device.

  This function implements EFI_STATUS_CODE_PROTOCOL.ReportStatusCode().
  It calls into the workers which dispatches the platform specific listeners.

  @param  CodeType         Indicates the type of status code being reported.
  @param  Value            Describes the current status of a hardware or software entity.
                           This included information about the class and subclass that is used to
                           classify the entity as well as an operation.
  @param  Instance         The enumeration of a hardware or software entity within
                           the system. Valid instance numbers start with 1.
  @param  CallerId         This optional parameter may be used to identify the caller.
                           This parameter allows the status code driver to apply different rules to
                           different callers.
  @param  Data             This optional parameter may be used to pass additional data.

  @retval EFI_SUCCESS      The function completed successfully
  @retval EFI_DEVICE_ERROR The function should not be completed due to a device error.

**/
EFI_STATUS
EFIAPI
ReportDispatcher (
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  )
{
  //
  // Use atom operation to avoid the reentant of report.
  // If current status is not zero, then the function is reentrancy.
  //
  if (InterlockedCompareExchange32 (&mStatusCodeNestStatus, 0, 1) == 1) {
    return EFI_DEVICE_ERROR;
  }

  if (FeaturePcdGet (PcdStatusCodeUseEfiSerial) || FeaturePcdGet (PcdStatusCodeUseHardSerial)) {
    SerialStatusCodeReportWorker (
      CodeType,
      Value,
      Instance,
      CallerId,
      Data
      );
  }
  if (FeaturePcdGet (PcdStatusCodeUseRuntimeMemory)) {
    RtMemoryStatusCodeReportWorker (
      CodeType,
      Value,
      Instance
      );
  }
  if (FeaturePcdGet (PcdStatusCodeUseDataHub)) {
    DataHubStatusCodeReportWorker (
      CodeType,
      Value,
      Instance,
      CallerId,
      Data
      );
  }
  if (FeaturePcdGet (PcdStatusCodeUseOEM)) {
    //
    // Call OEM hook status code library API to report status code to OEM device
    //
    OemHookStatusCodeReport (
      CodeType,
      Value,
      Instance,
      CallerId,
      Data
      );
  }

  //
  // Restore the nest status of report
  //
  InterlockedCompareExchange32 (&mStatusCodeNestStatus, 1, 0);

  return EFI_SUCCESS;
}


/**
  Virtual address change notification call back. It converts global pointer
  to virtual address.

  @param  Event         Event whose notification function is being invoked.
  @param  Context       Pointer to the notification function's context, which is
                        always zero in current implementation.

**/
VOID
EFIAPI
VirtualAddressChangeCallBack (
  IN EFI_EVENT        Event,
  IN VOID             *Context
  )
{
  //
  // Convert memory status code table to virtual address;
  //
  EfiConvertPointer (
    0,
    (VOID **) &mRtMemoryStatusCodeTable
    );
}

