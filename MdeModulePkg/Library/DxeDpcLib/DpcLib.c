/** @file
  Help functions to access UDP service.
  
Copyright (c) 2005 - 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/

#include <Uefi.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Dpc.h>

//
// Pointer to the DPC Protocol
//
EFI_DPC_PROTOCOL  *mDpc;

/**
  This constructor function caches the EFI_DPC_PROTOCOL pointer.

  @param[in] ImageHandle The firmware allocated handle for the EFI image.
  @param[in] SystemTable A pointer to the EFI System Table.

  @retval EFI_SUCCESS The constructor always return EFI_SUCCESS.

**/
EFI_STATUS
EFIAPI
DpcLibConstructor (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  //
  // Locate the EFI_DPC_PROTOCOL in the handle database
  //
  Status = gBS->LocateProtocol (&gEfiDpcProtocolGuid, NULL, (VOID **)&mDpc);
  ASSERT_EFI_ERROR (Status);

  return EFI_SUCCESS;
}

/**
  Add a Deferred Procedure Call to the end of the DPC queue.

  @param[in]  DpcTpl        The EFI_TPL that the DPC should be invoked.
  @param[in]  DpcProcedure  Pointer to the DPC's function.
  @param[in]  DpcContext    Pointer to the DPC's context.  Passed to DpcProcedure
                            when DpcProcedure is invoked.

  @retval EFI_SUCCESS            The DPC was queued.
  @retval EFI_INVALID_PARAMETER  DpcTpl is not a valid EFI_TPL.
  @retval EFI_INVALID_PARAMETER  DpcProcedure is NULL.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to
                                 add the DPC to the queue.

**/
EFI_STATUS
EFIAPI
QueueDpc (
  IN EFI_TPL            DpcTpl,
  IN EFI_DPC_PROCEDURE  DpcProcedure,
  IN VOID               *DpcContext    OPTIONAL
  )
{
  //
  // Call the EFI_DPC_PROTOCOL to queue the DPC
  //
  return mDpc->QueueDpc (mDpc, DpcTpl, DpcProcedure, DpcContext);
}

/**
  Dispatch the queue of DPCs.  ALL DPCs that have been queued with a DpcTpl
  value greater than or equal to the current TPL are invoked in the order that
  they were queued.  DPCs with higher DpcTpl values are invoked before DPCs with
  lower DpcTpl values.

  @retval EFI_SUCCESS    One or more DPCs were invoked.
  @retval EFI_NOT_FOUND  No DPCs were invoked.

**/
EFI_STATUS
EFIAPI
DispatchDpc (
  VOID
  )
{
  //
  // Call the EFI_DPC_PROTOCOL to dispatch previously queued DPCs
  //
  return mDpc->DispatchDpc (mDpc);
}
