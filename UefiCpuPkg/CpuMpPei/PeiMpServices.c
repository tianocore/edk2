/** @file
  Implementation of Multiple Processor PPI services.

  Copyright (c) 2015, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "PeiMpServices.h"



/**
  Find the current Processor number by APIC ID.

  @param PeiCpuMpData        Pointer to PEI CPU MP Data
  @param ProcessorNumber     Return the pocessor number found

  @retval EFI_SUCCESS        ProcessorNumber is found and returned.
  @retval EFI_NOT_FOUND      ProcessorNumber is not found.
**/
EFI_STATUS
GetProcessorNumber (
  IN PEI_CPU_MP_DATA         *PeiCpuMpData,
  OUT UINTN                  *ProcessorNumber
  )
{
  UINTN                   TotalProcessorNumber;
  UINTN                   Index;

  TotalProcessorNumber = PeiCpuMpData->CpuCount;
  for (Index = 0; Index < TotalProcessorNumber; Index ++) {
    if (PeiCpuMpData->CpuData[Index].ApicId == GetInitialApicId ()) {
      *ProcessorNumber = Index;
      return EFI_SUCCESS;
    }
  }
  return EFI_NOT_FOUND;
}


/**
  This return the handle number for the calling processor.  This service may be
  called from the BSP and APs.

  This service returns the processor handle number for the calling processor.
  The returned value is in the range from 0 to the total number of logical
  processors minus 1. The total number of logical processors can be retrieved
  with EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors(). This service may be
  called from the BSP and APs. If ProcessorNumber is NULL, then EFI_INVALID_PARAMETER
  is returned. Otherwise, the current processors handle number is returned in
  ProcessorNumber, and EFI_SUCCESS is returned.

  @param[in]  PeiServices         An indirect pointer to the PEI Services Table
                                  published by the PEI Foundation.
  @param[in]  This                A pointer to the EFI_PEI_MP_SERVICES_PPI instance.
  @param[out] ProcessorNumber     The handle number of the AP. The range is from 0 to the
                                  total number of logical processors minus 1. The total
                                  number of logical processors can be retrieved by
                                  EFI_PEI_MP_SERVICES_PPI.GetNumberOfProcessors().

  @retval EFI_SUCCESS             The current processor handle number was returned in
                                  ProcessorNumber.
  @retval EFI_INVALID_PARAMETER   ProcessorNumber is NULL.
**/
EFI_STATUS
EFIAPI
PeiWhoAmI (
  IN  CONST EFI_PEI_SERVICES   **PeiServices,
  IN  EFI_PEI_MP_SERVICES_PPI  *This,
  OUT UINTN                    *ProcessorNumber
  )
{
  PEI_CPU_MP_DATA         *PeiCpuMpData;

  PeiCpuMpData = GetMpHobData ();
  if (PeiCpuMpData == NULL) {
    return EFI_NOT_FOUND;
  }

  if (ProcessorNumber == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  return GetProcessorNumber (PeiCpuMpData, ProcessorNumber);
}

