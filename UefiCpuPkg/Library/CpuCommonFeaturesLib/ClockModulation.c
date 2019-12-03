/** @file
  Clock Modulation feature.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

typedef struct  {
  CPUID_THERMAL_POWER_MANAGEMENT_EAX  ThermalPowerManagementEax;
  MSR_IA32_CLOCK_MODULATION_REGISTER  ClockModulation;
} CLOCK_MODULATION_CONFIG_DATA;

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
VOID *
EFIAPI
ClockModulationGetConfigData (
  IN UINTN  NumberOfProcessors
  )
{
  UINT32    *ConfigData;

  ConfigData = AllocateZeroPool (sizeof (CLOCK_MODULATION_CONFIG_DATA) * NumberOfProcessors);
  ASSERT (ConfigData != NULL);
  return ConfigData;
}

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
  CLOCK_MODULATION_CONFIG_DATA         *CmConfigData;

  if (CpuInfo->CpuIdVersionInfoEdx.Bits.ACPI == 1) {
    CmConfigData = (CLOCK_MODULATION_CONFIG_DATA *) ConfigData;
    ASSERT (CmConfigData != NULL);
    AsmCpuid (
      CPUID_THERMAL_POWER_MANAGEMENT,
      &CmConfigData[ProcessorNumber].ThermalPowerManagementEax.Uint32,
      NULL,
      NULL,
      NULL
      );
    CmConfigData[ProcessorNumber].ClockModulation.Uint64 = AsmReadMsr64 (MSR_IA32_CLOCK_MODULATION);
    return TRUE;
  }
  return FALSE;
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
  CLOCK_MODULATION_CONFIG_DATA         *CmConfigData;
  MSR_IA32_CLOCK_MODULATION_REGISTER   *ClockModulation;

  CmConfigData = (CLOCK_MODULATION_CONFIG_DATA *) ConfigData;
  ASSERT (CmConfigData != NULL);
  ClockModulation = &CmConfigData[ProcessorNumber].ClockModulation;

  if (State) {
    ClockModulation->Bits.OnDemandClockModulationEnable = 1;
    ClockModulation->Bits.OnDemandClockModulationDutyCycle = PcdGet8 (PcdCpuClockModulationDutyCycle) >> 1;
    if (CmConfigData[ProcessorNumber].ThermalPowerManagementEax.Bits.ECMD == 1) {
      ClockModulation->Bits.ExtendedOnDemandClockModulationDutyCycle = PcdGet8 (PcdCpuClockModulationDutyCycle) & BIT0;
    }
  } else {
    ClockModulation->Bits.OnDemandClockModulationEnable = 0;
  }

  CPU_REGISTER_TABLE_WRITE64 (
    ProcessorNumber,
    Msr,
    MSR_IA32_CLOCK_MODULATION,
    ClockModulation->Uint64
    );

  return RETURN_SUCCESS;
}
