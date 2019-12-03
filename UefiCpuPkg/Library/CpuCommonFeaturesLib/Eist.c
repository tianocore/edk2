/** @file
  Enhanced Intel SpeedStep feature.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Detects if Enhanced Intel SpeedStep feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Enhanced Intel SpeedStep feature is supported.
  @retval FALSE    Enhanced Intel SpeedStep feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
EistSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  return (CpuInfo->CpuIdVersionInfoEcx.Bits.EIST == 1);
}

/**
  Initializes Enhanced Intel SpeedStep feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Enhanced Intel SpeedStep feature
                               must be enabled.
                               If FALSE, then the Enhanced Intel SpeedStep feature
                               must be disabled.

  @retval RETURN_SUCCESS       Enhanced Intel SpeedStep feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
EistInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  //
  // The scope of the MSR_IA32_MISC_ENABLE is core for below processor type, only program
  // MSR_IA32_MISC_ENABLE for thread 0 in each core.
  //
  if (IS_ATOM_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_CORE_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_CORE2_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_MISC_ENABLE,
    MSR_IA32_MISC_ENABLE_REGISTER,
    Bits.EIST,
    (State) ? 1 : 0
    );
  return RETURN_SUCCESS;
}
