/** @file
  X2Apic feature.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
VOID *
EFIAPI
X2ApicGetConfigData (
  IN UINTN  NumberOfProcessors
  )
{
  BOOLEAN  *ConfigData;

  ConfigData = AllocateZeroPool (sizeof (BOOLEAN) * NumberOfProcessors);
  ASSERT (ConfigData != NULL);
  return ConfigData;
}

/**
  Detects if X2Apci feature supported on current processor.

  Detect if X2Apci has been already enabled.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     X2Apci feature is supported.
  @retval FALSE    X2Apci feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
X2ApicSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  BOOLEAN  *X2ApicEnabled;

  ASSERT (ConfigData != NULL);
  X2ApicEnabled = (BOOLEAN *)ConfigData;
  //
  // *ConfigData indicates if X2APIC enabled on current processor
  //
  X2ApicEnabled[ProcessorNumber] = (GetApicMode () == LOCAL_APIC_MODE_X2APIC) ? TRUE : FALSE;

  return (CpuInfo->CpuIdVersionInfoEcx.Bits.x2APIC == 1);
}

/**
  Initializes X2Apci feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the X2Apci feature must be enabled.
                               If FALSE, then the X2Apci feature must be disabled.

  @retval RETURN_SUCCESS       X2Apci feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
X2ApicInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  )
{
  BOOLEAN  *X2ApicEnabled;

  //
  // The scope of the MSR_IA32_APIC_BASE is core for below processor type, only program
  // MSR_IA32_APIC_BASE for thread 0 in each core.
  //
  if (IS_SILVERMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  ASSERT (ConfigData != NULL);
  X2ApicEnabled = (BOOLEAN *)ConfigData;
  if (X2ApicEnabled[ProcessorNumber]) {
    PRE_SMM_CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_APIC_BASE,
      MSR_IA32_APIC_BASE_REGISTER,
      Bits.EXTD,
      1
      );
  } else {
    //
    // Enable X2APIC mode only if X2APIC is not enabled,
    // Needn't to disabe X2APIC mode again if X2APIC is not enabled
    //
    if (State) {
      CPU_REGISTER_TABLE_WRITE_FIELD (
        ProcessorNumber,
        Msr,
        MSR_IA32_APIC_BASE,
        MSR_IA32_APIC_BASE_REGISTER,
        Bits.EXTD,
        1
        );
    }
  }

  return RETURN_SUCCESS;
}
