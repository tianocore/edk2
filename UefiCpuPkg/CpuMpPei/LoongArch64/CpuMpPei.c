/** @file
  CPU PEI Module installs CPU Multiple Processor PPI.

  Copyright (c) 2015 - 2022, Intel Corporation. All rights reserved.<BR>
  Copyright (c) 2025, Loongson Technology Corporation Limited. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "CpuMpPei.h"

EFI_PEI_PPI_DESCRIPTOR  mPeiCpuMpPpiList[] = {
  {
    EFI_PEI_PPI_DESCRIPTOR_PPI,
    &gEdkiiPeiMpServices2PpiGuid,
    &mMpServices2Ppi
  },
  {
    (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
    &gEfiPeiMpServicesPpiGuid,
    &mMpServicesPpi
  }
};

/**
  Initializes MP and exceptions handlers.

  @param  PeiServices     The pointer to the PEI Services Table.

  @retval EFI_SUCCESS     MP was successfully initialized.
  @retval others          Error occurred in MP initialization.

**/
EFI_STATUS
InitializeCpuMpWorker (
  IN CONST EFI_PEI_SERVICES  **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = MpInitLibInitialize ();
  if (EFI_ERROR (Status)) {
    return Status;
  }

  //
  // Install CPU MP PPI
  //
  Status = PeiServicesInstallPpi (mPeiCpuMpPpiList);
  ASSERT_EFI_ERROR (Status);

  return Status;
}

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
  EFI_STATUS  Status;

  Status = InitializeCpuMpWorker ((CONST EFI_PEI_SERVICES **)PeiServices);
  ASSERT_EFI_ERROR (Status);

  return Status;
}
