/** @file
  C1E feature.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Detects if C1E feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     C1E feature is supported.
  @retval FALSE    C1E feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
C1eSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  return IS_NEHALEM_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel);
}

/**
  Initializes C1E feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the C1E feature must be enabled.
                               If FALSE, then the C1E feature must be disabled.

  @retval RETURN_SUCCESS       C1E feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
C1eInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  //
  // The scope of C1EEnable bit in the MSR_NEHALEM_POWER_CTL is Package, only program
  // MSR_FEATURE_CONFIG for thread 0 core 0 in each package.
  //
  if ((CpuInfo->ProcessorInfo.Location.Thread != 0) || (CpuInfo->ProcessorInfo.Location.Core != 0)) {
  return RETURN_SUCCESS;
  }

  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_NEHALEM_POWER_CTL,
    MSR_NEHALEM_POWER_CTL_REGISTER,
    Bits.C1EEnable,
    (State) ? 1 : 0
    );
  return RETURN_SUCCESS;
}
