/** @file
  CPU MP Initialize Library common functions for Td guest.

  Copyright (c) 2020 - 2022, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include "MpIntelTdx.h"

/**
  Gets detailed MP-related information on the requested processor at the
  instant this call is made. This service may only be called from the BSP.

  In current stage only the BSP is workable. So ProcessorNumber should be 0.

  @param[in]  ProcessorNumber       The handle number of processor.
  @param[out] ProcessorInfoBuffer   A pointer to the buffer where information for
                                    the requested processor is deposited.
  @param[out]  HealthData            Return processor health data.

  @retval EFI_SUCCESS             Processor information was returned.
  @retval EFI_DEVICE_ERROR        The calling processor is an AP.
  @retval EFI_INVALID_PARAMETER   ProcessorInfoBuffer is NULL or ProcessorNumber is not 0.
  @retval EFI_NOT_FOUND           The processor with the handle specified by
                                  ProcessorNumber does not exist in the platform.
  @retval EFI_NOT_READY           MP Initialize Library is not initialized.

**/
EFI_STATUS
TdxMpInitLibGetProcessorInfo (
  IN  UINTN                      ProcessorNumber,
  OUT EFI_PROCESSOR_INFORMATION  *ProcessorInfoBuffer,
  OUT EFI_HEALTH_FLAGS           *HealthData  OPTIONAL
  )
{
  UINTN  OriginalProcessorNumber;

  //
  // Lower 24 bits contains the actual processor number.
  //
  OriginalProcessorNumber = ProcessorNumber;
  ProcessorNumber        &= BIT24 - 1;

  if ((ProcessorInfoBuffer == NULL) || (ProcessorNumber != 0)) {
    return EFI_INVALID_PARAMETER;
  }

  ProcessorInfoBuffer->ProcessorId = 0;
  ProcessorInfoBuffer->StatusFlag  = PROCESSOR_AS_BSP_BIT | PROCESSOR_ENABLED_BIT;
  ZeroMem (&ProcessorInfoBuffer->Location, sizeof (EFI_CPU_PHYSICAL_LOCATION));

  if ((OriginalProcessorNumber & CPU_V2_EXTENDED_TOPOLOGY) != 0) {
    ZeroMem (&ProcessorInfoBuffer->ExtendedInformation.Location2, sizeof (EFI_CPU_PHYSICAL_LOCATION2));
  }

  if (HealthData != NULL) {
    HealthData->Uint32 = 0;
  }

  return EFI_SUCCESS;
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
TdxMpInitLibGetNumberOfProcessors (
  OUT UINTN *NumberOfProcessors, OPTIONAL
  OUT UINTN *NumberOfEnabledProcessors OPTIONAL
  )
{
  ASSERT (NumberOfProcessors != NULL || NumberOfEnabledProcessors != NULL);
  //
  // In current stage only the BSP is workable. So NumberOfProcessors
  // & NumberOfEnableddProcessors are both 1.
  //
  if (NumberOfProcessors != NULL) {
    *NumberOfProcessors = 1;
  }

  if (NumberOfEnabledProcessors != NULL) {
    *NumberOfEnabledProcessors = 1;
  }

  return EFI_SUCCESS;
}
