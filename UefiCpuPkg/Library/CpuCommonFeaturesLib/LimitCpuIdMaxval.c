/** @file
  LimitCpuidMaxval Feature.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuCommonFeatures.h"

/**
  Detects if LimitCpuidMaxval feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     LimitCpuidMaxval feature is supported.
  @retval FALSE    LimitCpuidMaxval feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
LimitCpuidMaxvalSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  UINT32  Eax;

  AsmCpuid (CPUID_SIGNATURE, &Eax, NULL, NULL, NULL);
  return (Eax > 3);
}

/**
  Initializes LimitCpuidMaxval feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the LimitCpuidMaxval feature must be enabled.
                               If FALSE, then the LimitCpuidMaxval feature must be disabled.

  @retval RETURN_SUCCESS       LimitCpuidMaxval feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
LimitCpuidMaxvalInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_MISC_ENABLE,
    MSR_IA32_MISC_ENABLE_REGISTER,
    Bits.LimitCpuidMaxval,
    (State) ? 1 : 0
    );
  return RETURN_SUCCESS;
}
