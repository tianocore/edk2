/** @file
  Debug Port Library implementation based on usb3 debug port.

  Copyright (c) 2014 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/
#include "DebugCommunicationLibUsb3Internal.h"

/**
  Synchronize the specified transfer ring to update the enqueue and dequeue pointer.

  @param  Handle      Debug port handle.
  @param  TrsRing     The transfer ring to sync.

  @retval EFI_SUCCESS The transfer ring is synchronized successfully.

**/
EFI_STATUS
EFIAPI
XhcSyncTrsRing (
  IN USB3_DEBUG_PORT_HANDLE  *Handle,
  IN TRANSFER_RING           *TrsRing
  )
{
  UINTN         Index;
  TRB_TEMPLATE  *TrsTrb;
  UINT32        CycleBit;

  ASSERT (TrsRing != NULL);

  //
  // Calculate the latest RingEnqueue and RingPCS
  //
  TrsTrb = (TRB_TEMPLATE *)(UINTN)TrsRing->RingEnqueue;

  ASSERT (TrsTrb != NULL);

  for (Index = 0; Index < TrsRing->TrbNumber; Index++) {
    if (TrsTrb->CycleBit != (TrsRing->RingPCS & BIT0)) {
      break;
    }

    TrsTrb++;
    if ((UINT8)TrsTrb->Type == TRB_TYPE_LINK) {
      ASSERT (((LINK_TRB *)TrsTrb)->TC != 0);
      //
      // set cycle bit in Link TRB as normal
      //
      ((LINK_TRB *)TrsTrb)->CycleBit = TrsRing->RingPCS & BIT0;
      //
      // Toggle PCS maintained by software
      //
      TrsRing->RingPCS = (TrsRing->RingPCS & BIT0) ? 0 : 1;
      TrsTrb           = (TRB_TEMPLATE *)(UINTN)((TrsTrb->Parameter1 | LShiftU64 ((UINT64)TrsTrb->Parameter2, 32)) & ~0x0F);
    }
  }

  ASSERT (Index != TrsRing->TrbNumber);

  if ((EFI_PHYSICAL_ADDRESS)(UINTN)TrsTrb != TrsRing->RingEnqueue) {
    TrsRing->RingEnqueue = (EFI_PHYSICAL_ADDRESS)(UINTN)TrsTrb;
  }

  //
  // Clear the Trb context for enqueue, but reserve the PCS bit which indicates free Trb.
  //
  CycleBit = TrsTrb->CycleBit;
  ZeroMem (TrsTrb, sizeof (TRB_TEMPLATE));
  TrsTrb->CycleBit = CycleBit;

  return EFI_SUCCESS;
}

/**
  Synchronize the specified event ring to update the enqueue and dequeue pointer.

  @param  Handle      Debug port handle.
  @param  EvtRing     The event ring to sync.

  @retval EFI_SUCCESS The event ring is synchronized successfully.

**/
EFI_STATUS
EFIAPI
XhcSyncEventRing (
  IN USB3_DEBUG_PORT_HANDLE  *Handle,
  IN EVENT_RING              *EvtRing
  )
{
  UINTN         Index;
  TRB_TEMPLATE  *EvtTrb1;

  ASSERT (EvtRing != NULL);

  //
  // Calculate the EventRingEnqueue and EventRingCCS.
  // Note: only support single Segment
  //
  EvtTrb1 = (TRB_TEMPLATE *)(UINTN)EvtRing->EventRingDequeue;

  for (Index = 0; Index < EvtRing->TrbNumber; Index++) {
    if (EvtTrb1->CycleBit != EvtRing->EventRingCCS) {
      break;
    }

    EvtTrb1++;

    if ((UINTN)EvtTrb1 >= ((UINTN)EvtRing->EventRingSeg0 + sizeof (TRB_TEMPLATE) * EvtRing->TrbNumber)) {
      EvtTrb1               = (TRB_TEMPLATE *)(UINTN)EvtRing->EventRingSeg0;
      EvtRing->EventRingCCS = (EvtRing->EventRingCCS) ? 0 : 1;
    }
  }

  if (Index < EvtRing->TrbNumber) {
    EvtRing->EventRingEnqueue = (EFI_PHYSICAL_ADDRESS)(UINTN)EvtTrb1;
  } else {
    ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}

/**
  Check if there is a new generated event.

  @param  Handle        Debug port handle.
  @param  EvtRing       The event ring to check.
  @param  NewEvtTrb     The new event TRB found.

  @retval EFI_SUCCESS   Found a new event TRB at the event ring.
  @retval EFI_NOT_READY The event ring has no new event.

**/
EFI_STATUS
EFIAPI
XhcCheckNewEvent (
  IN  USB3_DEBUG_PORT_HANDLE  *Handle,
  IN  EVENT_RING              *EvtRing,
  OUT TRB_TEMPLATE            **NewEvtTrb
  )
{
  EFI_STATUS  Status;

  ASSERT (EvtRing != NULL);

  *NewEvtTrb = (TRB_TEMPLATE *)(UINTN)EvtRing->EventRingDequeue;

  if (EvtRing->EventRingDequeue == EvtRing->EventRingEnqueue) {
    return EFI_NOT_READY;
  }

  Status = EFI_SUCCESS;

  EvtRing->EventRingDequeue += sizeof (TRB_TEMPLATE);
  //
  // If the dequeue pointer is beyond the ring, then roll-back it to the beginning of the ring.
  //
  if ((UINTN)EvtRing->EventRingDequeue >= ((UINTN)EvtRing->EventRingSeg0 + sizeof (TRB_TEMPLATE) * EvtRing->TrbNumber)) {
    EvtRing->EventRingDequeue = EvtRing->EventRingSeg0;
  }

  return Status;
}

/**
  Check if the Trb is a transaction of the URB.

  @param Ring   The transfer ring to be checked.
  @param Trb    The TRB to be checked.

  @retval TRUE  It is a transaction of the URB.
  @retval FALSE It is not any transaction of the URB.

**/
BOOLEAN
IsTrbInTrsRing (
  IN  TRANSFER_RING  *Ring,
  IN  TRB_TEMPLATE   *Trb
  )
{
  TRB_TEMPLATE  *CheckedTrb;
  UINTN         Index;

  CheckedTrb = (TRB_TEMPLATE *)(UINTN)Ring->RingSeg0;

  ASSERT (Ring->TrbNumber == TR_RING_TRB_NUMBER);

  for (Index = 0; Index < Ring->TrbNumber; Index++) {
    if (Trb == CheckedTrb) {
      return TRUE;
    }

    CheckedTrb++;
  }

  return FALSE;
}

/**
  Check the URB's execution result and update the URB's
  result accordingly.

  @param  Handle          Debug port handle.
  @param  Urb             The URB to check result.

**/
VOID
XhcCheckUrbResult (
  IN  USB3_DEBUG_PORT_HANDLE  *Handle,
  IN  URB                     *Urb
  )
{
  EVT_TRB_TRANSFER  *EvtTrb;
  TRB_TEMPLATE      *TRBPtr;
  UINTN             Index;
  EFI_STATUS        Status;
  URB               *CheckedUrb;
  UINT64            XhcDequeue;
  UINT32            High;
  UINT32            Low;

  ASSERT ((Handle != NULL) && (Urb != NULL));

  if (Urb->Finished) {
    goto EXIT;
  }

  EvtTrb = NULL;

  //
  // Traverse the event ring to find out all new events from the previous check.
  //
  XhcSyncEventRing (Handle, &Handle->EventRing);

  for (Index = 0; Index < Handle->EventRing.TrbNumber; Index++) {
    Status = XhcCheckNewEvent (Handle, &Handle->EventRing, ((TRB_TEMPLATE **)&EvtTrb));
    if (Status == EFI_NOT_READY) {
      //
      // All new events are handled, return directly.
      //
      goto EXIT;
    }

    if ((EvtTrb->Type != TRB_TYPE_COMMAND_COMPLT_EVENT) && (EvtTrb->Type != TRB_TYPE_TRANS_EVENT)) {
      continue;
    }

    TRBPtr = (TRB_TEMPLATE *)(UINTN)(EvtTrb->TRBPtrLo | LShiftU64 ((UINT64)EvtTrb->TRBPtrHi, 32));

    if (IsTrbInTrsRing ((TRANSFER_RING *)(UINTN)(Urb->Ring), TRBPtr)) {
      CheckedUrb = Urb;
    } else if (IsTrbInTrsRing ((TRANSFER_RING *)(UINTN)(Handle->UrbIn.Ring), TRBPtr)) {
      //
      // If it is read event and it should be generated by poll, and current operation is write, we need save data into internal buffer.
      // Internal buffer is used by next read.
      //
      Handle->DataCount = (UINT8)(Handle->UrbIn.DataLen - EvtTrb->Length);
      CopyMem ((VOID *)(UINTN)Handle->Data, (VOID *)(UINTN)Handle->UrbIn.Data, Handle->DataCount);
      //
      // Fill this TRB complete with CycleBit, otherwise next read will fail with old TRB.
      //
      TRBPtr->CycleBit = (TRBPtr->CycleBit & BIT0) ? 0 : 1;
      continue;
    } else {
      continue;
    }

    if ((EvtTrb->Completecode == TRB_COMPLETION_SHORT_PACKET) ||
        (EvtTrb->Completecode == TRB_COMPLETION_SUCCESS))
    {
      //
      // The length of data which were transferred.
      //
      CheckedUrb->Completed += (((TRANSFER_TRB_NORMAL *)TRBPtr)->Length - EvtTrb->Length);
    } else {
      CheckedUrb->Result |= EFI_USB_ERR_TIMEOUT;
    }

    //
    // This Urb has been processed
    //
    CheckedUrb->Finished = TRUE;
  }

EXIT:
  //
  // Advance event ring to last available entry
  //
  // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
  // So divide it to two 32-bytes width register access.
  //
  Low        = XhcReadDebugReg (Handle, XHC_DC_DCERDP);
  High       = XhcReadDebugReg (Handle, XHC_DC_DCERDP + 4);
  XhcDequeue = (UINT64)(LShiftU64 ((UINT64)High, 32) | Low);

  if ((XhcDequeue & (~0x0F)) != ((UINT64)(UINTN)Handle->EventRing.EventRingDequeue & (~0x0F))) {
    //
    // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
    // So divide it to two 32-bytes width register access.
    //
    XhcWriteDebugReg (Handle, XHC_DC_DCERDP, XHC_LOW_32BIT (Handle->EventRing.EventRingDequeue));
    XhcWriteDebugReg (Handle, XHC_DC_DCERDP + 4, XHC_HIGH_32BIT (Handle->EventRing.EventRingDequeue));
  }
}

/**
  Ring the door bell to notify XHCI there is a transaction to be executed.

  @param  Handle        Debug port handle.
  @param  Urb           The pointer to URB.

  @retval EFI_SUCCESS   Successfully ring the door bell.

**/
EFI_STATUS
EFIAPI
XhcRingDoorBell (
  IN USB3_DEBUG_PORT_HANDLE  *Handle,
  IN URB                     *Urb
  )
{
  UINT32  Dcdb;

  //
  // 7.6.8.2 DCDB Register
  //
  Dcdb = (Urb->Direction == EfiUsbDataIn) ? 0x100 : 0x0;

  XhcWriteDebugReg (
    Handle,
    XHC_DC_DCDB,
    Dcdb
    );

  return EFI_SUCCESS;
}

/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Handle            Debug port handle.
  @param  Urb               The URB to execute.
  @param  Timeout           The time to wait before abort, in microsecond.

**/
VOID
XhcExecTransfer (
  IN  USB3_DEBUG_PORT_HANDLE  *Handle,
  IN  URB                     *Urb,
  IN  UINTN                   Timeout
  )
{
  TRANSFER_RING  *Ring;
  TRB_TEMPLATE   *Trb;
  UINTN          Loop;
  UINTN          Index;

  Loop = Timeout / XHC_DEBUG_PORT_1_MILLISECOND;
  if (Timeout == 0) {
    Loop = 0xFFFFFFFF;
  }

  XhcRingDoorBell (Handle, Urb);
  //
  // Event Ring Not Empty bit can only be set to 1 by XHC after ringing door bell with some delay.
  //
  for (Index = 0; Index < Loop; Index++) {
    XhcCheckUrbResult (Handle, Urb);
    if (Urb->Finished) {
      break;
    }

    MicroSecondDelay (XHC_DEBUG_PORT_1_MILLISECOND);
  }

  if (Index == Loop) {
    //
    // If time out occurs.
    //
    Urb->Result |= EFI_USB_ERR_TIMEOUT;
  }

  //
  // If URB transfer is error, restore transfer ring to original value before URB transfer
  // This will make the current transfer TRB is always at the latest unused one in transfer ring.
  //
  Ring = (TRANSFER_RING *)(UINTN)Urb->Ring;
  if ((Urb->Result != EFI_USB_NOERROR) && (Urb->Direction == EfiUsbDataIn)) {
    //
    // Adjust Enqueue pointer
    //
    Ring->RingEnqueue = Urb->Trb;
    //
    // Clear CCS flag for next use
    //
    Trb           = (TRB_TEMPLATE *)(UINTN)Urb->Trb;
    Trb->CycleBit = ((~Ring->RingPCS) & BIT0);
  } else {
    //
    // Update transfer ring for next transfer.
    //
    XhcSyncTrsRing (Handle, Ring);
  }
}

/**
  Create a transfer TRB.

  @param  Handle  Debug port handle.
  @param  Urb     The urb used to construct the transfer TRB.

  @return Created TRB or NULL

**/
EFI_STATUS
XhcCreateTransferTrb (
  IN USB3_DEBUG_PORT_HANDLE  *Handle,
  IN URB                     *Urb
  )
{
  TRANSFER_RING  *EPRing;
  TRB            *Trb;

  if (Urb->Direction == EfiUsbDataIn) {
    EPRing = &Handle->TransferRingIn;
  } else {
    EPRing = &Handle->TransferRingOut;
  }

  Urb->Ring = (EFI_PHYSICAL_ADDRESS)(UINTN)EPRing;
  XhcSyncTrsRing (Handle, EPRing);

  Urb->Trb                 = EPRing->RingEnqueue;
  Trb                      = (TRB *)(UINTN)EPRing->RingEnqueue;
  Trb->TrbNormal.TRBPtrLo  = XHC_LOW_32BIT (Urb->Data);
  Trb->TrbNormal.TRBPtrHi  = XHC_HIGH_32BIT (Urb->Data);
  Trb->TrbNormal.Length    = Urb->DataLen;
  Trb->TrbNormal.TDSize    = 0;
  Trb->TrbNormal.IntTarget = 0;
  Trb->TrbNormal.ISP       = 1;
  Trb->TrbNormal.IOC       = 1;
  Trb->TrbNormal.Type      = TRB_TYPE_NORMAL;

  //
  // Update the cycle bit to indicate this TRB has been consumed.
  //
  Trb->TrbNormal.CycleBit = EPRing->RingPCS & BIT0;

  return EFI_SUCCESS;
}

/**
  Create a new URB for a new transaction.

  @param  Handle     Debug port handle.
  @param  Direction  The direction of data flow.
  @param  Data       The user data to transfer
  @param  DataLen    The length of data buffer

  @return Created URB or NULL

**/
URB *
XhcCreateUrb (
  IN USB3_DEBUG_PORT_HANDLE  *Handle,
  IN EFI_USB_DATA_DIRECTION  Direction,
  IN VOID                    *Data,
  IN UINTN                   DataLen
  )
{
  EFI_STATUS            Status;
  URB                   *Urb;
  EFI_PHYSICAL_ADDRESS  UrbData;

  if (Direction == EfiUsbDataIn) {
    Urb = &Handle->UrbIn;
  } else {
    Urb = &Handle->UrbOut;
  }

  UrbData = Urb->Data;

  ZeroMem (Urb, sizeof (URB));
  Urb->Direction = Direction;

  //
  // Allocate memory to move data from CAR or SMRAM to normal memory
  // to make XHCI DMA successfully
  // re-use the pre-allocate buffer in PEI to avoid DXE memory service or gBS are not ready
  //
  Urb->Data = UrbData;

  if (Direction == EfiUsbDataIn) {
    //
    // Do not break URB data in buffer as it may contain the data which were just put in via DMA by XHC
    //
    Urb->DataLen = (UINT32)DataLen;
  } else {
    //
    // Put data into URB data out buffer which will create TRBs
    //
    ZeroMem ((VOID *)(UINTN)Urb->Data, DataLen);
    CopyMem ((VOID *)(UINTN)Urb->Data, Data, DataLen);
    Urb->DataLen = (UINT32)DataLen;
  }

  Status = XhcCreateTransferTrb (Handle, Urb);
  ASSERT_EFI_ERROR (Status);

  return Urb;
}

/**
  Submits bulk transfer to a bulk endpoint of a USB device.

  @param  Handle                Debug port handle.
  @param  Direction             The direction of data transfer.
  @param  Data                  Array of pointers to the buffers of data to transmit
                                from or receive into.
  @param  DataLength            The length of the data buffer.
  @param  Timeout               Indicates the maximum time, in microsecond, which
                                the transfer is allowed to complete.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_OUT_OF_RESOURCES  The transfer failed due to lack of resource.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
XhcDataTransfer (
  IN     USB3_DEBUG_PORT_HANDLE  *Handle,
  IN     EFI_USB_DATA_DIRECTION  Direction,
  IN OUT VOID                    *Data,
  IN OUT UINTN                   *DataLength,
  IN     UINTN                   Timeout
  )
{
  URB         *Urb;
  EFI_STATUS  Status;

  //
  // Validate the parameters
  //
  if ((DataLength == NULL) || (*DataLength == 0) || (Data == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  //
  // Create a new URB, insert it into the asynchronous
  // schedule list, then poll the execution status.
  //
  Urb = XhcCreateUrb (Handle, Direction, Data, *DataLength);
  ASSERT (Urb != NULL);

  XhcExecTransfer (Handle, Urb, Timeout);

  //
  // Make sure the data received from HW can fit in the received buffer.
  //
  if (Urb->Completed > *DataLength) {
    return EFI_DEVICE_ERROR;
  }

  *DataLength = Urb->Completed;

  Status = EFI_TIMEOUT;
  if (Urb->Result == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

  if (Direction == EfiUsbDataIn) {
    //
    // Move data from internal buffer to outside buffer (outside buffer may be in SMRAM...)
    // SMRAM does not allow to do DMA, so we create an internal buffer.
    //
    CopyMem (Data, (VOID *)(UINTN)Urb->Data, *DataLength);
  }

  return Status;
}
