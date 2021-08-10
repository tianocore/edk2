/** @file
  CPU MP Initialize Library common functions.

  Copyright (c) 2016 - 2020, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2020, AMD Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "MpLib.h"
#include "MpIntelTdx.h"
#include <Library/DebugLib.h>

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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
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
  ASSERT (FALSE);
  return EFI_UNSUPPORTED;
}

/**
  Whether Intel TDX is enabled.

  @return TRUE    TDX enabled
  @return FALSE   TDX not enabled
**/
BOOLEAN
EFIAPI
MpTdxIsEnabled (
  VOID
  )
{
  return FALSE;
}

/**
  The TDCALL instruction causes a VM exit to the Intel TDX module.  It is
  used to call guest-side Intel TDX functions, either local or a TD exit
  to the host VMM, as selected by Leaf.
  Leaf functions are described at <https://software.intel.com/content/
  www/us/en/develop/articles/intel-trust-domain-extensions.html>

  @param[in]      Leaf        Leaf number of TDCALL instruction
  @param[in]      Arg1        Arg1
  @param[in]      Arg2        Arg2
  @param[in]      Arg3        Arg3
  @param[in,out]  Results  Returned result of the Leaf function

  @return EFI_SUCCESS
  @return Other           See individual leaf functions
**/
EFI_STATUS
EFIAPI
MyTdCall (
  IN UINT64           Leaf,
  IN UINT64           Arg1,
  IN UINT64           Arg2,
  IN UINT64           Arg3,
  IN OUT VOID         *Results
  )
{
  return EFI_UNSUPPORTED;
}
