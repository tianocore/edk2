/** @file
  Minimal block driver for Mini-OS.

  Copyright (c) 2007-2008 Samuel Thibault.
  Copyright (C) 2014, Citrix Ltd.
  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent
**/

#include <Library/PrintLib.h>
#include <Library/DebugLib.h>

#include "BlockFront.h"

#include <IndustryStandard/Xen/io/protocols.h>
#include <IndustryStandard/Xen/io/xenbus.h>

/**
  Helper to read an integer from XenStore.

  If the number overflows according to the range defined by UINT64,
  then ASSERT().

  @param This         A pointer to a XENBUS_PROTOCOL instance.
  @param Node         The XenStore node to read from.
  @param FromBackend  Read frontend or backend value.
  @param ValuePtr     Where to put the value.

  @retval XENSTORE_STATUS_SUCCESS  If successful, will update ValuePtr.
  @return                          Any other return value indicate the error,
                                   ValuePtr is not updated in this case.
**/
STATIC
XENSTORE_STATUS
XenBusReadUint64 (
  IN  XENBUS_PROTOCOL  *This,
  IN  CONST CHAR8      *Node,
  IN  BOOLEAN          FromBackend,
  OUT UINT64           *ValuePtr
  )
{
  XENSTORE_STATUS  Status;
  CHAR8            *Ptr;

  if (!FromBackend) {
    Status = This->XsRead (This, XST_NIL, Node, (VOID **)&Ptr);
  } else {
    Status = This->XsBackendRead (This, XST_NIL, Node, (VOID **)&Ptr);
  }

  if (Status != XENSTORE_STATUS_SUCCESS) {
    return Status;
  }

  // AsciiStrDecimalToUint64 will ASSERT if Ptr overflow UINT64.
  *ValuePtr = AsciiStrDecimalToUint64 (Ptr);
  FreePool (Ptr);
  return Status;
}

/**
  Free an instance of XEN_BLOCK_FRONT_DEVICE.

  @param Dev  The instance to free.
**/
STATIC
VOID
XenPvBlockFree (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  )
{
  XENBUS_PROTOCOL  *XenBusIo = Dev->XenBusIo;

  if (Dev->RingRef != 0) {
    XenBusIo->GrantEndAccess (XenBusIo, Dev->RingRef);
  }

  if (Dev->Ring.sring != NULL) {
    FreePages (Dev->Ring.sring, 1);
  }

  if (Dev->EventChannel != 0) {
    XenBusIo->EventChannelClose (XenBusIo, Dev->EventChannel);
  }

  FreePool (Dev);
}

/**
  Wait until the backend has reached the ExpectedState.

  @param Dev            A XEN_BLOCK_FRONT_DEVICE instance.
  @param ExpectedState  The backend state expected.
  @param LastStatePtr   An optional pointer where to right the final state.

  @return Return XENSTORE_STATUS_SUCCESS if the new backend state is ExpectedState
          or return an error otherwise.
**/
STATIC
XENSTORE_STATUS
XenPvBlkWaitForBackendState (
  IN  XEN_BLOCK_FRONT_DEVICE  *Dev,
  IN  XenbusState             ExpectedState,
  OUT XenbusState             *LastStatePtr OPTIONAL
  )
{
  XENBUS_PROTOCOL  *XenBusIo = Dev->XenBusIo;
  XenbusState      State;
  UINT64           Value;
  XENSTORE_STATUS  Status = XENSTORE_STATUS_SUCCESS;

  while (TRUE) {
    Status = XenBusReadUint64 (XenBusIo, "state", TRUE, &Value);
    if (Status != XENSTORE_STATUS_SUCCESS) {
      return Status;
    }

    if (Value > XenbusStateReconfigured) {
      //
      // Value is not a State value.
      //
      return XENSTORE_STATUS_EIO;
    }

    State = Value;
    if (State == ExpectedState) {
      break;
    } else if (State > ExpectedState) {
      Status = XENSTORE_STATUS_FAIL;
      break;
    }

    DEBUG ((
      DEBUG_INFO,
      "XenPvBlk: waiting backend state %d, current: %d\n",
      ExpectedState,
      State
      ));
    XenBusIo->WaitForWatch (XenBusIo, Dev->StateWatchToken);
  }

  if (LastStatePtr != NULL) {
    *LastStatePtr = State;
  }

  return Status;
}

EFI_STATUS
XenPvBlockFrontInitialization (
  IN  XENBUS_PROTOCOL         *XenBusIo,
  IN  CONST CHAR8             *NodeName,
  OUT XEN_BLOCK_FRONT_DEVICE  **DevPtr
  )
{
  XENSTORE_TRANSACTION    Transaction;
  CHAR8                   *DeviceType;
  blkif_sring_t           *SharedRing;
  XENSTORE_STATUS         Status;
  XEN_BLOCK_FRONT_DEVICE  *Dev;
  XenbusState             State;
  UINT64                  Value;
  CHAR8                   *Params;

  ASSERT (NodeName != NULL);

  Dev = AllocateZeroPool (sizeof (XEN_BLOCK_FRONT_DEVICE));
  if (Dev == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Dev->Signature = XEN_BLOCK_FRONT_SIGNATURE;
  Dev->NodeName  = NodeName;
  Dev->XenBusIo  = XenBusIo;
  Dev->DeviceId  = XenBusIo->DeviceId;

  XenBusIo->XsRead (XenBusIo, XST_NIL, "device-type", (VOID **)&DeviceType);
  if (AsciiStrCmp (DeviceType, "cdrom") == 0) {
    Dev->MediaInfo.CdRom = TRUE;
  } else {
    Dev->MediaInfo.CdRom = FALSE;
  }

  FreePool (DeviceType);

  if (Dev->MediaInfo.CdRom) {
    Status = XenBusIo->XsBackendRead (XenBusIo, XST_NIL, "params", (VOID **)&Params);
    if (Status != XENSTORE_STATUS_SUCCESS) {
      DEBUG ((DEBUG_ERROR, "%a: Failed to read params (%d)\n", __func__, Status));
      goto Error;
    }

    if ((AsciiStrLen (Params) == 0) || (AsciiStrCmp (Params, "aio:") == 0)) {
      FreePool (Params);
      DEBUG ((DEBUG_INFO, "%a: Empty cdrom\n", __func__));
      goto Error;
    }

    FreePool (Params);
  }

  Status = XenBusReadUint64 (XenBusIo, "backend-id", FALSE, &Value);
  if ((Status != XENSTORE_STATUS_SUCCESS) || (Value > MAX_UINT16)) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: Failed to get backend-id (%d)\n",
      Status
      ));
    goto Error;
  }

  Dev->DomainId = (domid_t)Value;
  XenBusIo->EventChannelAllocate (XenBusIo, Dev->DomainId, &Dev->EventChannel);

  SharedRing = (blkif_sring_t *)AllocatePages (1);
  SHARED_RING_INIT (SharedRing);
  FRONT_RING_INIT (&Dev->Ring, SharedRing, EFI_PAGE_SIZE);
  XenBusIo->GrantAccess (
              XenBusIo,
              Dev->DomainId,
              (INTN)SharedRing >> EFI_PAGE_SHIFT,
              FALSE,
              &Dev->RingRef
              );

Again:
  Status = XenBusIo->XsTransactionStart (XenBusIo, &Transaction);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((DEBUG_WARN, "XenPvBlk: Failed to start transaction, %d\n", Status));
    goto Error;
  }

  Status = XenBusIo->XsPrintf (
                       XenBusIo,
                       &Transaction,
                       NodeName,
                       "ring-ref",
                       "%d",
                       Dev->RingRef
                       );
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "XenPvBlk: Failed to write ring-ref.\n"));
    goto AbortTransaction;
  }

  Status = XenBusIo->XsPrintf (
                       XenBusIo,
                       &Transaction,
                       NodeName,
                       "event-channel",
                       "%d",
                       Dev->EventChannel
                       );
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "XenPvBlk: Failed to write event-channel.\n"));
    goto AbortTransaction;
  }

  Status = XenBusIo->XsPrintf (
                       XenBusIo,
                       &Transaction,
                       NodeName,
                       "protocol",
                       "%a",
                       XEN_IO_PROTO_ABI_NATIVE
                       );
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "XenPvBlk: Failed to write protocol.\n"));
    goto AbortTransaction;
  }

  Status = XenBusIo->SetState (XenBusIo, &Transaction, XenbusStateConnected);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((DEBUG_ERROR, "XenPvBlk: Failed to switch state.\n"));
    goto AbortTransaction;
  }

  Status = XenBusIo->XsTransactionEnd (XenBusIo, &Transaction, FALSE);
  if (Status == XENSTORE_STATUS_EAGAIN) {
    goto Again;
  }

  XenBusIo->RegisterWatchBackend (XenBusIo, "state", &Dev->StateWatchToken);

  //
  // Waiting for backend
  //
  Status = XenPvBlkWaitForBackendState (Dev, XenbusStateConnected, &State);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: backend for %a/%d not available, rc=%d state=%d\n",
      XenBusIo->Type,
      XenBusIo->DeviceId,
      Status,
      State
      ));
    goto Error2;
  }

  Status = XenBusReadUint64 (XenBusIo, "info", TRUE, &Value);
  if ((Status != XENSTORE_STATUS_SUCCESS) || (Value > MAX_UINT32)) {
    goto Error2;
  }

  Dev->MediaInfo.VDiskInfo = (UINT32)Value;
  if (Dev->MediaInfo.VDiskInfo & VDISK_READONLY) {
    Dev->MediaInfo.ReadWrite = FALSE;
  } else {
    Dev->MediaInfo.ReadWrite = TRUE;
  }

  Status = XenBusReadUint64 (XenBusIo, "sectors", TRUE, &Dev->MediaInfo.Sectors);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    goto Error2;
  }

  Status = XenBusReadUint64 (XenBusIo, "sector-size", TRUE, &Value);
  if ((Status != XENSTORE_STATUS_SUCCESS) || (Value > MAX_UINT32)) {
    goto Error2;
  }

  if ((UINT32)Value % 512 != 0) {
    //
    // This is not supported by the driver.
    //
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: Unsupported sector-size value %Lu, "
      "it must be a multiple of 512\n",
      Value
      ));
    goto Error2;
  }

  Dev->MediaInfo.SectorSize = (UINT32)Value;

  // Default value
  Value = 0;
  XenBusReadUint64 (XenBusIo, "feature-barrier", TRUE, &Value);
  if (Value == 1) {
    Dev->MediaInfo.FeatureBarrier = TRUE;
  } else {
    Dev->MediaInfo.FeatureBarrier = FALSE;
  }

  // Default value
  Value = 0;
  XenBusReadUint64 (XenBusIo, "feature-flush-cache", TRUE, &Value);
  if (Value == 1) {
    Dev->MediaInfo.FeatureFlushCache = TRUE;
  } else {
    Dev->MediaInfo.FeatureFlushCache = FALSE;
  }

  DEBUG ((
    DEBUG_INFO,
    "XenPvBlk: New disk with %ld sectors of %d bytes\n",
    Dev->MediaInfo.Sectors,
    Dev->MediaInfo.SectorSize
    ));

  *DevPtr = Dev;
  return EFI_SUCCESS;

Error2:
  XenBusIo->UnregisterWatch (XenBusIo, Dev->StateWatchToken);
  XenBusIo->XsRemove (XenBusIo, XST_NIL, "ring-ref");
  XenBusIo->XsRemove (XenBusIo, XST_NIL, "event-channel");
  XenBusIo->XsRemove (XenBusIo, XST_NIL, "protocol");
  goto Error;
AbortTransaction:
  XenBusIo->XsTransactionEnd (XenBusIo, &Transaction, TRUE);
Error:
  XenPvBlockFree (Dev);
  return EFI_DEVICE_ERROR;
}

VOID
XenPvBlockFrontShutdown (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  )
{
  XENBUS_PROTOCOL  *XenBusIo = Dev->XenBusIo;
  XENSTORE_STATUS  Status;
  UINT64           Value;

  XenPvBlockSync (Dev);

  Status = XenBusIo->SetState (XenBusIo, XST_NIL, XenbusStateClosing);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: error while changing state to Closing: %d\n",
      Status
      ));
    goto Close;
  }

  Status = XenPvBlkWaitForBackendState (Dev, XenbusStateClosing, NULL);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: error while waiting for closing backend state: %d\n",
      Status
      ));
    goto Close;
  }

  Status = XenBusIo->SetState (XenBusIo, XST_NIL, XenbusStateClosed);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: error while changing state to Closed: %d\n",
      Status
      ));
    goto Close;
  }

  Status = XenPvBlkWaitForBackendState (Dev, XenbusStateClosed, NULL);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: error while waiting for closed backend state: %d\n",
      Status
      ));
    goto Close;
  }

  Status = XenBusIo->SetState (XenBusIo, XST_NIL, XenbusStateInitialising);
  if (Status != XENSTORE_STATUS_SUCCESS) {
    DEBUG ((
      DEBUG_ERROR,
      "XenPvBlk: error while changing state to initialising: %d\n",
      Status
      ));
    goto Close;
  }

  while (TRUE) {
    Status = XenBusReadUint64 (XenBusIo, "state", TRUE, &Value);
    if (Status != XENSTORE_STATUS_SUCCESS) {
      DEBUG ((
        DEBUG_ERROR,
        "XenPvBlk: error while waiting for new backend state: %d\n",
        Status
        ));
      goto Close;
    }

    if ((Value <= XenbusStateInitWait) || (Value >= XenbusStateClosed)) {
      break;
    }

    DEBUG ((
      DEBUG_INFO,
      "XenPvBlk: waiting backend state %d, current: %Lu\n",
      XenbusStateInitWait,
      Value
      ));
    XenBusIo->WaitForWatch (XenBusIo, Dev->StateWatchToken);
  }

Close:
  XenBusIo->UnregisterWatch (XenBusIo, Dev->StateWatchToken);
  XenBusIo->XsRemove (XenBusIo, XST_NIL, "ring-ref");
  XenBusIo->XsRemove (XenBusIo, XST_NIL, "event-channel");
  XenBusIo->XsRemove (XenBusIo, XST_NIL, "protocol");

  XenPvBlockFree (Dev);
}

STATIC
VOID
XenPvBlockWaitSlot (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  )
{
  /* Wait for a slot */
  if (RING_FULL (&Dev->Ring)) {
    while (TRUE) {
      XenPvBlockAsyncIoPoll (Dev);
      if (!RING_FULL (&Dev->Ring)) {
        break;
      }

      /* Really no slot, could wait for an event on Dev->EventChannel. */
    }
  }
}

VOID
XenPvBlockAsyncIo (
  IN OUT XEN_BLOCK_FRONT_IO  *IoData,
  IN     BOOLEAN             IsWrite
  )
{
  XEN_BLOCK_FRONT_DEVICE  *Dev      = IoData->Dev;
  XENBUS_PROTOCOL         *XenBusIo = Dev->XenBusIo;
  blkif_request_t         *Request;
  RING_IDX                RingIndex;
  BOOLEAN                 Notify;
  INT32                   NumSegments, Index;
  UINTN                   Start, End;

  // Can't io at non-sector-aligned location
  ASSERT (!(IoData->Sector & ((Dev->MediaInfo.SectorSize / 512) - 1)));
  // Can't io non-sector-sized amounts
  ASSERT (!(IoData->Size & (Dev->MediaInfo.SectorSize - 1)));
  // Can't io non-sector-aligned buffer
  ASSERT (!((UINTN)IoData->Buffer & (Dev->MediaInfo.SectorSize - 1)));

  Start          = (UINTN)IoData->Buffer & ~EFI_PAGE_MASK;
  End            = ((UINTN)IoData->Buffer + IoData->Size + EFI_PAGE_SIZE - 1) & ~EFI_PAGE_MASK;
  IoData->NumRef = NumSegments = (INT32)((End - Start) / EFI_PAGE_SIZE);

  ASSERT (NumSegments <= BLKIF_MAX_SEGMENTS_PER_REQUEST);

  XenPvBlockWaitSlot (Dev);
  RingIndex = Dev->Ring.req_prod_pvt;
  Request   = RING_GET_REQUEST (&Dev->Ring, RingIndex);

  Request->operation     = IsWrite ? BLKIF_OP_WRITE : BLKIF_OP_READ;
  Request->nr_segments   = (UINT8)NumSegments;
  Request->handle        = Dev->DeviceId;
  Request->id            = (UINTN)IoData;
  Request->sector_number = IoData->Sector;

  for (Index = 0; Index < NumSegments; Index++) {
    Request->seg[Index].first_sect = 0;
    Request->seg[Index].last_sect  = EFI_PAGE_SIZE / 512 - 1;
  }

  Request->seg[0].first_sect              = (UINT8)(((UINTN)IoData->Buffer & EFI_PAGE_MASK) / 512);
  Request->seg[NumSegments - 1].last_sect =
    (UINT8)((((UINTN)IoData->Buffer + IoData->Size - 1) & EFI_PAGE_MASK) / 512);
  for (Index = 0; Index < NumSegments; Index++) {
    UINTN  Data = Start + Index * EFI_PAGE_SIZE;
    XenBusIo->GrantAccess (
                XenBusIo,
                Dev->DomainId,
                Data >> EFI_PAGE_SHIFT,
                IsWrite,
                &Request->seg[Index].gref
                );
    IoData->GrantRef[Index] = Request->seg[Index].gref;
  }

  Dev->Ring.req_prod_pvt = RingIndex + 1;

  MemoryFence ();
  RING_PUSH_REQUESTS_AND_CHECK_NOTIFY (&Dev->Ring, Notify);

  if (Notify) {
    UINT32  ReturnCode;
    ReturnCode = XenBusIo->EventChannelNotify (XenBusIo, Dev->EventChannel);
    if (ReturnCode != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "XenPvBlk: Unexpected return value from EventChannelNotify: %d\n",
        ReturnCode
        ));
    }
  }
}

EFI_STATUS
XenPvBlockIo (
  IN OUT XEN_BLOCK_FRONT_IO  *IoData,
  IN     BOOLEAN             IsWrite
  )
{
  //
  // Status value that correspond to an IO in progress.
  //
  IoData->Status = EFI_ALREADY_STARTED;
  XenPvBlockAsyncIo (IoData, IsWrite);

  while (IoData->Status == EFI_ALREADY_STARTED) {
    XenPvBlockAsyncIoPoll (IoData->Dev);
  }

  return IoData->Status;
}

STATIC
VOID
XenPvBlockPushOperation (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev,
  IN UINT8                   Operation,
  IN UINT64                  Id
  )
{
  INT32            Index;
  blkif_request_t  *Request;
  BOOLEAN          Notify;

  XenPvBlockWaitSlot (Dev);
  Index                = Dev->Ring.req_prod_pvt;
  Request              = RING_GET_REQUEST (&Dev->Ring, Index);
  Request->operation   = Operation;
  Request->nr_segments = 0;
  Request->handle      = Dev->DeviceId;
  Request->id          = Id;
  /* Not needed anyway, but the backend will check it */
  Request->sector_number = 0;
  Dev->Ring.req_prod_pvt = Index + 1;
  MemoryFence ();
  RING_PUSH_REQUESTS_AND_CHECK_NOTIFY (&Dev->Ring, Notify);
  if (Notify) {
    XENBUS_PROTOCOL  *XenBusIo = Dev->XenBusIo;
    UINT32           ReturnCode;
    ReturnCode = XenBusIo->EventChannelNotify (XenBusIo, Dev->EventChannel);
    if (ReturnCode != 0) {
      DEBUG ((
        DEBUG_ERROR,
        "XenPvBlk: Unexpected return value from EventChannelNotify: %d\n",
        ReturnCode
        ));
    }
  }
}

VOID
XenPvBlockSync (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  )
{
  if (Dev->MediaInfo.ReadWrite) {
    if (Dev->MediaInfo.FeatureBarrier) {
      XenPvBlockPushOperation (Dev, BLKIF_OP_WRITE_BARRIER, 0);
    }

    if (Dev->MediaInfo.FeatureFlushCache) {
      XenPvBlockPushOperation (Dev, BLKIF_OP_FLUSH_DISKCACHE, 0);
    }
  }

  /* Note: This won't finish if another thread enqueues requests.  */
  while (TRUE) {
    XenPvBlockAsyncIoPoll (Dev);
    if (RING_FREE_REQUESTS (&Dev->Ring) == RING_SIZE (&Dev->Ring)) {
      break;
    }
  }
}

VOID
XenPvBlockAsyncIoPoll (
  IN XEN_BLOCK_FRONT_DEVICE  *Dev
  )
{
  RING_IDX          ProducerIndex, ConsumerIndex;
  blkif_response_t  *Response;
  INT32             More;

  do {
    ProducerIndex = Dev->Ring.sring->rsp_prod;
    /* Ensure we see queued responses up to 'ProducerIndex'. */
    MemoryFence ();
    ConsumerIndex = Dev->Ring.rsp_cons;

    while (ConsumerIndex != ProducerIndex) {
      XEN_BLOCK_FRONT_IO  *IoData = NULL;
      INT16               Status;

      Response = RING_GET_RESPONSE (&Dev->Ring, ConsumerIndex);

      IoData = (VOID *)(UINTN)Response->id;
      Status = Response->status;

      switch (Response->operation) {
        case BLKIF_OP_READ:
        case BLKIF_OP_WRITE:
        {
          INT32  Index;

          if (Status != BLKIF_RSP_OKAY) {
            DEBUG ((
              DEBUG_ERROR,
              "XenPvBlk: "
              "%a error %d on %a at sector %Lx, num bytes %Lx\n",
              Response->operation == BLKIF_OP_READ ? "read" : "write",
              Status,
              IoData->Dev->NodeName,
              (UINT64)IoData->Sector,
              (UINT64)IoData->Size
              ));
          }

          for (Index = 0; Index < IoData->NumRef; Index++) {
            Dev->XenBusIo->GrantEndAccess (Dev->XenBusIo, IoData->GrantRef[Index]);
          }

          break;
        }

        case BLKIF_OP_WRITE_BARRIER:
          if (Status != BLKIF_RSP_OKAY) {
            DEBUG ((DEBUG_ERROR, "XenPvBlk: write barrier error %d\n", Status));
          }

          break;
        case BLKIF_OP_FLUSH_DISKCACHE:
          if (Status != BLKIF_RSP_OKAY) {
            DEBUG ((DEBUG_ERROR, "XenPvBlk: flush error %d\n", Status));
          }

          break;

        default:
          DEBUG ((
            DEBUG_ERROR,
            "XenPvBlk: unrecognized block operation %d response (status %d)\n",
            Response->operation,
            Status
            ));
          break;
      }

      Dev->Ring.rsp_cons = ++ConsumerIndex;
      if (IoData != NULL) {
        IoData->Status = Status ? EFI_DEVICE_ERROR : EFI_SUCCESS;
      }

      if (Dev->Ring.rsp_cons != ConsumerIndex) {
        /* We reentered, we must not continue here */
        break;
      }
    }

    RING_FINAL_CHECK_FOR_RESPONSES (&Dev->Ring, More);
  } while (More != 0);
}
