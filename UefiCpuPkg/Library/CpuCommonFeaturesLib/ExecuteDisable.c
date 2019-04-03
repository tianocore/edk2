/** @file
  Execute Disable feature.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Detects if Execute Disable feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Execute Disable feature is supported.
  @retval FALSE    Execute Disable feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
ExecuteDisableSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  UINT32                         Eax;
  CPUID_EXTENDED_CPU_SIG_EDX     Edx;

  AsmCpuid (CPUID_EXTENDED_FUNCTION, &Eax, NULL, NULL, NULL);
  if (Eax <= CPUID_EXTENDED_FUNCTION) {
    //
    // Extended CPUID functions are not supported on this processor.
    //
    return FALSE;
  }

  AsmCpuid (CPUID_EXTENDED_CPU_SIG, NULL, NULL, NULL, &Edx.Uint32);
  return (Edx.Bits.NX != 0);
}

/**
  Initializes Execute Disable feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Execute Disable feature must be enabled.
                               If FALSE, then the Execute Disable feature must be disabled.

  @retval RETURN_SUCCESS       Execute Disable feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
ExecuteDisableInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  //
  // The scope of the MSR_IA32_EFER is core for below processor type, only program
  // MSR_IA32_EFER for thread 0 in each core.
  //
  if (IS_SILVERMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_EFER,
    MSR_IA32_EFER_REGISTER,
    Bits.NXE,
    (State) ? 1 : 0
    );
  return RETURN_SUCCESS;
}
