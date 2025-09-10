/**@file
  Install a callback when necessary for setting the Feature Control MSR on all
  processors.

  Copyright (C) 2016, Red Hat, Inc.

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/DebugLib.h>
#include <Library/HobLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/QemuFwCfgLib.h>
#include <Ppi/MpServices.h>
#include <Register/ArchitecturalMsr.h>
#include <IndustryStandard/Tdx.h>

#include "Platform.h"

/**
  Write the Feature Control MSR on an Application Processor or the Boot
  Processor.

  All APs execute this function in parallel. The BSP executes the function
  separately.

  @param[in,out] WorkSpace  Pointer to the input/output argument workspace
                            shared by all processors.
**/
STATIC
VOID
EFIAPI
WriteFeatureControl (
  IN OUT VOID  *WorkSpace
  )
{
  EFI_HOB_PLATFORM_INFO  *PlatformInfoHob = WorkSpace;

  if (TdIsEnabled ()) {
    TdVmCall (
      TDVMCALL_WRMSR,
      (UINT64)MSR_IA32_FEATURE_CONTROL,
      PlatformInfoHob->FeatureControlValue,
      0,
      0,
      0
      );
  } else {
    AsmWriteMsr64 (
      MSR_IA32_FEATURE_CONTROL,
      PlatformInfoHob->FeatureControlValue
      );
  }
}

/**
  Notification function called when EFI_PEI_MP_SERVICES_PPI becomes available.

  @param[in] PeiServices      Indirect reference to the PEI Services Table.
  @param[in] NotifyDescriptor Address of the notification descriptor data
                              structure.
  @param[in] Ppi              Address of the PPI that was installed.

  @return  Status of the notification. The status code returned from this
           function is ignored.
**/
STATIC
EFI_STATUS
EFIAPI
OnMpServicesAvailable (
  IN EFI_PEI_SERVICES           **PeiServices,
  IN EFI_PEI_NOTIFY_DESCRIPTOR  *NotifyDescriptor,
  IN VOID                       *Ppi
  )
{
  EFI_PEI_MP_SERVICES_PPI  *MpServices;
  EFI_STATUS               Status;
  EFI_HOB_PLATFORM_INFO    *PlatformInfoHob;
  EFI_HOB_GUID_TYPE        *GuidHob;

  GuidHob = GetFirstGuidHob (&gUefiOvmfPkgPlatformInfoGuid);
  if (GuidHob == NULL) {
    return EFI_UNSUPPORTED;
  }

  PlatformInfoHob = (EFI_HOB_PLATFORM_INFO *)GET_GUID_HOB_DATA (GuidHob);

  DEBUG ((DEBUG_VERBOSE, "%a: %a\n", gEfiCallerBaseName, __func__));

  //
  // Write the MSR on all the APs in parallel.
  //
  MpServices = Ppi;
  Status     = MpServices->StartupAllAPs (
                             (CONST EFI_PEI_SERVICES **)PeiServices,
                             MpServices,
                             WriteFeatureControl, // Procedure
                             FALSE,               // SingleThread
                             0,                   // TimeoutInMicroSeconds: inf.
                             PlatformInfoHob      // ProcedureArgument
                             );
  if (EFI_ERROR (Status) && (Status != EFI_NOT_STARTED)) {
    DEBUG ((DEBUG_ERROR, "%a: StartupAllAps(): %r\n", __func__, Status));
    return Status;
  }

  //
  // Now write the MSR on the BSP too.
  //
  WriteFeatureControl (PlatformInfoHob);
  return EFI_SUCCESS;
}

//
// Notification object for registering the callback, for when
// EFI_PEI_MP_SERVICES_PPI becomes available.
//
STATIC CONST EFI_PEI_NOTIFY_DESCRIPTOR  mMpServicesNotify = {
  EFI_PEI_PPI_DESCRIPTOR_NOTIFY_CALLBACK | // Flags
  EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST,
  &gEfiPeiMpServicesPpiGuid,               // Guid
  OnMpServicesAvailable                    // Notify
};

VOID
InstallFeatureControlCallback (
  IN OUT EFI_HOB_PLATFORM_INFO  *PlatformInfoHob
  )
{
  EFI_STATUS            Status;
  FIRMWARE_CONFIG_ITEM  FwCfgItem;
  UINTN                 FwCfgSize;

  Status = QemuFwCfgFindFile (
             "etc/msr_feature_control",
             &FwCfgItem,
             &FwCfgSize
             );
  if (EFI_ERROR (Status) || (FwCfgSize != sizeof (PlatformInfoHob->FeatureControlValue))) {
    //
    // Nothing to do.
    //
    return;
  }

  PlatformInfoHob->FeatureControl = TRUE;
  QemuFwCfgSelectItem (FwCfgItem);
  QemuFwCfgReadBytes (
    sizeof (PlatformInfoHob->FeatureControlValue),
    &(PlatformInfoHob->FeatureControlValue)
    );

  Status = PeiServicesNotifyPpi (&mMpServicesNotify);
  if (EFI_ERROR (Status)) {
    DEBUG ((
      DEBUG_ERROR,
      "%a: failed to set up MP Services callback: %r\n",
      __func__,
      Status
      ));
  }
}
