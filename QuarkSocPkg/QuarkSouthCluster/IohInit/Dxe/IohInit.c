/** @file
QuarkSCSocId module initialization module

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
