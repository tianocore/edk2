/** @file
  Local machine check exception feature.

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
  Detects if Local machine check exception feature supported on current 
  processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Local machine check exception feature is supported.
  @retval FALSE    Local machine check exception feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
LmceSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  MSR_IA32_MCG_CAP_REGISTER    McgCap;

  McgCap.Uint64 = AsmReadMsr64 (MSR_IA32_MCG_CAP);
  if (ProcessorNumber == 0) {
    DEBUG ((EFI_D_INFO, "LMCE eanble = %x\n", (BOOLEAN) (McgCap.Bits.MCG_LMCE_P != 0)));
  }
  return (BOOLEAN) (McgCap.Bits.MCG_LMCE_P != 0);
}

/**
  Initializes Local machine check exception feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Local machine check exception
                               feature must be enabled.
                               If FALSE, then the Local machine check exception
                               feature must be disabled.

  @retval RETURN_SUCCESS       Local machine check exception feature is initialized.

**/
RETURN_STATUS
EFIAPI
LmceInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_IA32_FEATURE_CONTROL_REGISTER    *MsrRegister;

  ASSERT (ConfigData != NULL);
  MsrRegister = (MSR_IA32_FEATURE_CONTROL_REGISTER *) ConfigData;
  if (MsrRegister[ProcessorNumber].Bits.Lock == 0) {
    CPU_REGISTER_TABLE_WRITE_FIELD (
      ProcessorNumber,
      Msr,
      MSR_IA32_FEATURE_CONTROL,
      MSR_IA32_FEATURE_CONTROL_REGISTER,
      Bits.LmceOn,
      (State) ? 1 : 0
      );
  }
  return RETURN_SUCCESS;
}
