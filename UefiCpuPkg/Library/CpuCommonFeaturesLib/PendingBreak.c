/** @file
  Pending Break feature.

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
  Detects if Pending Break feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Pending Break feature is supported.
  @retval FALSE    Pending Break feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
PendingBreakSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  if (IS_ATOM_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_CORE2_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_CORE_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_PENTIUM_4_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_PENTIUM_M_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    return (CpuInfo->CpuIdVersionInfoEdx.Bits.PBE == 1);
  }
  return FALSE;
}

/**
  Initializes Pending Break feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Pending Break feature must be enabled.
                               If FALSE, then the Pending Break feature must be disabled.

  @retval RETURN_SUCCESS       Pending Break feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
PendingBreakInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  //
  // ATOM, CORE2, CORE, PENTIUM_4 and IS_PENTIUM_M_PROCESSOR have the same MSR index,
  // Simply use MSR_ATOM_IA32_MISC_ENABLE here
  //
  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_ATOM_IA32_MISC_ENABLE,
    MSR_ATOM_IA32_MISC_ENABLE_REGISTER,
    Bits.FERR,
    (State) ? 1 : 0
    );
  return RETURN_SUCCESS;
}
