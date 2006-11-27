/*++

Copyright (c) 2006, Intel Corporation
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

EFI_STATUS
CreateArchSpecificHobs (
  OUT EFI_PHYSICAL_ADDRESS      *BspStore
  )
/*++

Routine Description:

  Creates architecture-specific HOBs.

  Note: New parameters should NOT be added for any HOBs that are added to this
        function.  BspStore is a special case because it is required for the
        call to SwitchStacks() in DxeLoad().

Arguments:

  BspStore    - The address of the BSP Store for those architectures that need
                it.  Otherwise 0.

Returns:

  EFI_SUCCESS   - The HOBs were created successfully.

--*/
{
  EFI_STATUS  Status;

  Status = EFI_SUCCESS;

  ASSERT (NULL != BspStore);

  //
  // Allocate 16KB for the BspStore
  //
  Status = PeiServicesAllocatePages (EfiBootServicesData, EFI_SIZE_TO_PAGES (BSP_STORE_SIZE), BspStore);
  if (EFI_ERROR (Status)) {
    return Status;
  }

  BuildBspStoreHob (
    *BspStore,
    BSP_STORE_SIZE,
    EfiBootServicesData
    );

  return EFI_SUCCESS;
}

/**
  Transfers control to a function starting with a new stack.

  Transfers control to the function specified by EntryPoint using the new stack
  specified by NewStack and passing in the parameters specified by Context1 and
  Context2. Context1 and Context2 are optional and may be NULL. The function
  EntryPoint must never return.

  If EntryPoint is NULL, then ASSERT().
  If NewStack is NULL, then ASSERT().

  @param  EntryPoint  A pointer to function to call with the new stack.
  @param  Context1    A pointer to the context to pass into the EntryPoint
                      function.
  @param  Context2    A pointer to the context to pass into the EntryPoint
                      function.
  @param  NewStack    A pointer to the new stack to use for the EntryPoint
                      function.
  @param  NewBsp      A pointer to the new BSP for the EntryPoint on IPF. It's
                      Reserved on other architectures.

**/
VOID
EFIAPI
SwitchIplStacks (
  IN      SWITCH_STACK_ENTRY_POINT  EntryPoint,
  IN      VOID                      *Context1,  OPTIONAL
  IN      VOID                      *Context2,  OPTIONAL
  IN      VOID                      *NewStack,
  IN      VOID                      *NewBsp
  )
{
  AsmSwitchStackAndBackingStore (
    EntryPoint,
    Context1,
    Context2,
    NewStack,
    NewBsp
    );
}
