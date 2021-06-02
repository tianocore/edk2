/** @file
  Protected Processor Inventory Number(PPIN) feature.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
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
PpinGetConfigData (
  IN UINTN               NumberOfProcessors
  )
{
  VOID          *ConfigData;

  ConfigData = AllocateZeroPool (sizeof (MSR_IVY_BRIDGE_PPIN_CTL_REGISTER) * NumberOfProcessors);
  ASSERT (ConfigData != NULL);
  return ConfigData;
}

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
  MSR_IVY_BRIDGE_PPIN_CTL_REGISTER           *MsrPpinCtrl;

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
    if (PlatformInfo.Bits.PPIN_CAP != 0) {
      MsrPpinCtrl = (MSR_IVY_BRIDGE_PPIN_CTL_REGISTER *) ConfigData;
      ASSERT (MsrPpinCtrl != NULL);
      MsrPpinCtrl[ProcessorNumber].Uint64 = AsmReadMsr64 (MSR_IVY_BRIDGE_PPIN_CTL);
      return TRUE;
    }
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

  @note This service could be called by BSP only.
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
  MSR_IVY_BRIDGE_PPIN_CTL_REGISTER     *MsrPpinCtrl;

  MsrPpinCtrl = (MSR_IVY_BRIDGE_PPIN_CTL_REGISTER *) ConfigData;
  ASSERT (MsrPpinCtrl != NULL);

  //
  // Check whether processor already lock this register.
  // If already locked, just based on the request state and
  // the current state to return the status.
  //
  if (MsrPpinCtrl[ProcessorNumber].Bits.LockOut != 0) {
    return MsrPpinCtrl[ProcessorNumber].Bits.Enable_PPIN == State ? RETURN_SUCCESS : RETURN_DEVICE_ERROR;
  }

  //
  // Support function already check the processor which support PPIN feature, so this function not need
  // to check the processor again.
  //
  // The scope of the MSR_IVY_BRIDGE_PPIN_CTL is package level, only program MSR_IVY_BRIDGE_PPIN_CTL
  // once for each package.
  //
  if ((CpuInfo->First.Thread == 0) || (CpuInfo->First.Core == 0)) {
    return RETURN_SUCCESS;
  }

  if (State) {
    //
    // Enable and Unlock.
    // According to SDM, once Enable_PPIN is set, attempt to write 1 to LockOut will cause #GP.
    //
    MsrPpinCtrl[ProcessorNumber].Bits.Enable_PPIN = 1;
    MsrPpinCtrl[ProcessorNumber].Bits.LockOut = 0;
  } else {
    //
    // Disable and Lock.
    // According to SDM, writing 1 to LockOut is permitted only if Enable_PPIN is clear.
    //
    MsrPpinCtrl[ProcessorNumber].Bits.Enable_PPIN = 0;
    MsrPpinCtrl[ProcessorNumber].Bits.LockOut = 1;
  }

  CPU_REGISTER_TABLE_WRITE64 (
    ProcessorNumber,
    Msr,
    MSR_IVY_BRIDGE_PPIN_CTL,
    MsrPpinCtrl[ProcessorNumber].Uint64
    );

  return RETURN_SUCCESS;
}
