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

//
// Event for Exit Boot Services Callback
//
EFI_EVENT mExitBootServicesEvent = NULL;

/**
  Report status code to all supported device.
  Calls into the workers which dispatches the platform specific
  listeners.

  @param  Type          Indicates the type of status code being reported.
                        The type EFI_STATUS_CODE_TYPE is defined in "Related Definitions" below.
  @param  Value         Describes the current status of a hardware or software entity.
                        This includes information about the class and subclass that is used to classify the entity
                        as well as an operation.  For progress codes, the operation is the current activity.
                        For error codes, it is the exception.  For debug codes, it is not defined at this time.
                        Type EFI_STATUS_CODE_VALUE is defined in "Related Definitions" below.
                        Specific values are discussed in the Intel? Platform Innovation Framework for EFI Status Code Specification.
  @param  Instance      The enumeration of a hardware or software entity within the system.
                        A system may contain multiple entities that match a class/subclass pairing.
                        The instance differentiates between them.  An instance of 0 indicates that instance
                        information is unavailable, not meaningful, or not relevant.  Valid instance numbers start with 1.
  @param  CallerId      This optional parameter may be used to identify the caller.
                        This parameter allows the status code driver to apply different rules to different callers.
  @param  Data          This optional parameter may be used to pass additional data.
                        Type EFI_STATUS_CODE_DATA is defined in "Related Definitions" below.
                        The contents of this data type may have additional GUID-specific data.  The standard GUIDs and
                        their associated data structures are defined in the Intel? Platform Innovation Framework for EFI Status Codes Specification.

  @return               Always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
ReportDispatcher (
  IN EFI_STATUS_CODE_TYPE     Type,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN EFI_GUID                 *CallerId  OPTIONAL,
  IN EFI_STATUS_CODE_DATA     *Data      OPTIONAL
  );

//
// Declaration of status code protocol.
//
EFI_STATUS_CODE_PROTOCOL  mEfiStatusCodeProtocol  = {
  ReportDispatcher
};

//
// Delaration of DXE status code controller
//
DXE_STATUS_CODE_CONTROLLER gDxeStatusCode = {
  //
  // Initialize nest status as non nested.
  //
  0,
  {NULL, NULL}
};

/**

  Install the ReportStatusCode runtime service.

  @param ImageHandle     Image handle of the loaded driver
  @param SystemTable     Pointer to the System Table

  @return                The function always returns success.

**/
EFI_STATUS
DxeStatusCodeDriverEntry (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_HANDLE  Handle       = NULL;
  EFI_STATUS  Status;

  //
  // Dispatch initialization request to supported devices
  //
  InitializationDispatcherWorker ();

  //
  // Install Status Code Architectural Protocol implementation as defined in Tiano
  // Architecture Specification.
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &Handle,
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
                  &gEfiEventExitBootServicesGuid,
                  &mExitBootServicesEvent
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}

/**
  Report status code to all supported device.
  Calls into the workers which dispatches the platform specific
  listeners.

  @param  CodeType      Indicates the type of status code being reported.
                        The type EFI_STATUS_CODE_TYPE is defined in "Related Definitions" below.
  @param  Value         Describes the current status of a hardware or software entity.
                        This includes information about the class and subclass that is used to classify the entity
                        as well as an operation.  For progress codes, the operation is the current activity.
                        For error codes, it is the exception.  For debug codes, it is not defined at this time.
                        Type EFI_STATUS_CODE_VALUE is defined in "Related Definitions" below.
                        Specific values are discussed in the Intel? Platform Innovation Framework for EFI Status Code Specification.
  @param  Instance      The enumeration of a hardware or software entity within the system.
                        A system may contain multiple entities that match a class/subclass pairing.
                        The instance differentiates between them.  An instance of 0 indicates that instance
                        information is unavailable, not meaningful, or not relevant.  Valid instance numbers start with 1.
  @param  CallerId      This optional parameter may be used to identify the caller.
                        This parameter allows the status code driver to apply different rules to different callers.
  @param  Data          This optional parameter may be used to pass additional data.
                        Type EFI_STATUS_CODE_DATA is defined in "Related Definitions" below.
                        The contents of this data type may have additional GUID-specific data.  The standard GUIDs and
                        their associated data structures are defined in the Intel? Platform Innovation Framework for EFI Status Codes Specification.

  @return               Always return EFI_SUCCESS.

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
  if (1 == InterlockedCompareExchange32 (&gDxeStatusCode.StatusCodeNestStatus, 0, 1)) {
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
      gDxeStatusCode.RtMemoryStatusCodeTable[PHYSICAL_MODE],
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
  InterlockedCompareExchange32 (&gDxeStatusCode.StatusCodeNestStatus, 1, 0);

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
    (VOID **) &gDxeStatusCode.RtMemoryStatusCodeTable[PHYSICAL_MODE]
    );
}

