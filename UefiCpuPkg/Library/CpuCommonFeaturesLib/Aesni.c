/** @file
  AESNI feature.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
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
AesniGetConfigData (
  IN UINTN  NumberOfProcessors
  )
{
  UINT64                            *ConfigData;

  ConfigData = AllocateZeroPool (sizeof (UINT64) * NumberOfProcessors);
  ASSERT (ConfigData != NULL);
  return ConfigData;
}

/**
  Detects if AESNI feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     AESNI feature is supported.
  @retval FALSE    AESNI feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
AesniSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  MSR_SANDY_BRIDGE_FEATURE_CONFIG_REGISTER   *MsrFeatureConfig;

  if (CpuInfo->CpuIdVersionInfoEcx.Bits.AESNI == 1) {
    MsrFeatureConfig = (MSR_SANDY_BRIDGE_FEATURE_CONFIG_REGISTER *) ConfigData;
    ASSERT (MsrFeatureConfig != NULL);
    MsrFeatureConfig[ProcessorNumber].Uint64 = AsmReadMsr64 (MSR_SANDY_BRIDGE_FEATURE_CONFIG);
    return TRUE;
  }
  return FALSE;
}

/**
  Initializes AESNI feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the AESNI feature must be enabled.
                               If FALSE, then the AESNI feature must be disabled.

  @retval RETURN_SUCCESS       AESNI feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
AesniInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_SANDY_BRIDGE_FEATURE_CONFIG_REGISTER   *MsrFeatureConfig;

  //
  // SANDY_BRIDGE, SILVERMONT, XEON_5600, XEON_7, and XEON_PHI have the same MSR index,
  // Simply use MSR_SANDY_BRIDGE_FEATURE_CONFIG here
  //
  // The scope of the MSR_SANDY_BRIDGE_FEATURE_CONFIG is Core, only program MSR_FEATURE_CONFIG for thread 0
  // of each core. Otherwise, once a thread in the core disabled AES, the other thread will cause GP when
  // programming it.
  //
  if (CpuInfo->ProcessorInfo.Location.Thread == 0) {
    MsrFeatureConfig = (MSR_SANDY_BRIDGE_FEATURE_CONFIG_REGISTER *) ConfigData;
    ASSERT (MsrFeatureConfig != NULL);
    if ((MsrFeatureConfig[ProcessorNumber].Bits.AESConfiguration & BIT0) == 0) {
      CPU_REGISTER_TABLE_WRITE_FIELD (
        ProcessorNumber,
        Msr,
        MSR_SANDY_BRIDGE_FEATURE_CONFIG,
        MSR_SANDY_BRIDGE_FEATURE_CONFIG_REGISTER,
        Bits.AESConfiguration,
        BIT0 | ((State) ? 0 : BIT1)
        );
    }
  }
  return RETURN_SUCCESS;
}
