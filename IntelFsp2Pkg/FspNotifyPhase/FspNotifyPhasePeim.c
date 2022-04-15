/** @file
  Source file for FSP notify phase PEI module

  Copyright (c) 2016 - 2022, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include "FspNotifyPhasePeim.h"

/**

   This function waits for FSP notify.

   @param This          Entry point for DXE IPL PPI.
   @param PeiServices   General purpose services available to every PEIM.
   @param HobList       Address to the Pei HOB list.

   @return EFI_SUCCESS              This function never returns.

**/
EFI_STATUS
EFIAPI
WaitForNotify (
  IN CONST EFI_DXE_IPL_PPI  *This,
  IN EFI_PEI_SERVICES       **PeiServices,
  IN EFI_PEI_HOB_POINTERS   HobList
  );

CONST EFI_DXE_IPL_PPI  mDxeIplPpi = {
  WaitForNotify
};

CONST EFI_PEI_PPI_DESCRIPTOR  mInstallDxeIplPpi = {
  EFI_PEI_PPI_DESCRIPTOR_PPI,
  &gEfiDxeIplPpiGuid,
  (VOID *)&mDxeIplPpi
};

CONST EFI_PEI_PPI_DESCRIPTOR  gEndOfPeiSignalPpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gEfiEndOfPeiSignalPpiGuid,
  NULL
};

CONST EFI_PEI_PPI_DESCRIPTOR  gFspReadyForNotifyPhasePpi = {
  (EFI_PEI_PPI_DESCRIPTOR_PPI | EFI_PEI_PPI_DESCRIPTOR_TERMINATE_LIST),
  &gFspReadyForNotifyPhasePpiGuid,
  NULL
};

/**

   This function waits for FSP notify.

   @param This          Entry point for DXE IPL PPI.
   @param PeiServices   General purpose services available to every PEIM.
   @param HobList       Address to the Pei HOB list.

   @return EFI_SUCCESS              This function never returns.

**/
EFI_STATUS
EFIAPI
WaitForNotify (
  IN CONST EFI_DXE_IPL_PPI  *This,
  IN EFI_PEI_SERVICES       **PeiServices,
  IN EFI_PEI_HOB_POINTERS   HobList
  )
{
  EFI_STATUS  Status;

  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FSP HOB is located at 0x%08X\n", HobList));

  //
  // End of PEI phase signal
  //
  Status = PeiServicesInstallPpi (&gEndOfPeiSignalPpi);
  ASSERT_EFI_ERROR (Status);

  //
  // Give control back to BootLoader after FspSiliconInit
  //
  DEBUG ((DEBUG_INFO | DEBUG_INIT, "FSP is waiting for NOTIFY\n"));
  FspSiliconInitDone2 (EFI_SUCCESS);

  //
  // BootLoader called FSP again through NotifyPhase
  //
  FspWaitForNotify ();

  if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
    //
    // Should not come here
    //
    while (TRUE) {
      DEBUG ((DEBUG_ERROR, "No FSP API should be called after FSP is DONE!\n"));
      SetFspApiReturnStatus (EFI_UNSUPPORTED);
      Pei2LoaderSwitchStack ();
    }
  }

  return EFI_SUCCESS;
}

/**
  FSP notify phase PEI module entry point

  @param[in]  FileHandle           Not used.
  @param[in]  PeiServices          General purpose services available to every PEIM.

  @retval     EFI_SUCCESS          The function completes successfully
  @retval     EFI_OUT_OF_RESOURCES Insufficient resources to create database
**/
EFI_STATUS
EFIAPI
FspNotifyPhasePeimEntryPoint (
  IN       EFI_PEI_FILE_HANDLE  FileHandle,
  IN CONST EFI_PEI_SERVICES     **PeiServices
  )
{
  EFI_STATUS              Status;
  VOID                    *OldDxeIplPpi;
  EFI_PEI_PPI_DESCRIPTOR  *OldDescriptor;

  DEBUG ((DEBUG_INFO | DEBUG_INIT, "The entry of FspNotificationPeim\n"));

  if (GetFspGlobalDataPointer ()->FspMode == FSP_IN_API_MODE) {
    //
    // Locate old DXE IPL PPI
    //
    Status = PeiServicesLocatePpi (
               &gEfiDxeIplPpiGuid,
               0,
               &OldDescriptor,
               &OldDxeIplPpi
               );
    ASSERT_EFI_ERROR (Status);

    //
    // Re-install the DXE IPL PPI to wait for notify
    //
    Status = PeiServicesReInstallPpi (OldDescriptor, &mInstallDxeIplPpi);
    ASSERT_EFI_ERROR (Status);
  } else {
    Status = PeiServicesInstallPpi (&gFspReadyForNotifyPhasePpi);
    ASSERT_EFI_ERROR (Status);
  }

  return EFI_SUCCESS;
}
