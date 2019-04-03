/** @file
  Clock Modulation feature.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Detects if Clock Modulation feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Clock Modulation feature is supported.
  @retval FALSE    Clock Modulation feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
ClockModulationSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  return (CpuInfo->CpuIdVersionInfoEdx.Bits.ACPI == 1);
}

/**
  Initializes Clock Modulation feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Clock Modulation feature must be enabled.
                               If FALSE, then the Clock Modulation feature must be disabled.

  @retval RETURN_SUCCESS       Clock Modulation feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
ClockModulationInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  CPUID_THERMAL_POWER_MANAGEMENT_EAX   ThermalPowerManagementEax;
  AsmCpuid (CPUID_THERMAL_POWER_MANAGEMENT, &ThermalPowerManagementEax.Uint32, NULL, NULL, NULL);

  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_CLOCK_MODULATION,
    MSR_IA32_CLOCK_MODULATION_REGISTER,
    Bits.OnDemandClockModulationDutyCycle,
    PcdGet8 (PcdCpuClockModulationDutyCycle) >> 1
    );
  if (ThermalPowerManagementEax.Bits.ECMD == 1) {
    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_CLOCK_MODULATION,
      MSR_IA32_CLOCK_MODULATION_REGISTER,
      Bits.ExtendedOnDemandClockModulationDutyCycle,
      PcdGet8 (PcdCpuClockModulationDutyCycle) & BIT0
      );
  }
  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IA32_CLOCK_MODULATION,
    MSR_IA32_CLOCK_MODULATION_REGISTER,
    Bits.OnDemandClockModulationEnable,
    (State) ? 1 : 0
    );
  return RETURN_SUCCESS;
}
