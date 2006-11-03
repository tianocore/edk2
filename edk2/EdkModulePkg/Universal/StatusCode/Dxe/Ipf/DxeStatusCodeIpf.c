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

  case ReportStatusCodeService:

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
  EFI_STATUS Status;

  //
  // Dispatch initialization request to supported devices
  //
  InitializationDispatcherWorker ();

  //
  // Initialize ESAL capabilities.
  //
  Status = RegisterEsalClass (
             &gEfiExtendedSalStatusCodeServicesProtocolGuid,
             &gDxeStatusCode,
             ReportEsalServiceEntry,
             StatusCode,
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

