/** @file
  This library registers CPU features defined in Intel(R) 64 and IA-32
  Architectures Software Developer's Manual.

  Copyright (c) 2017 - 2020, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuCommonFeatures.h"

/**
  Register CPU features.

  @retval  RETURN_SUCCESS            Register successfully
**/
RETURN_STATUS
EFIAPI
CpuCommonFeaturesLibConstructor (
  VOID
  )
{
  RETURN_STATUS  Status;

  if (IsCpuFeatureSupported (CPU_FEATURE_AESNI)) {
    Status = RegisterCpuFeature (
               "AESNI",
               AesniGetConfigData,
               AesniSupport,
               AesniInitialize,
               CPU_FEATURE_AESNI,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_MWAIT)) {
    Status = RegisterCpuFeature (
               "MWAIT",
               NULL,
               MonitorMwaitSupport,
               MonitorMwaitInitialize,
               CPU_FEATURE_MWAIT,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_ACPI)) {
    Status = RegisterCpuFeature (
               "ACPI",
               ClockModulationGetConfigData,
               ClockModulationSupport,
               ClockModulationInitialize,
               CPU_FEATURE_ACPI,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_EIST)) {
    Status = RegisterCpuFeature (
               "EIST",
               NULL,
               EistSupport,
               EistInitialize,
               CPU_FEATURE_EIST,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_FASTSTRINGS)) {
    Status = RegisterCpuFeature (
               "FastStrings",
               NULL,
               NULL,
               FastStringsInitialize,
               CPU_FEATURE_FASTSTRINGS,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_LOCK_FEATURE_CONTROL_REGISTER)) {
    Status = RegisterCpuFeature (
               "Lock Feature Control Register",
               NULL,
               LockFeatureControlRegisterSupport,
               LockFeatureControlRegisterInitialize,
               CPU_FEATURE_LOCK_FEATURE_CONTROL_REGISTER,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_SMX)) {
    Status = RegisterCpuFeature (
               "SMX",
               NULL,
               SmxSupport,
               SmxInitialize,
               CPU_FEATURE_SMX,
               CPU_FEATURE_LOCK_FEATURE_CONTROL_REGISTER | CPU_FEATURE_THREAD_BEFORE,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_VMX)) {
    Status = RegisterCpuFeature (
               "VMX",
               NULL,
               VmxSupport,
               VmxInitialize,
               CPU_FEATURE_VMX,
               CPU_FEATURE_LOCK_FEATURE_CONTROL_REGISTER | CPU_FEATURE_THREAD_BEFORE,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_LIMIT_CPUID_MAX_VAL)) {
    Status = RegisterCpuFeature (
               "Limit CpuId Maximum Value",
               NULL,
               LimitCpuidMaxvalSupport,
               LimitCpuidMaxvalInitialize,
               CPU_FEATURE_LIMIT_CPUID_MAX_VAL,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_MCE)) {
    Status = RegisterCpuFeature (
               "Machine Check Enable",
               NULL,
               MceSupport,
               MceInitialize,
               CPU_FEATURE_MCE,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_MCA)) {
    Status = RegisterCpuFeature (
               "Machine Check Architect",
               NULL,
               McaSupport,
               McaInitialize,
               CPU_FEATURE_MCA,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_MCG_CTL)) {
    Status = RegisterCpuFeature (
               "MCG_CTL",
               NULL,
               McgCtlSupport,
               McgCtlInitialize,
               CPU_FEATURE_MCG_CTL,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_PENDING_BREAK)) {
    Status = RegisterCpuFeature (
               "Pending Break",
               NULL,
               PendingBreakSupport,
               PendingBreakInitialize,
               CPU_FEATURE_PENDING_BREAK,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_C1E)) {
    Status = RegisterCpuFeature (
               "C1E",
               NULL,
               C1eSupport,
               C1eInitialize,
               CPU_FEATURE_C1E,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_X2APIC)) {
    Status = RegisterCpuFeature (
               "X2Apic",
               X2ApicGetConfigData,
               X2ApicSupport,
               X2ApicInitialize,
               CPU_FEATURE_X2APIC,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_PPIN)) {
    Status = RegisterCpuFeature (
               "PPIN",
               PpinGetConfigData,
               PpinSupport,
               PpinInitialize,
               CPU_FEATURE_PPIN,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_LMCE)) {
    Status = RegisterCpuFeature (
               "LMCE",
               NULL,
               LmceSupport,
               LmceInitialize,
               CPU_FEATURE_LMCE,
               CPU_FEATURE_LOCK_FEATURE_CONTROL_REGISTER | CPU_FEATURE_THREAD_BEFORE,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  if (IsCpuFeatureSupported (CPU_FEATURE_PROC_TRACE)) {
    Status = RegisterCpuFeature (
               "Proc Trace",
               ProcTraceGetConfigData,
               ProcTraceSupport,
               ProcTraceInitialize,
               CPU_FEATURE_PROC_TRACE,
               CPU_FEATURE_END
               );
    ASSERT_EFI_ERROR (Status);
  }

  return RETURN_SUCCESS;
}
