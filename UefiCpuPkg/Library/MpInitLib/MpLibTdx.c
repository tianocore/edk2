/** @file
  CPU MP Initialize Library common functions.

  Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include "MpIntelTdx.h"
#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <IndustryStandard/Tdx.h>

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.
  @param[out]  HealthData            Return processor health data.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
TdxMpInitLibGetProcessorInfo (
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer,
  OUT EFI_HEALTH_FLAGS           *HealthData  OPTIONAL
  )
{
  EFI_STATUS              Status;
  TD_RETURN_DATA          TdReturnData;

  if (ProcessorInfoBuffer == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  ASSERT(Status == EFI_SUCCESS);

  if (ProcessorNumber >= TdReturnData.TdInfo.NumVcpus) {
    return EFI_NOT_FOUND;
  }

  ProcessorInfoBuffer->ProcessorId = ProcessorNumber;
  ProcessorInfoBuffer->StatusFlag  = 0;
  if (ProcessorNumber == 0) {
    ProcessorInfoBuffer->StatusFlag |= PROCESSOR_AS_BSP_BIT;
  }
  ProcessorInfoBuffer->StatusFlag |= PROCESSOR_ENABLED_BIT;

  //
  // Get processor location information
  //
  GetProcessorLocationByApicId (
    (UINT32)ProcessorNumber,
    &ProcessorInfoBuffer->Location.Package,
    &ProcessorInfoBuffer->Location.Core,
    &ProcessorInfoBuffer->Location.Thread
    );

  if (HealthData != NULL) {
    HealthData->Uint32 = 0;
  }

  return Status;
}

/**
  Retrieves the number of logical processor in the platform and the number of
  those logical processors that are enabled on this boot. This service may only
  be called from the BSP.

  @param[out] NumberOfProcessors          Pointer to the total number of logical
                                          processors in the system, including the BSP
                                          and disabled APs.
  @param[out] NumberOfEnabledProcessors   Pointer to the number of enabled logical
                                          processors that exist in system, including
                                          the BSP.

  @retval EFI_SUCCESS             The number of logical processors and enabled
                                  logical processors was retrieved.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   NumberOfProcessors is NULL and NumberOfEnabledProcessors
                                  is NULL.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
EFIAPI
TdxMpInitLibGetNumberOfProcessors (
  OUT UINTN                     *NumberOfProcessors,       OPTIONAL
  OUT UINTN                     *NumberOfEnabledProcessors OPTIONAL
  )
{
  EFI_STATUS              Status;
  TD_RETURN_DATA          TdReturnData;

  if ((NumberOfProcessors == NULL) && (NumberOfEnabledProcessors == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = TdCall (TDCALL_TDINFO, 0, 0, 0, &TdReturnData);
  ASSERT(Status == EFI_SUCCESS);

  if (NumberOfProcessors != NULL) {
    *NumberOfProcessors = TdReturnData.TdInfo.NumVcpus;
  }
  if (NumberOfEnabledProcessors != NULL) {
    *NumberOfEnabledProcessors = TdReturnData.TdInfo.MaxVcpus;
  }

  return Status;
}
