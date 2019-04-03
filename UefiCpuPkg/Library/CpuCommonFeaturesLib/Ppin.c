/** @file
  Protected Processor Inventory Number(PPIN) feature.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Detects if Protected Processor Inventory Number feature supported on current
  processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Protected Processor Inventory Number feature is supported.
  @retval FALSE    Protected Processor Inventory Number feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
PpinSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  )
{
  MSR_IVY_BRIDGE_PLATFORM_INFO_1_REGISTER    PlatformInfo;

  if ((CpuInfo->DisplayFamily == 0x06) &&
      ((CpuInfo->DisplayModel == 0x3E) ||      // Xeon E5 V2
       (CpuInfo->DisplayModel == 0x56) ||      // Xeon Processor D Product
       (CpuInfo->DisplayModel == 0x4F) ||      // Xeon E5 v4, E7 v4
       (CpuInfo->DisplayModel == 0x55) ||      // Xeon Processor Scalable
       (CpuInfo->DisplayModel == 0x57) ||      // Xeon Phi processor 3200, 5200, 7200 series.
       (CpuInfo->DisplayModel == 0x85)         // Future Xeon phi processor
     )) {
    //
    // Check whether platform support this feature.
    //
    PlatformInfo.Uint64 = AsmReadMsr64 (MSR_IVY_BRIDGE_PLATFORM_INFO_1);
    return (PlatformInfo.Bits.PPIN_CAP != 0);
  }

  return FALSE;
}

/**
  Initializes Protected Processor Inventory Number feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Protected Processor Inventory
                               Number feature must be enabled.
                               If FALSE, then the Protected Processor Inventory
                               Number feature must be disabled.

  @retval RETURN_SUCCESS       Protected Processor Inventory Number feature is
                               initialized.
  @retval RETURN_DEVICE_ERROR  Device can't change state because it has been
                               locked.

**/
RETURN_STATUS
EFIAPI
PpinInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData,  OPTIONAL
  IN BOOLEAN                           State
  )
{
  MSR_IVY_BRIDGE_PPIN_CTL_REGISTER     MsrPpinCtrl;

  //
  // Check whether device already lock this register.
  // If already locked, just base on the request state and
  // the current state to return the status.
  //
  MsrPpinCtrl.Uint64 = AsmReadMsr64 (MSR_IVY_BRIDGE_PPIN_CTL);
  if (MsrPpinCtrl.Bits.LockOut != 0) {
    return MsrPpinCtrl.Bits.Enable_PPIN == State ? RETURN_SUCCESS : RETURN_DEVICE_ERROR;
  }

  //
  // Support function already check the processor which support PPIN feature, so this function not need
  // to check the processor again.
  //
  // The scope of the MSR_IVY_BRIDGE_PPIN_CTL is package level, only program MSR_IVY_BRIDGE_PPIN_CTL for
  // thread 0 core 0 in each package.
  //
  if ((CpuInfo->ProcessorInfo.Location.Thread != 0) || (CpuInfo->ProcessorInfo.Location.Core != 0)) {
    return RETURN_SUCCESS;
  }

  CPU_REGISTER_TABLE_WRITE_FIELD (
    ProcessorNumber,
    Msr,
    MSR_IVY_BRIDGE_PPIN_CTL,
    MSR_IVY_BRIDGE_PPIN_CTL_REGISTER,
    Bits.Enable_PPIN,
    (State) ? 1 : 0
    );

  return RETURN_SUCCESS;
}
