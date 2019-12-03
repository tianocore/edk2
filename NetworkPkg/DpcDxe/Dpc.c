/** @file

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

Module Name:

  Dpc.c

Abstract:


**/

#include "Dpc.h"

//
// Handle for the EFI_DPC_PROTOCOL instance
//
EFI_HANDLE  mDpcHandle = NULL;

//
// The EFI_DPC_PROTOCOL instances that is installed onto mDpcHandle
//
EFI_DPC_PROTOCOL mDpc = {
  DpcQueueDpc,
  DpcDispatchDpc
};

//
// Global variables used to meaasure the DPC Queue Depths
//
UINTN  mDpcQueueDepth = 0;
UINTN  mMaxDpcQueueDepth = 0;

//
// Free list of DPC entries.  As DPCs are queued, entries are removed from this
// free list.  As DPC entries are dispatched, DPC entries are added to the free list.
// If the free list is empty and a DPC is queued, the free list is grown by allocating
// an additional set of DPC entries.
//
LIST_ENTRY      mDpcEntryFreeList = INITIALIZE_LIST_HEAD_VARIABLE(mDpcEntryFreeList);

//
// An array of DPC queues.  A DPC queue is allocated for every leval EFI_TPL value.
// As DPCs are queued, they are added to the end of the linked list.
// As DPCs are dispatched, they are removed from the beginning of the linked list.
//
LIST_ENTRY      mDpcQueue[TPL_HIGH_LEVEL + 1];

/**
  Add a Deferred Procedure Call to the end of the DPC queue.

  @param  This          Protocol instance pointer.
  @param  DpcTpl        The EFI_TPL that the DPC should be invoked.
  @param  DpcProcedure  Pointer to the DPC's function.
  @param  DpcContext    Pointer to the DPC's context.  Passed to DpcProcedure
                        when DpcProcedure is invoked.

  @retval EFI_SUCCESS            The DPC was queued.
  @retval EFI_INVALID_PARAMETER  DpcTpl is not a valid EFI_TPL.
  @retval EFI_INVALID_PARAMETER  DpcProcedure is NULL.
  @retval EFI_OUT_OF_RESOURCES   There are not enough resources available to
                                 add the DPC to the queue.

**/
EFI_STATUS
EFIAPI
DpcQueueDpc (
  IN EFI_DPC_PROTOCOL   *This,
  IN EFI_TPL            DpcTpl,
  IN EFI_DPC_PROCEDURE  DpcProcedure,
  IN VOID               *DpcContext    OPTIONAL
  )
{
  EFI_STATUS  ReturnStatus;
  EFI_TPL     OriginalTpl;
  DPC_ENTRY   *DpcEntry;
  UINTN       Index;

  //
  // Make sure DpcTpl is valid
  //
  if (DpcTpl < TPL_APPLICATION || DpcTpl > TPL_HIGH_LEVEL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Make sure DpcProcedure is valid
  //
  if (DpcProcedure == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Assume this function will succeed
  //
  ReturnStatus = EFI_SUCCESS;

  //
  // Raise the TPL level to TPL_HIGH_LEVEL for DPC list operation and save the
  // current TPL value so it can be restored when this function returns.
  //
  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Check to see if there are any entries in the DPC free list
  //
  if (IsListEmpty (&mDpcEntryFreeList)) {
    //
    // If the current TPL is greater than TPL_NOTIFY, then memory allocations
    // can not be performed, so the free list can not be expanded.  In this case
    // return EFI_OUT_OF_RESOURCES.
    //
    if (OriginalTpl > TPL_NOTIFY) {
      ReturnStatus = EFI_OUT_OF_RESOURCES;
      goto Done;
    }

    //
    // Add 64 DPC entries to the free list
    //
    for (Index = 0; Index < 64; Index++) {
      //
      // Lower the TPL level to perform a memory allocation
      //
      gBS->RestoreTPL (OriginalTpl);

      //
      // Allocate a new DPC entry
      //
      DpcEntry = AllocatePool (sizeof (DPC_ENTRY));

      //
      // Raise the TPL level back to TPL_HIGH_LEVEL for DPC list operations
      //
      gBS->RaiseTPL (TPL_HIGH_LEVEL);

      //
      // If the allocation of a DPC entry fails, and the free list is empty,
      // then return EFI_OUT_OF_RESOURCES.
      //
      if (DpcEntry == NULL) {
        if (IsListEmpty (&mDpcEntryFreeList)) {
          ReturnStatus = EFI_OUT_OF_RESOURCES;
          goto Done;
        }
      }

      //
      // Add the newly allocated DPC entry to the DPC free list
      //
      InsertTailList (&mDpcEntryFreeList, &DpcEntry->ListEntry);
    }
  }

  //
  // Retrieve the first node from the free list of DPCs
  //
  DpcEntry = (DPC_ENTRY *)(GetFirstNode (&mDpcEntryFreeList));

  //
  // Remove the first node from the free list of DPCs
  //
  RemoveEntryList (&DpcEntry->ListEntry);

  //
  // Fill in the DPC entry with the DpcProcedure and DpcContext
  //
  DpcEntry->DpcProcedure = DpcProcedure;
  DpcEntry->DpcContext   = DpcContext;

  //
  // Add the DPC entry to the end of the list for the specified DplTpl.
  //
  InsertTailList (&mDpcQueue[DpcTpl], &DpcEntry->ListEntry);

  //
  // Increment the measured DPC queue depth across all TPLs
  //
  mDpcQueueDepth++;

  //
  // Measure the maximum DPC queue depth across all TPLs
  //
  if (mDpcQueueDepth > mMaxDpcQueueDepth) {
    mMaxDpcQueueDepth = mDpcQueueDepth;
  }

Done:
  //
  // Restore the original TPL level when this function was called
  //
  gBS->RestoreTPL (OriginalTpl);

  return ReturnStatus;
}

/**
  Dispatch the queue of DPCs.  ALL DPCs that have been queued with a DpcTpl
  value greater than or equal to the current TPL are invoked in the order that
  they were queued.  DPCs with higher DpcTpl values are invoked before DPCs with
  lower DpcTpl values.

  @param  This  Protocol instance pointer.

  @retval EFI_SUCCESS    One or more DPCs were invoked.
  @retval EFI_NOT_FOUND  No DPCs were invoked.

**/
EFI_STATUS
EFIAPI
DpcDispatchDpc (
  IN EFI_DPC_PROTOCOL  *This
  )
{
  EFI_STATUS  ReturnStatus;
  EFI_TPL     OriginalTpl;
  EFI_TPL     Tpl;
  DPC_ENTRY   *DpcEntry;

  //
  // Assume that no DPCs will be invoked
  //
  ReturnStatus = EFI_NOT_FOUND;

  //
  // Raise the TPL level to TPL_HIGH_LEVEL for DPC list operation and save the
  // current TPL value so it can be restored when this function returns.
  //
  OriginalTpl = gBS->RaiseTPL (TPL_HIGH_LEVEL);

  //
  // Check to see if there are 1 or more DPCs currently queued
  //
  if (mDpcQueueDepth > 0) {
    //
    // Loop from TPL_HIGH_LEVEL down to the current TPL value
    //
    for (Tpl = TPL_HIGH_LEVEL; Tpl >= OriginalTpl; Tpl--) {
      //
      // Check to see if the DPC queue is empty
      //
      while (!IsListEmpty (&mDpcQueue[Tpl])) {
        //
        // Retrieve the first DPC entry from the DPC queue specified by Tpl
        //
        DpcEntry = (DPC_ENTRY *)(GetFirstNode (&mDpcQueue[Tpl]));

        //
        // Remove the first DPC entry from the DPC queue specified by Tpl
        //
        RemoveEntryList (&DpcEntry->ListEntry);

        //
        // Decrement the measured DPC Queue Depth across all TPLs
        //
        mDpcQueueDepth--;

        //
        // Lower the TPL to TPL value of the current DPC queue
        //
        gBS->RestoreTPL (Tpl);

        //
        // Invoke the DPC passing in its context
        //
        (DpcEntry->DpcProcedure) (DpcEntry->DpcContext);

        //
        // At least one DPC has been invoked, so set the return status to EFI_SUCCESS
        //
        ReturnStatus = EFI_SUCCESS;

        //
        // Raise the TPL level back to TPL_HIGH_LEVEL for DPC list operations
        //
        gBS->RaiseTPL (TPL_HIGH_LEVEL);

        //
        // Add the invoked DPC entry to the DPC free list
        //
        InsertTailList (&mDpcEntryFreeList, &DpcEntry->ListEntry);
      }
    }
  }

  //
  // Restore the original TPL level when this function was called
  //
  gBS->RestoreTPL (OriginalTpl);

  return ReturnStatus;
}

/**
  The entry point for DPC driver which installs the EFI_DPC_PROTOCOL onto a new handle.

  @param  ImageHandle            The image handle of the driver.
  @param  SystemTable            The system table.

  @retval EFI_SUCCES             The DPC queues were initialized and the EFI_DPC_PROTOCOL was
                                 installed onto a new handle.
  @retval Others                 Failed to install EFI_DPC_PROTOCOL.

**/
EFI_STATUS
EFIAPI
DpcDriverEntryPoint (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;
  UINTN       Index;

  //
  // ASSERT() if the EFI_DPC_PROTOCOL is already present in the handle database
  //
  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiDpcProtocolGuid);

  //
  // Initialize the DPC queue for all possible TPL values
  //
  for (Index = 0; Index <= TPL_HIGH_LEVEL; Index++) {
    InitializeListHead (&mDpcQueue[Index]);
  }

  //
  // Install the EFI_DPC_PROTOCOL instance onto a new handle
  //
  Status = gBS->InstallMultipleProtocolInterfaces (
                  &mDpcHandle,
                  &gEfiDpcProtocolGuid,
                  &mDpc,
                  NULL
                  );
  ASSERT_EFI_ERROR (Status);

  return Status;
}
