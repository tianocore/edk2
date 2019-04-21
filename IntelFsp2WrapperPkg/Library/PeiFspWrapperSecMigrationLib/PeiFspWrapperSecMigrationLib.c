/** @file
  Migrates FSP wrapper SEC structures after permanent memory is installed.

Copyright (c) 2019, Intel Corporation. All rights reserved.<BR>

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Pi/PiPeiCis.h>
#include <Library/BaseLib.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/PeiServicesLib.h>
#include <Library/PeiServicesTablePointerLib.h>
#include <Ppi/RepublishSecPpi.h>
#include <Ppi/TopOfTemporaryRam.h>

GLOBAL_REMOVE_IF_UNREFERENCED EFI_PEI_PPI_DESCRIPTOR mTopOfMemoryRamPpiDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gTopOfTemporaryRamPpiGuid,
  NULL
  };

/**
  This interface re-installs PPIs installed in SecCore from a post-memory PEIM.

  This is to allow a platform that may not support relocation of SecCore to update the PPI instance to a post-memory
  copy from a PEIM that has been shadowed to permanent memory.

  @retval EFI_SUCCESS    The SecCore PPIs were re-installed successfully.
  @retval Others         An error occurred re-installing the SecCore PPIs.

**/
EFI_STATUS
EFIAPI
RepublishFspWrapperSecPpis (
  VOID
  )
{
  EFI_STATUS                            Status;
  EFI_PEI_PPI_DESCRIPTOR                *PeiPpiDescriptor;
  VOID                                  *PeiPpi;

  Status = PeiServicesLocatePpi (
             &gTopOfTemporaryRamPpiGuid,
             0,
             &PeiPpiDescriptor,
             (VOID **) &PeiPpi
             );
  if (!EFI_ERROR (Status)) {
    mTopOfMemoryRamPpiDescriptor.Ppi = PeiPpiDescriptor->Ppi;
    Status = PeiServicesReInstallPpi (
               PeiPpiDescriptor,
               &mTopOfMemoryRamPpiDescriptor
               );
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}

GLOBAL_REMOVE_IF_UNREFERENCED STATIC REPUBLISH_SEC_PPI_PPI  mEdkiiRepublishSecPpiPpi = {
                                                                    RepublishFspWrapperSecPpis
                                                                    };

GLOBAL_REMOVE_IF_UNREFERENCED STATIC EFI_PEI_PPI_DESCRIPTOR mEdkiiRepublishSecPpiDescriptor = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gRepublishSecPpiPpiGuid,
  &mEdkiiRepublishSecPpiPpi
  };

/**
  The constructor installs an instance of REPUBLISH_SEC_PPI_PPI to migrate FSP wrapper SEC
  structures to permanent memory.

  @param  FileHandle   The handle of FFS header the loaded driver.
  @param  PeiServices  The pointer to the PEI services.

  @retval EFI_SUCCESS  The constructor always returns EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
PeiFspWrapperSecMigrationLibConstructor (
  IN EFI_PEI_FILE_HANDLE        FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS  Status;

  Status = PeiServicesInstallPpi (&mEdkiiRepublishSecPpiDescriptor);
  ASSERT_EFI_ERROR (Status);

  return Status;
}