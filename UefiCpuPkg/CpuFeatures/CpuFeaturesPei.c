/** @file
  CPU Features PEIM driver to initialize CPU features.

  Copyright (c) 2017, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiPei.h>

#include <Library/BaseLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/RegisterCpuFeaturesLib.h>
#include <Library/HobLib.h>

#include <Guid/CpuFeaturesInitDone.h>

EFI_PEI_PPI_DESCRIPTOR           mPeiCpuFeaturesInitDonePpiDesc = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEdkiiCpuFeaturesInitDoneGuid,
  NULL
};

/**
  CPU Features driver entry point function.

  It will perform CPU features initialization, except for
  PcdCpuFeaturesInitOnS3Resume is FALSE on S3 resume.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   CPU Features is initialized successfully.
**/
EFI_STATUS
EFIAPI
CpuFeaturesPeimInitialize (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS                 Status;
  EFI_BOOT_MODE              BootMode;

  Status = PeiServicesGetBootMode (&BootMode);
  ASSERT_EFI_ERROR (Status);

  if (BootMode == BOOT_ON_S3_RESUME &&
      !PcdGetBool (PcdCpuFeaturesInitOnS3Resume)) {
    //
    // Does nothing when if PcdCpuFeaturesInitOnS3Resume is FLASE
    // on S3 boot mode
    //
    return EFI_SUCCESS;
  }

  CpuFeaturesDetect ();

  CpuFeaturesInitialize ();

  //
  // Install CPU Features Init Done PPI
  //
  Status = PeiServicesInstallPpi(&mPeiCpuFeaturesInitDonePpiDesc);
  ASSERT_EFI_ERROR (Status);

  //
  // Build HOB to let CpuFeatureDxe driver skip the initialization process.
  //
  BuildGuidHob (&gEdkiiCpuFeaturesInitDoneGuid, 0);

  return EFI_SUCCESS;
}

