/** @file
  CPU PEI Module installs CPU Multiple Processor PPI.

  Copyright (c) 2015 - 2016, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "CpuMpPei.h"
/**
  The Entry point of the MP CPU PEIM.

  This function will wakeup APs and collect CPU AP count and install the
  Mp Service Ppi.

  @param  FileHandle    Handle of the file being invoked.
  @param  PeiServices   Describes the list of possible PEI Services.

  @retval EFI_SUCCESS   MpServicePpi is installed successfully.

**/
EFI_STATUS
EFIAPI
CpuMpPeimInit (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS           Status;
  EFI_VECTOR_HANDOFF_INFO         *VectorInfo;
  EFI_PEI_VECTOR_HANDOFF_INFO_PPI *VectorHandoffInfoPpi;

  //
  // Get Vector Hand-off Info PPI
  //
  VectorInfo = NULL;
  Status = PeiServicesLocatePpi (
             &gEfiVectorHandoffInfoPpiGuid,
             0,
             NULL,
             (VOID **)&VectorHandoffInfoPpi
             );
  if (Status == EFI_SUCCESS) {
    VectorInfo = VectorHandoffInfoPpi->Info;
  }
  Status = InitializeCpuExceptionHandlers (VectorInfo);
  ASSERT_EFI_ERROR (Status);
  
  //
  // Wakeup APs to do initialization
  //
  Status = MpInitLibInitialize ();
  ASSERT_EFI_ERROR (Status);

  //
  // Update and publish CPU BIST information
  //
  CollectBistDataFromPpi (PeiServices);

  //
  // Install CPU MP PPI
  //
  Status = PeiServicesInstallPpi(&mPeiCpuMpPpiDesc);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
