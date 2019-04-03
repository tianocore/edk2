/** @file
QuarkSCSocId module initialization module

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "CommonHeader.h"
#include "IohBds.h"

/**
   The entry function for IohInit driver.

   This function just call initialization function.

   @param ImageHandle   The driver image handle for GmchInit driver
   @param SystemTable   The pointer to System Table

   @retval EFI_SUCCESS  Success to initialize every module.
   @return EFI_STATUS   The status of initialization work.

**/
EFI_STATUS
EFIAPI
IohInit (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{

  InitializeIohSsvidSsid(IOH_BUS, IOH_PCI_IOSF2AHB_0_DEV_NUM, 0);

  InitializeIohSsvidSsid(IOH_BUS, IOH_PCI_IOSF2AHB_1_DEV_NUM, 0);

  return EFI_SUCCESS;
}
