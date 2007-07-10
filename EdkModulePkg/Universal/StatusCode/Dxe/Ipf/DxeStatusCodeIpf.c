/** @file
* Status code driver for IPF architecture.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  DxeStatusCodeIpf.c

**/

#include "DxeStatusCode.h"


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
STATIC
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
STATIC
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
STATIC
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

  Main entry for Extended SAL ReportStatusCode Services

  @param FunctionId        Function Id which needed to be called
  @param Arg2              Efi status code type
  @param Arg3              Efi status code value
  @param Arg4              Instance number
  @param Arg5              Caller Id
  @param Arg6              Efi status code data
  @param Arg7              Not used
  @param Arg8              Not used
  @param ExtendedSalProc   Esal Proc pointer
  @param VirtualMode       If this function is called in virtual mode
  @param Global            This module's global variable pointer

  @return Value returned in SAL_RETURN_REGS

--*/
STATIC
SAL_RETURN_REGS
EFIAPI
ReportEsalServiceEntry (
  IN  UINT64                    FunctionId,
  IN  UINT64                    Arg2,
  IN  UINT64                    Arg3,
  IN  UINT64                    Arg4,
  IN  UINT64                    Arg5,
  IN  UINT64                    Arg6,
  IN  UINT64                    Arg7,
  IN  UINT64                    Arg8,
  IN  SAL_EXTENDED_SAL_PROC     ExtendedSalProc,
  IN  BOOLEAN                   VirtualMode,
  IN  VOID                      *Global
  )
{
  SAL_RETURN_REGS               ReturnVal;
  DXE_STATUS_CODE_CONTROLLER    *DxeStatusCode;

  switch (FunctionId) {

  case ReportStatusCodeServiceFunctionId:

    DxeStatusCode = (DXE_STATUS_CODE_CONTROLLER *) Global;

    //
    // Use atom operation to avoid the reentant of report.
    // If current status is not zero, then the function is reentrancy.
    //
    if (1 == InterlockedCompareExchange32 (&DxeStatusCode->StatusCodeNestStatus, 0, 1)) {
      ReturnVal.Status = EFI_DEVICE_ERROR;
      return ReturnVal;
    }

    if (FeaturePcdGet (PcdStatusCodeUseEfiSerial) || FeaturePcdGet (PcdStatusCodeUseHardSerial)) {
      SerialStatusCodeReportWorker (
        (EFI_STATUS_CODE_TYPE)    Arg2,
        (EFI_STATUS_CODE_VALUE)   Arg3,
        (UINT32)                  Arg4,
        (EFI_GUID *)              Arg5,
        (EFI_STATUS_CODE_DATA *)  Arg6
        );
    }
    if (FeaturePcdGet (PcdStatusCodeUseRuntimeMemory)) {
      RtMemoryStatusCodeReportWorker (
        DxeStatusCode->RtMemoryStatusCodeTable[VirtualMode],
        (EFI_STATUS_CODE_TYPE)    Arg2,
        (EFI_STATUS_CODE_VALUE)   Arg3,
        (UINT32)                  Arg4
        );
    }
    if (FeaturePcdGet (PcdStatusCodeUseDataHub)) {
      DataHubStatusCodeReportWorker (
        (EFI_STATUS_CODE_TYPE)    Arg2,
        (EFI_STATUS_CODE_VALUE)   Arg3,
        (UINT32)                  Arg4,
        (EFI_GUID *)              Arg5,
        (EFI_STATUS_CODE_DATA *)  Arg6
        );
    }
    if (FeaturePcdGet (PcdStatusCodeUseOEM)) {
      OemHookStatusCodeReport (
        (EFI_STATUS_CODE_TYPE)    Arg2,
        (EFI_STATUS_CODE_VALUE)   Arg3,
        (UINT32)                  Arg4,
        (EFI_GUID *)              Arg5,
        (EFI_STATUS_CODE_DATA *)  Arg6
        );
    }

    //
    // Restore the nest status of report
    //
    InterlockedCompareExchange32 (&DxeStatusCode->StatusCodeNestStatus, 1, 0);

    ReturnVal.Status = EFI_SUCCESS;

    break;

  default:
    ReturnVal.Status = EFI_SAL_INVALID_ARGUMENT;
    break;
  }

  return ReturnVal;
}

/**

  Install the ReportStatusCode runtime service.

  @param ImageHandle     Image handle of the loaded driver
  @param SystemTable     Pointer to the System Table

  @return                The function always returns success.

**/
EFI_STATUS
DxeStatusCodeDriverEntry (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle       = NULL;

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

  //
  // Initialize ESAL capabilities.
  //
  Status = RegisterEsalClass (
             &gEfiExtendedSalStatusCodeServicesProtocolGuid,
            &gDxeStatusCode,
             ReportEsalServiceEntry,
             StatusCodeFunctionId,
             NULL
             );
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}


/**
  Virtual address change notification call back. It converts physical mode global pointer to
  virtual mode.

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
  gDxeStatusCode.RtMemoryStatusCodeTable[VIRTUAL_MODE] =
    gDxeStatusCode.RtMemoryStatusCodeTable[PHYSICAL_MODE];

  //
  // Convert the physical mode pointer to virtual mode point.
  //
  EfiConvertPointer (
    0,
    (VOID **) &gDxeStatusCode.RtMemoryStatusCodeTable[VIRTUAL_MODE]
    );
}

