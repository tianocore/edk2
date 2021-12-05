/** @file
  CPU Common features library header file.

  Copyright (c) 2017 - 2019, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _CPU_COMMON_FEATURES_H_
#define _CPU_COMMON_FEATURES_H_

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/PcdLib.h>
#include <Library/DebugLib.h>
#include <Library/RegisterCpuFeaturesLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/MemoryAllocationLib.h>
#include <Library/LocalApicLib.h>

#include <Register/Intel/Cpuid.h>
#include <Register/Intel/Msr.h>

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
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if Enhanced Intel SpeedStep feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Enhanced Intel SpeedStep feature is supported.
  @retval FALSE    Enhanced Intel SpeedStep feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
EistSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes Enhanced Intel SpeedStep feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Enhanced Intel SpeedStep feature
                               must be enabled.
                               If FALSE, then the Enhanced Intel SpeedStep feature
                               must be disabled.

  @retval RETURN_SUCCESS       Enhanced Intel SpeedStep feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
EistInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Initializes Fast-Strings feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Fast-Strings feature must be enabled.
                               If FALSE, then the Fast-Strings feature must be disabled.

  @retval RETURN_SUCCESS       Fast-Strings feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
FastStringsInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if MONITOR/MWAIT feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     MONITOR/MWAIT feature is supported.
  @retval FALSE    MONITOR/MWAIT feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
MonitorMwaitSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes MONITOR/MWAIT feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the MONITOR/MWAIT feature must be enabled.
                               If FALSE, then the MONITOR/MWAIT feature must be disabled.

  @retval RETURN_SUCCESS       MONITOR/MWAIT feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
MonitorMwaitInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if LimitCpuidMaxval feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     LimitCpuidMaxval feature is supported.
  @retval FALSE    LimitCpuidMaxval feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
LimitCpuidMaxvalSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes LimitCpuidMaxval feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the LimitCpuidMaxval feature must be enabled.
                               If FALSE, then the LimitCpuidMaxval feature must be disabled.

  @retval RETURN_SUCCESS       LimitCpuidMaxval feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
LimitCpuidMaxvalInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if Machine Check Exception feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Machine Check Exception feature is supported.
  @retval FALSE    Machine Check Exception feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
MceSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes Machine Check Exception feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Machine Check Exception feature must be enabled.
                               If FALSE, then the Machine Check Exception feature must be disabled.

  @retval RETURN_SUCCESS       Machine Check Exception feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
MceInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if Machine Check Architecture feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Machine Check Architecture feature is supported.
  @retval FALSE    Machine Check Architecture feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
McaSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes Machine Check Architecture feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Machine Check Architecture feature must be enabled.
                               If FALSE, then the Machine Check Architecture feature must be disabled.

  @retval RETURN_SUCCESS       Machine Check Architecture feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
McaInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if IA32_MCG_CTL feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     IA32_MCG_CTL feature is supported.
  @retval FALSE    IA32_MCG_CTL feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
McgCtlSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes IA32_MCG_CTL feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the IA32_MCG_CTL feature must be enabled.
                               If FALSE, then the IA32_MCG_CTL feature must be disabled.

  @retval RETURN_SUCCESS       IA32_MCG_CTL feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
McgCtlInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Detects if C1E feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     C1E feature is supported.
  @retval FALSE    C1E feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
C1eSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes C1E feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the C1E feature must be enabled.
                               If FALSE, then the C1E feature must be disabled.

  @retval RETURN_SUCCESS       C1E feature is initialized.

  @note This service could be called by BSP only.
**/
RETURN_STATUS
EFIAPI
C1eInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  );

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
  );

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
VOID *
EFIAPI
PpinGetConfigData (
  IN UINTN  NumberOfProcessors
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

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
  );

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
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

/**
  Prepares for the data used by CPU feature detection and initialization.

  @param[in]  NumberOfProcessors  The number of CPUs in the platform.

  @return  Pointer to a buffer of CPU related configuration data.

  @note This service could be called by BSP only.
**/
VOID *
EFIAPI
ProcTraceGetConfigData (
  IN UINTN  NumberOfProcessors
  );

/**
  Detects if Intel Processor Trace feature supported on current processor.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().

  @retval TRUE     Processor Trace feature is supported.
  @retval FALSE    Processor Trace feature is not supported.

  @note This service could be called by BSP/APs.
**/
BOOLEAN
EFIAPI
ProcTraceSupport (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData  OPTIONAL
  );

/**
  Initializes Intel Processor Trace feature to specific state.

  @param[in]  ProcessorNumber  The index of the CPU executing this function.
  @param[in]  CpuInfo          A pointer to the REGISTER_CPU_FEATURE_INFORMATION
                               structure for the CPU executing this function.
  @param[in]  ConfigData       A pointer to the configuration buffer returned
                               by CPU_FEATURE_GET_CONFIG_DATA.  NULL if
                               CPU_FEATURE_GET_CONFIG_DATA was not provided in
                               RegisterCpuFeature().
  @param[in]  State            If TRUE, then the Processor Trace feature must be
                               enabled.
                               If FALSE, then the Processor Trace feature must be
                               disabled.

  @retval RETURN_SUCCESS       Intel Processor Trace feature is initialized.

**/
RETURN_STATUS
EFIAPI
ProcTraceInitialize (
  IN UINTN                             ProcessorNumber,
  IN REGISTER_CPU_FEATURE_INFORMATION  *CpuInfo,
  IN VOID                              *ConfigData   OPTIONAL,
  IN BOOLEAN                           State
  );

#endif
