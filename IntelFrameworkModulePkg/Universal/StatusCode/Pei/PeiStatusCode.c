/** @file
  Generic PeiStatusCode Module.

  Copyright (c) 2006, Intel Corporation
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

  Module Name:  PeiStatusCode.c

**/

#include "PeiStatusCode.h"

EFI_PEI_PROGRESS_CODE_PPI     mStatusCodePpi           = {
  ReportDispatcher
  };

EFI_PEI_PPI_DESCRIPTOR        mStatusCodePpiDescriptor = {
  EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiStatusCodePpiGuid,
  &mStatusCodePpi
  };

/**
  Report status code to all supported device.


  @param  PeiServices

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
  IN CONST EFI_PEI_SERVICES         **PeiServices,
  IN EFI_STATUS_CODE_TYPE     CodeType,
  IN EFI_STATUS_CODE_VALUE    Value,
  IN UINT32                   Instance,
  IN CONST EFI_GUID                 *CallerId OPTIONAL,
  IN CONST EFI_STATUS_CODE_DATA     *Data OPTIONAL
  )
{
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    SerialStatusCodeReportWorker (
      CodeType,
      Value,
      Instance,
      CallerId,
      Data
      );
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    MemoryStatusCodeReportWorker (
      CodeType,
      Value,
      Instance
      );
  }
  if (FeaturePcdGet (PcdStatusCodeUseOEM)) {
    OemHookStatusCodeReport (
      CodeType,
      Value,
      Instance,
      (EFI_GUID *)CallerId,
      (EFI_STATUS_CODE_DATA *)Data
      );
  }

  return EFI_SUCCESS;
}

/**
  Initialize PEI status codes and publish the status code
  PPI.

  @param  FileHandle  Handle of the file being invoked.
  @param  PeiServices Describes the list of possible PEI Services.

  @return                  The function always returns success.

**/
EFI_STATUS
PeiStatusCodeDriverEntry (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                  Status;

  //
  // Dispatch initialization request to sub-statuscode-devices.
  // If enable UseSerial, then initialize serial port.
  // if enable UseMemory, then initialize memory status code worker.
  // if enable UseOEM, then initialize Oem status code.
  //
  if (FeaturePcdGet (PcdStatusCodeUseSerial)) {
    Status = SerialPortInitialize();
    ASSERT_EFI_ERROR (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUseMemory)) {
    Status = MemoryStatusCodeInitializeWorker ();
    ASSERT_EFI_ERROR  (Status);
  }
  if (FeaturePcdGet (PcdStatusCodeUseOEM)) {
    Status = OemHookStatusCodeInitialize ();
    ASSERT_EFI_ERROR  (Status);
  }

  //
  // Install PeiStatusCodePpi.
  // PeiServices use this Ppi to output status code.
  // use library
  if (!FeaturePcdGet(PcdNtEmulatorEnable)) {
    Status = PeiServicesInstallPpi (&mStatusCodePpiDescriptor);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

