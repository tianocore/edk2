/** @file
  CPU Features DXE driver to initialize CPU features.

  Copyright (c) 2017 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/RegisterCpuFeaturesLib.h>
#include <Library/HobLib.h>

#include <Protocol/SmmConfiguration.h>
#include <Guid/CpuFeaturesInitDone.h>

/**
  Worker function to perform CPU feature initialization.

**/
VOID
CpuFeaturesInitializeWorker (
  VOID
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  CpuFeaturesDetect ();

  CpuFeaturesInitialize ();

  //
  // Install CPU Features Init Done Protocol
  //
  Handle = NULL;
  Status = gBS->InstallProtocolInterface (
                  &Handle,
                  &gEdkiiCpuFeaturesInitDoneGuid,
                  EFI_NATIVE_INTERFACE,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);
}

/**
  Event notification that initialize CPU features when gEfiSmmConfigurationProtocol installs.

  @param[in]  Event                 The Event that is being processed, not used.
  @param[in]  Context               Event Context, not used.
**/
VOID
EFIAPI
SmmConfigurationEventNotify (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  EFI_STATUS                      Status;
  EFI_SMM_CONFIGURATION_PROTOCOL  *SmmConfiguration;

  //
  // Make sure this notification is for this handler
  //
  Status = gBS->LocateProtocol (&gEfiSmmConfigurationProtocolGuid, NULL, (VOID **)&SmmConfiguration);
  if (EFI_ERROR (Status)) {
    return;
  }

  CpuFeaturesInitializeWorker ();
}

/**
  CPU Features driver entry point function.

  If PcdCpuFeaturesInitAfterSmmRelocation is TRUE, it will register one
  SMM Configuration Protocol notify function to perform CPU features
  initialization. Otherwise, it will perform CPU features initialization
  directly.

  @param ImageHandle     Image handle this driver.
  @param SystemTable     Pointer to the System Table.

  @retval EFI_SUCCESS   CPU Features is initialized successfully.
**/
EFI_STATUS
EFIAPI
CpuFeaturesDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  VOID        *Registration;
  EFI_STATUS  Status;
  EFI_HANDLE  Handle;

  if (GetFirstGuidHob (&gEdkiiCpuFeaturesInitDoneGuid) != NULL) {
    //
    // Try to find HOB first. This HOB exist means CPU features have
    // been initialized by CpuFeaturesPei driver, just install
    // gEdkiiCpuFeaturesInitDoneGuid.
    //
    Handle = NULL;
    Status = gBS->InstallProtocolInterface (
                    &Handle,
                    &gEdkiiCpuFeaturesInitDoneGuid,
                    EFI_NATIVE_INTERFACE,
                    NULL
                    );
    ASSERT_EFI_ERROR (Status);
    return Status;
  }

  if (PcdGetBool (PcdCpuFeaturesInitAfterSmmRelocation)) {
    //
    // Install notification callback on SMM Configuration Protocol
    //
    EfiCreateProtocolNotifyEvent (
      &gEfiSmmConfigurationProtocolGuid,
      TPL_CALLBACK,
      SmmConfigurationEventNotify,
      NULL,
      &Registration
      );
  } else {
    CpuFeaturesInitializeWorker ();
  }

  return EFI_SUCCESS;
}
