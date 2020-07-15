/** @file
  Features in MSR_IA32_FEATURE_CONTROL register.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Detects if VMX feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     VMX feature is supported.
  @retval FALSE    VMX feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
VmxSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  return (CpuInfo->CpuIdVersionInfoEcx.Bits.VMX == 1);
}

/**
  Initializes VMX feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the VMX feature must be enabled.
                               If FALSE, then the VMX feature must be disabled.

  @retval RETURN_SUCCESS       VMX feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
VmxInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  //
  // The scope of EnableVmxOutsideSmx bit in the MSR_IA32_FEATURE_CONTROL is core for
  // below processor type, only program MSR_IA32_FEATURE_CONTROL for thread 0 in each
  // core.
  //
  if (IS_SILVERMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_GOLDMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_GOLDMONT_PLUS_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  CPU_REGISTER_TABLE_TEST_THEN_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_FEATURE_CONTROL,
    MSR_IA32_FEATURE_CONTROL_REGISTER,
    Bits.EnableVmxOutsideSmx,
    (State) ? 1 : 0
    );

  return RETURN_SUCCESS;
}

/**
  Detects if Lock Feature Control Register feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Lock Feature Control Register feature is supported.
  @retval FALSE    Lock Feature Control Register feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
LockFeatureControlRegisterSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  return TRUE;
}

/**
  Initializes Lock Feature Control Register feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Lock Feature Control Register feature must be enabled.
                               If FALSE, then the Lock Feature Control Register feature must be disabled.

  @retval RETURN_SUCCESS       Lock Feature Control Register feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
LockFeatureControlRegisterInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  //
  // The scope of Lock bit in the MSR_IA32_FEATURE_CONTROL is core for
  // below processor type, only program MSR_IA32_FEATURE_CONTROL for thread 0 in each
  // core.
  //
  if (IS_SILVERMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_GOLDMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_GOLDMONT_PLUS_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  CPU_REGISTER_TABLE_TEST_THEN_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_FEATURE_CONTROL,
    MSR_IA32_FEATURE_CONTROL_REGISTER,
    Bits.Lock,
    1
    );

  return RETURN_SUCCESS;
}

/**
  Detects if SMX feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     SMX feature is supported.
  @retval FALSE    SMX feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
SmxSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  return (CpuInfo->CpuIdVersionInfoEcx.Bits.SMX == 1);
}

/**
  Initializes SMX feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then SMX feature must be enabled.
                               If FALSE, then SMX feature must be disabled.

  @retval RETURN_SUCCESS       SMX feature is initialized.
  @retval RETURN_UNSUPPORTED   VMX not initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
SmxInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  RETURN_STATUS                        Status;

  //
  // The scope of Lock bit in the MSR_IA32_FEATURE_CONTROL is core for
  // below processor type, only program MSR_IA32_FEATURE_CONTROL for thread 0 in each
  // core.
  //
  if (IS_GOLDMONT_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel) ||
      IS_GOLDMONT_PLUS_PROCESSOR (CpuInfo->DisplayFamily, CpuInfo->DisplayModel)) {
    if (CpuInfo->ProcessorInfo.Location.Thread != 0) {
      return RETURN_SUCCESS;
    }
  }

  Status = RETURN_SUCCESS;

  if (State && (!IsCpuFeatureInSetting (CPU_FEATURE_VMX))) {
    DEBUG ((DEBUG_WARN, "Warning :: Can't enable SMX feature when VMX feature not enabled, disable it.\n"));
    State = FALSE;
    Status = RETURN_UNSUPPORTED;
  }

  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    ControlRegister,
    4,
    IA32_CR4,
    Bits.SMXE,
    (State) ? 1 : 0
  )

  CPU_REGISTER_TABLE_TEST_THEN_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_FEATURE_CONTROL,
    MSR_IA32_FEATURE_CONTROL_REGISTER,
    Bits.SenterLocalFunctionEnables,
    (State) ? 0x7F : 0
    );

  CPU_REGISTER_TABLE_TEST_THEN_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_FEATURE_CONTROL,
    MSR_IA32_FEATURE_CONTROL_REGISTER,
    Bits.SenterGlobalEnable,
    (State) ? 1 : 0
    );

  CPU_REGISTER_TABLE_TEST_THEN_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_FEATURE_CONTROL,
    MSR_IA32_FEATURE_CONTROL_REGISTER,
    Bits.EnableVmxInsideSmx,
    (State) ? 1 : 0
    );

  return Status;
}
