/*++

Copyright (c) 2006 - 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  IpfDxeLoad.c

Abstract:

  Ipf-specifc functionality for DxeLoad.

--*/

#include "DxeIpl.h"

VOID
HandOffToDxeCore (
  IN EFI_PHYSICAL_ADDRESS   DxeCoreEntryPoint,
  IN EFI_PEI_HOB_POINTERS   HobList,
  IN EFI_PEI_PPI_DESCRIPTOR *EndOfPeiSignal
  )
{
  VOID                *BaseOfStack;
  VOID                *TopOfStack;
  VOID                *BspStore;
  EFI_STATUS          Status;

  //
  // Allocate 128KB for the Stack
  //
  BaseOfStack = AllocatePages (EFI_SIZE_TO_PAGES (STACK_SIZE));
  ASSERT (BaseOfStack != NULL);

  //
  // Allocate 16KB for the BspStore
  //
  BspStore    = AllocatePages (EFI_SIZE_TO_PAGES (BSP_STORE_SIZE));
  ASSERT (BspStore != NULL);
  //
  // Build BspStoreHob
  //
  BuildBspStoreHob ((EFI_PHYSICAL_ADDRESS) (UINTN) BspStore, BSP_STORE_SIZE, EfiBootServicesData);

  //
  // Compute the top of the stack we were allocated. Pre-allocate a UINTN
  // for safety.
  //
  TopOfStack = (VOID *) ((UINTN) BaseOfStack + EFI_SIZE_TO_PAGES (STACK_SIZE) * EFI_PAGE_SIZE - CPU_STACK_ALIGNMENT);
  TopOfStack = ALIGN_POINTER (TopOfStack, CPU_STACK_ALIGNMENT);

  //
  // End of PEI phase singal
  //
  Status = PeiServicesInstallPpi (EndOfPeiSignal);
  ASSERT_EFI_ERROR (Status);

  SwitchStack (
    (SWITCH_STACK_ENTRY_POINT)(UINTN)DxeCoreEntryPoint,
    HobList.Raw,
    NULL,
    TopOfStack,
    BspStore
    );
}
