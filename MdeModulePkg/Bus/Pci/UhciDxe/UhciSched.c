/** @file

  The EHCI register operation routines.

Copyright (c) 2007 - 2018, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Uhci.h"

/**
  Create Frame List Structure.

  @param  Uhc                    UHCI device.

  @retval EFI_OUT_OF_RESOURCES   Can't allocate memory resources.
  @retval EFI_UNSUPPORTED        Map memory fail.
  @retval EFI_SUCCESS            Success.

**/
EFI_STATUS
UhciInitFrameList (
  IN USB_HC_DEV  *Uhc
  )
{
  EFI_PHYSICAL_ADDRESS  MappedAddr;
  EFI_STATUS            Status;
  VOID                  *Buffer;
  VOID                  *Mapping;
  UINTN                 Pages;
  UINTN                 Bytes;
  UINTN                 Index;
  EFI_PHYSICAL_ADDRESS  PhyAddr;

  //
  // The Frame List is a common buffer that will be
  // accessed by both the cpu and the usb bus master
  // at the same time. The Frame List ocupies 4K bytes,
  // and must be aligned on 4-Kbyte boundaries.
  //
  Bytes = 4096;
  Pages = EFI_SIZE_TO_PAGES (Bytes);

  Status = Uhc->PciIo->AllocateBuffer (
                         Uhc->PciIo,
                         AllocateAnyPages,
                         EfiBootServicesData,
                         Pages,
                         &Buffer,
                         0
                         );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = Uhc->PciIo->Map (
                         Uhc->PciIo,
                         EfiPciIoOperationBusMasterCommonBuffer,
                         Buffer,
                         &Bytes,
                         &MappedAddr,
                         &Mapping
                         );

  if (EFI_ERROR (Status) || (Bytes != 4096)) {
    Status = EFI_UNSUPPORTED;
    goto ON_ERROR;
  }

  Uhc->FrameBase    = (UINT32 *)(UINTN)Buffer;
  Uhc->FrameMapping = Mapping;

  //
  // Tell the Host Controller where the Frame List lies,
  // by set the Frame List Base Address Register.
  //
  UhciSetFrameListBaseAddr (Uhc->PciIo, (VOID *)(UINTN)MappedAddr);

  //
  // Allocate the QH used by sync interrupt/control/bulk transfer.
  // FS ctrl/bulk queue head is set to loopback so additional BW
  // can be reclaimed. Notice, LS don't support bulk transfer and
  // also doesn't support BW reclamation.
  //
  Uhc->SyncIntQh = UhciCreateQh (Uhc, 1);
  Uhc->CtrlQh    = UhciCreateQh (Uhc, 1);
  Uhc->BulkQh    = UhciCreateQh (Uhc, 1);

  if ((Uhc->SyncIntQh == NULL) || (Uhc->CtrlQh == NULL) || (Uhc->BulkQh == NULL)) {
    Uhc->PciIo->Unmap (Uhc->PciIo, Mapping);
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  //
  //                                                +-------------+
  //                                                |             |
  // Link the three together: SyncIntQh->CtrlQh->BulkQh <---------+
  // Each frame entry is linked to this sequence of QH. These QH
  // will remain on the schedul, never got removed
  //
  PhyAddr                          = UsbHcGetPciAddressForHostMem (Uhc->MemPool, Uhc->CtrlQh, sizeof (UHCI_QH_HW));
  Uhc->SyncIntQh->QhHw.HorizonLink = QH_HLINK (PhyAddr, FALSE);
  Uhc->SyncIntQh->NextQh           = Uhc->CtrlQh;

  PhyAddr                       = UsbHcGetPciAddressForHostMem (Uhc->MemPool, Uhc->BulkQh, sizeof (UHCI_QH_HW));
  Uhc->CtrlQh->QhHw.HorizonLink = QH_HLINK (PhyAddr, FALSE);
  Uhc->CtrlQh->NextQh           = Uhc->BulkQh;

  //
  // Some old platform such as Intel's Tiger 4 has a difficult time
  // in supporting the full speed bandwidth reclamation in the previous
  // mentioned form. Most new platforms don't suffer it.
  //
  Uhc->BulkQh->QhHw.HorizonLink = QH_HLINK (PhyAddr, FALSE);

  Uhc->BulkQh->NextQh = NULL;

  Uhc->FrameBaseHostAddr = AllocateZeroPool (4096);
  if (Uhc->FrameBaseHostAddr == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_ERROR;
  }

  PhyAddr = UsbHcGetPciAddressForHostMem (Uhc->MemPool, Uhc->SyncIntQh, sizeof (UHCI_QH_HW));
  for (Index = 0; Index < UHCI_FRAME_NUM; Index++) {
    Uhc->FrameBase[Index]         = QH_HLINK (PhyAddr, FALSE);
    Uhc->FrameBaseHostAddr[Index] = (UINT32)(UINTN)Uhc->SyncIntQh;
  }

  return EFI_SUCCESS;

ON_ERROR:
  if (Uhc->SyncIntQh != NULL) {
    UsbHcFreeMem (Uhc->MemPool, Uhc->SyncIntQh, sizeof (UHCI_QH_SW));
  }

  if (Uhc->CtrlQh != NULL) {
    UsbHcFreeMem (Uhc->MemPool, Uhc->CtrlQh, sizeof (UHCI_QH_SW));
  }

  if (Uhc->BulkQh != NULL) {
    UsbHcFreeMem (Uhc->MemPool, Uhc->BulkQh, sizeof (UHCI_QH_SW));
  }

  Uhc->PciIo->FreeBuffer (Uhc->PciIo, Pages, Buffer);
  return Status;
}

/**
  Destory FrameList buffer.

  @param  Uhc                    The UHCI device.

**/
VOID
UhciDestoryFrameList (
  IN USB_HC_DEV  *Uhc
  )
{
  //
  // Unmap the common buffer for framelist entry,
  // and free the common buffer.
  // Uhci's frame list occupy 4k memory.
  //
  Uhc->PciIo->Unmap (Uhc->PciIo, Uhc->FrameMapping);

  Uhc->PciIo->FreeBuffer (
                Uhc->PciIo,
                EFI_SIZE_TO_PAGES (4096),
                (VOID *)Uhc->FrameBase
                );

  if (Uhc->FrameBaseHostAddr != NULL) {
    FreePool (Uhc->FrameBaseHostAddr);
  }

  if (Uhc->SyncIntQh != NULL) {
    UsbHcFreeMem (Uhc->MemPool, Uhc->SyncIntQh, sizeof (UHCI_QH_SW));
  }

  if (Uhc->CtrlQh != NULL) {
    UsbHcFreeMem (Uhc->MemPool, Uhc->CtrlQh, sizeof (UHCI_QH_SW));
  }

  if (Uhc->BulkQh != NULL) {
    UsbHcFreeMem (Uhc->MemPool, Uhc->BulkQh, sizeof (UHCI_QH_SW));
  }

  Uhc->FrameBase         = NULL;
  Uhc->FrameBaseHostAddr = NULL;
  Uhc->SyncIntQh         = NULL;
  Uhc->CtrlQh            = NULL;
  Uhc->BulkQh            = NULL;
}

/**
  Convert the poll rate to the maximum 2^n that is smaller
  than Interval.

  @param  Interval               The poll rate to convert.

  @return The converted poll rate.

**/
UINTN
UhciConvertPollRate (
  IN  UINTN  Interval
  )
{
  UINTN  BitCount;

  ASSERT (Interval != 0);

  //
  // To safeguard RELEASE mode wherein ASSERT is effectively not there,
  // if inadvertently Interval is still 0 here, treat it the same as 1.
  //
  if (Interval == 0) {
    Interval = 1;
  }

  //
  // Find the index (1 based) of the highest non-zero bit
  //
  BitCount = 0;

  while (Interval != 0) {
    Interval >>= 1;
    BitCount++;
  }

  return (UINTN)1 << (BitCount - 1);
}

/**
  Link a queue head (for asynchronous interrupt transfer) to
  the frame list.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head to link into.

**/
VOID
UhciLinkQhToFrameList (
  USB_HC_DEV  *Uhc,
  UHCI_QH_SW  *Qh
  )
{
  UINTN                 Index;
  UHCI_QH_SW            *Prev;
  UHCI_QH_SW            *Next;
  EFI_PHYSICAL_ADDRESS  PhyAddr;
  EFI_PHYSICAL_ADDRESS  QhPciAddr;

  ASSERT ((Uhc->FrameBase != NULL) && (Qh != NULL));

  QhPciAddr = UsbHcGetPciAddressForHostMem (Uhc->MemPool, Qh, sizeof (UHCI_QH_HW));

  for (Index = 0; Index < UHCI_FRAME_NUM; Index += Qh->Interval) {
    //
    // First QH can't be NULL because we always keep static queue
    // heads on the frame list
    //
    ASSERT (!LINK_TERMINATED (Uhc->FrameBase[Index]));
    Next = (UHCI_QH_SW *)(UINTN)Uhc->FrameBaseHostAddr[Index];
    Prev = NULL;

    //
    // Now, insert the queue head (Qh) into this frame:
    // 1. Find a queue head with the same poll interval, just insert
    //    Qh after this queue head, then we are done.
    //
    // 2. Find the position to insert the queue head into:
    //      Previous head's interval is bigger than Qh's
    //      Next head's interval is less than Qh's
    //    Then, insert the Qh between then
    //
    // This method is very much the same as that used by EHCI.
    // Because each QH's interval is round down to 2^n, poll
    // rate is correct.
    //
    while (Next->Interval > Qh->Interval) {
      Prev = Next;
      Next = Next->NextQh;
      ASSERT (Next != NULL);
    }

    //
    // The entry may have been linked into the frame by early insertation.
    // For example: if insert a Qh with Qh.Interval == 4, and there is a Qh
    // with Qh.Interval == 8 on the frame. If so, we are done with this frame.
    // It isn't necessary to compare all the QH with the same interval to
    // Qh. This is because if there is other QH with the same interval, Qh
    // should has been inserted after that at FrameBase[0] and at FrameBase[0] it is
    // impossible (Next == Qh)
    //
    if (Next == Qh) {
      continue;
    }

    if (Next->Interval == Qh->Interval) {
      //
      // If there is a QH with the same interval, it locates at
      // FrameBase[0], and we can simply insert it after this QH. We
      // are all done.
      //
      ASSERT ((Index == 0) && (Qh->NextQh == NULL));

      Prev = Next;
      Next = Next->NextQh;

      Qh->NextQh   = Next;
      Prev->NextQh = Qh;

      Qh->QhHw.HorizonLink = Prev->QhHw.HorizonLink;

      Prev->QhHw.HorizonLink = QH_HLINK (QhPciAddr, FALSE);
      break;
    }

    //
    // OK, find the right position, insert it in. If Qh's next
    // link has already been set, it is in position. This is
    // guarranted by 2^n polling interval.
    //
    if (Qh->NextQh == NULL) {
      Qh->NextQh           = Next;
      PhyAddr              = UsbHcGetPciAddressForHostMem (Uhc->MemPool, Next, sizeof (UHCI_QH_HW));
      Qh->QhHw.HorizonLink = QH_HLINK (PhyAddr, FALSE);
    }

    if (Prev == NULL) {
      Uhc->FrameBase[Index]         = QH_HLINK (QhPciAddr, FALSE);
      Uhc->FrameBaseHostAddr[Index] = (UINT32)(UINTN)Qh;
    } else {
      Prev->NextQh           = Qh;
      Prev->QhHw.HorizonLink = QH_HLINK (QhPciAddr, FALSE);
    }
  }
}

/**
  Unlink QH from the frame list is easier: find all
  the precedence node, and pointer there next to QhSw's
  next.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head to unlink.

**/
VOID
UhciUnlinkQhFromFrameList (
  USB_HC_DEV  *Uhc,
  UHCI_QH_SW  *Qh
  )
{
  UINTN       Index;
  UHCI_QH_SW  *Prev;
  UHCI_QH_SW  *This;

  ASSERT ((Uhc->FrameBase != NULL) && (Qh != NULL));

  for (Index = 0; Index < UHCI_FRAME_NUM; Index += Qh->Interval) {
    //
    // Frame link can't be NULL because we always keep static
    // queue heads on the frame list
    //
    ASSERT (!LINK_TERMINATED (Uhc->FrameBase[Index]));
    This = (UHCI_QH_SW *)(UINTN)Uhc->FrameBaseHostAddr[Index];
    Prev = NULL;

    //
    // Walk through the frame's QH list to find the
    // queue head to remove
    //
    while ((This != NULL) && (This != Qh)) {
      Prev = This;
      This = This->NextQh;
    }

    //
    // Qh may have already been unlinked from this frame
    // by early action.
    //
    if (This == NULL) {
      continue;
    }

    if (Prev == NULL) {
      //
      // Qh is the first entry in the frame
      //
      Uhc->FrameBase[Index]         = Qh->QhHw.HorizonLink;
      Uhc->FrameBaseHostAddr[Index] = (UINT32)(UINTN)Qh->NextQh;
    } else {
      Prev->NextQh           = Qh->NextQh;
      Prev->QhHw.HorizonLink = Qh->QhHw.HorizonLink;
    }
  }
}

/**
  Check TDs Results.

  @param  Uhc                    This UHCI device.
  @param  Td                     UHCI_TD_SW to check.
  @param  IsLow                  Is Low Speed Device.
  @param  QhResult               Return the result of this TD list.

  @return Whether the TD's result is finialized.

**/
BOOLEAN
UhciCheckTdStatus (
  IN  USB_HC_DEV      *Uhc,
  IN  UHCI_TD_SW      *Td,
  IN  BOOLEAN         IsLow,
  OUT UHCI_QH_RESULT  *QhResult
  )
{
  UINTN       Len;
  UINT8       State;
  UHCI_TD_HW  *TdHw;
  BOOLEAN     Finished;

  Finished = TRUE;

  //
  // Initialize the data toggle to that of the first
  // TD. The next toggle to use is either:
  // 1. first TD's toggle if no TD is executed OK
  // 2. the next toggle of last executed-OK TD
  //
  QhResult->Result     = EFI_USB_NOERROR;
  QhResult->NextToggle = (UINT8)Td->TdHw.DataToggle;
  QhResult->Complete   = 0;

  while (Td != NULL) {
    TdHw  = &Td->TdHw;
    State = (UINT8)TdHw->Status;

    //
    // UHCI will set STALLED bit when it abort the execution
    // of TD list. There are several reasons:
    //   1. BABBLE error happened
    //   2. Received a STALL response
    //   3. Error count decreased to zero.
    //
    // It also set CRC/Timeout/NAK/Buffer Error/BitStuff Error
    // bits when corresponding conditions happen. But these
    // conditions are not deadly, that is a TD can successfully
    // completes even these bits are set. But it is likely that
    // upper layer won't distinguish these condtions. So, only
    // set these bits when TD is actually halted.
    //
    if ((State & USBTD_STALLED) != 0) {
      if ((State & USBTD_BABBLE) != 0) {
        QhResult->Result |= EFI_USB_ERR_BABBLE;
      } else if (TdHw->ErrorCount != 0) {
        QhResult->Result |= EFI_USB_ERR_STALL;
      }

      if ((State & USBTD_CRC) != 0) {
        QhResult->Result |= EFI_USB_ERR_CRC;
      }

      if ((State & USBTD_BUFFERR) != 0) {
        QhResult->Result |= EFI_USB_ERR_BUFFER;
      }

      if ((Td->TdHw.Status & USBTD_BITSTUFF) != 0) {
        QhResult->Result |= EFI_USB_ERR_BITSTUFF;
      }

      if (TdHw->ErrorCount == 0) {
        QhResult->Result |= EFI_USB_ERR_TIMEOUT;
      }

      Finished = TRUE;
      goto ON_EXIT;
    } else if ((State & USBTD_ACTIVE) != 0) {
      //
      // The TD is still active, no need to check further.
      //
      QhResult->Result |= EFI_USB_ERR_NOTEXECUTE;

      Finished = FALSE;
      goto ON_EXIT;
    } else {
      //
      // Update the next data toggle, it is always the
      // next to the last known-good TD's data toggle if
      // any TD is executed OK
      //
      QhResult->NextToggle = (UINT8)(1 - (UINT8)TdHw->DataToggle);

      //
      // This TD is finished OK or met short packet read. Update the
      // transfer length if it isn't a SETUP.
      //
      Len = (TdHw->ActualLen + 1) & 0x7FF;

      if (TdHw->PidCode != SETUP_PACKET_ID) {
        QhResult->Complete += Len;
      }

      //
      // Short packet condition for full speed input TD, also
      // terminate the transfer
      //
      if (!IsLow && (TdHw->ShortPacket == 1) && (Len < Td->DataLen)) {
        DEBUG ((DEBUG_VERBOSE, "UhciCheckTdStatus: short packet read occurred\n"));

        Finished = TRUE;
        goto ON_EXIT;
      }
    }

    Td = Td->NextTd;
  }

ON_EXIT:
  //
  // Check whether HC is halted. Don't move this up. It must be
  // called after data toggle is successfully updated.
  //
  if (!UhciIsHcWorking (Uhc->PciIo)) {
    QhResult->Result |= EFI_USB_ERR_SYSTEM;
    Finished          = TRUE;
  }

  if (Finished) {
    Uhc->PciIo->Flush (Uhc->PciIo);
  }

  UhciAckAllInterrupt (Uhc);
  return Finished;
}

/**
  Check the result of the transfer.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head of the transfer.
  @param  Td                     The first TDs of the transfer.
  @param  TimeOut                TimeOut value in milliseconds.
  @param  IsLow                  Is Low Speed Device.
  @param  QhResult               The variable to return result.

  @retval EFI_SUCCESS            The transfer finished with success.
  @retval EFI_DEVICE_ERROR       Transfer failed.

**/
EFI_STATUS
UhciExecuteTransfer (
  IN  USB_HC_DEV      *Uhc,
  IN  UHCI_QH_SW      *Qh,
  IN  UHCI_TD_SW      *Td,
  IN  UINTN           TimeOut,
  IN  BOOLEAN         IsLow,
  OUT UHCI_QH_RESULT  *QhResult
  )
{
  UINTN       Index;
  UINTN       Delay;
  BOOLEAN     Finished;
  EFI_STATUS  Status;
  BOOLEAN     InfiniteLoop;

  Finished     = FALSE;
  Status       = EFI_SUCCESS;
  Delay        = TimeOut * UHC_1_MILLISECOND;
  InfiniteLoop = FALSE;

  //
  // According to UEFI spec section 16.2.4, If Timeout is 0, then the caller
  // must wait for the function to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR
  // is returned.
  //
  if (TimeOut == 0) {
    InfiniteLoop = TRUE;
  }

  for (Index = 0; InfiniteLoop || (Index < Delay); Index++) {
    Finished = UhciCheckTdStatus (Uhc, Td, IsLow, QhResult);

    //
    // Transfer is OK or some error occurred (TD inactive)
    //
    if (Finished) {
      break;
    }

    gBS->Stall (UHC_1_MICROSECOND);
  }

  if (!Finished) {
    DEBUG ((DEBUG_ERROR, "UhciExecuteTransfer: execution not finished for %dms\n", (UINT32)TimeOut));
    UhciDumpQh (Qh);
    UhciDumpTds (Td);

    Status = EFI_TIMEOUT;
  } else if (QhResult->Result != EFI_USB_NOERROR) {
    DEBUG ((DEBUG_ERROR, "UhciExecuteTransfer: execution failed with result %x\n", QhResult->Result));
    UhciDumpQh (Qh);
    UhciDumpTds (Td);

    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

/**
  Update Async Request, QH and TDs.

  @param  Uhc                    The UHCI device.
  @param  AsyncReq               The UHCI asynchronous transfer to update.
  @param  Result                 Transfer reslut.
  @param  NextToggle             The toggle of next data.

**/
VOID
UhciUpdateAsyncReq (
  IN USB_HC_DEV          *Uhc,
  IN UHCI_ASYNC_REQUEST  *AsyncReq,
  IN UINT32              Result,
  IN UINT32              NextToggle
  )
{
  UHCI_QH_SW  *Qh;
  UHCI_TD_SW  *FirstTd;
  UHCI_TD_SW  *Td;

  Qh      = AsyncReq->QhSw;
  FirstTd = AsyncReq->FirstTd;

  if (Result == EFI_USB_NOERROR) {
    //
    // The last transfer succeeds. Then we need to update
    // the Qh and Td for next round of transfer.
    // 1. Update the TD's data toggle
    // 2. Activate all the TDs
    // 3. Link the TD to the queue head again since during
    //    execution, queue head's TD pointer is changed by
    //    hardware.
    //
    for (Td = FirstTd; Td != NULL; Td = Td->NextTd) {
      Td->TdHw.DataToggle = NextToggle;
      NextToggle         ^= 1;
      Td->TdHw.Status    |= USBTD_ACTIVE;
    }

    UhciLinkTdToQh (Uhc, Qh, FirstTd);
    return;
  }
}

/**
  Create Async Request node, and Link to List.

  @param  Uhc                    The UHCI device.
  @param  Qh                     The queue head of the transfer.
  @param  FirstTd                First TD of the transfer.
  @param  DevAddr                Device Address.
  @param  EndPoint               EndPoint Address.
  @param  DataLen                Data length.
  @param  Interval               Polling Interval when inserted to frame list.
  @param  Data                   Data buffer, unmapped.
  @param  Callback               Callback after interrupt transfeer.
  @param  Context                Callback Context passed as function parameter.
  @param  IsLow                  Is Low Speed.

  @retval EFI_SUCCESS            An asynchronous transfer is created.
  @retval EFI_INVALID_PARAMETER  Paremeter is error.
  @retval EFI_OUT_OF_RESOURCES   Failed because of resource shortage.

**/
EFI_STATUS
UhciCreateAsyncReq (
  IN USB_HC_DEV                       *Uhc,
  IN UHCI_QH_SW                       *Qh,
  IN UHCI_TD_SW                       *FirstTd,
  IN UINT8                            DevAddr,
  IN UINT8                            EndPoint,
  IN UINTN                            DataLen,
  IN UINTN                            Interval,
  IN UINT8                            *Data,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK  Callback,
  IN VOID                             *Context,
  IN BOOLEAN                          IsLow
  )
{
  UHCI_ASYNC_REQUEST  *AsyncReq;

  AsyncReq = AllocatePool (sizeof (UHCI_ASYNC_REQUEST));

  if (AsyncReq == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  //
  // Fill Request field. Data is allocated host memory, not mapped
  //
  AsyncReq->Signature = UHCI_ASYNC_INT_SIGNATURE;
  AsyncReq->DevAddr   = DevAddr;
  AsyncReq->EndPoint  = EndPoint;
  AsyncReq->DataLen   = DataLen;
  AsyncReq->Interval  = UhciConvertPollRate (Interval);
  AsyncReq->Data      = Data;
  AsyncReq->Callback  = Callback;
  AsyncReq->Context   = Context;
  AsyncReq->QhSw      = Qh;
  AsyncReq->FirstTd   = FirstTd;
  AsyncReq->IsLow     = IsLow;

  //
  // Insert the new interrupt transfer to the head of the list.
  // The interrupt transfer's monitor function scans the whole
  // list from head to tail. The new interrupt transfer MUST be
  // added to the head of the list.
  //
  InsertHeadList (&(Uhc->AsyncIntList), &(AsyncReq->Link));

  return EFI_SUCCESS;
}

/**
  Free an asynchronous request's resource such as memory.

  @param  Uhc                    The UHCI device.
  @param  AsyncReq               The asynchronous request to free.

**/
VOID
UhciFreeAsyncReq (
  IN USB_HC_DEV          *Uhc,
  IN UHCI_ASYNC_REQUEST  *AsyncReq
  )
{
  ASSERT ((Uhc != NULL) && (AsyncReq != NULL));

  UhciDestoryTds (Uhc, AsyncReq->FirstTd);
  UsbHcFreeMem (Uhc->MemPool, AsyncReq->QhSw, sizeof (UHCI_QH_SW));

  if (AsyncReq->Data != NULL) {
    UsbHcFreeMem (Uhc->MemPool, AsyncReq->Data, AsyncReq->DataLen);
  }

  gBS->FreePool (AsyncReq);
}

/**
  Unlink an asynchronous request's from UHC's asynchronus list.
  also remove the queue head from the frame list. If FreeNow,
  release its resource also. Otherwise, add the request to the
  UHC's recycle list to wait for a while before release the memory.
  Until then, hardware won't hold point to the request.

  @param  Uhc                    The UHCI device.
  @param  AsyncReq               The asynchronous request to free.
  @param  FreeNow                If TRUE, free the resource immediately, otherwise
                                 add the request to recycle wait list.

**/
VOID
UhciUnlinkAsyncReq (
  IN USB_HC_DEV          *Uhc,
  IN UHCI_ASYNC_REQUEST  *AsyncReq,
  IN BOOLEAN             FreeNow
  )
{
  ASSERT ((Uhc != NULL) && (AsyncReq != NULL));

  RemoveEntryList (&(AsyncReq->Link));
  UhciUnlinkQhFromFrameList (Uhc, AsyncReq->QhSw);

  if (FreeNow) {
    UhciFreeAsyncReq (Uhc, AsyncReq);
  } else {
    //
    // To sychronize with hardware, mark the queue head as inactive
    // then add AsyncReq to UHC's recycle list
    //
    AsyncReq->QhSw->QhHw.VerticalLink = QH_VLINK (NULL, TRUE);
    AsyncReq->Recycle                 = Uhc->RecycleWait;
    Uhc->RecycleWait                  = AsyncReq;
  }
}

/**
  Delete Async Interrupt QH and TDs.

  @param  Uhc                    The UHCI device.
  @param  DevAddr                Device Address.
  @param  EndPoint               EndPoint Address.
  @param  Toggle                 The next data toggle to use.

  @retval EFI_SUCCESS            The request is deleted.
  @retval EFI_INVALID_PARAMETER  Paremeter is error.
  @retval EFI_NOT_FOUND          The asynchronous isn't found.

**/
EFI_STATUS
UhciRemoveAsyncReq (
  IN  USB_HC_DEV  *Uhc,
  IN  UINT8       DevAddr,
  IN  UINT8       EndPoint,
  OUT UINT8       *Toggle
  )
{
  EFI_STATUS          Status;
  UHCI_ASYNC_REQUEST  *AsyncReq;
  UHCI_QH_RESULT      QhResult;
  LIST_ENTRY          *Link;
  BOOLEAN             Found;

  Status = EFI_SUCCESS;

  //
  // If no asynchronous interrupt transaction exists
  //
  if (IsListEmpty (&(Uhc->AsyncIntList))) {
    return EFI_SUCCESS;
  }

  //
  // Find the asynchronous transfer to this device/endpoint pair
  //
  Found = FALSE;
  Link  = Uhc->AsyncIntList.ForwardLink;

  do {
    AsyncReq = UHCI_ASYNC_INT_FROM_LINK (Link);
    Link     = Link->ForwardLink;

    if ((AsyncReq->DevAddr == DevAddr) && (AsyncReq->EndPoint == EndPoint)) {
      Found = TRUE;
      break;
    }
  } while (Link != &(Uhc->AsyncIntList));

  if (!Found) {
    return EFI_NOT_FOUND;
  }

  //
  // Check the result of the async transfer then update it
  // to get the next data toggle to use.
  //
  UhciCheckTdStatus (Uhc, AsyncReq->FirstTd, AsyncReq->IsLow, &QhResult);
  *Toggle = QhResult.NextToggle;

  //
  // Don't release the request now, keep it to synchronize with hardware.
  //
  UhciUnlinkAsyncReq (Uhc, AsyncReq, FALSE);
  return Status;
}

/**
  Recycle the asynchronouse request. When a queue head
  is unlinked from frame list, host controller hardware
  may still hold a cached pointer to it. To synchronize
  with hardware, the request is released in two steps:
  first it is linked to the UHC's RecycleWait list. At
  the next time UhciMonitorAsyncReqList is fired, it is
  moved to UHC's Recylelist. Then, at another timer
  activation, all the requests on Recycle list is freed.
  This guarrantes that each unlink queue head keeps
  existing for at least 50ms, far enough for the hardware
  to clear its cache.

  @param  Uhc                    The UHCI device.

**/
VOID
UhciRecycleAsyncReq (
  IN USB_HC_DEV  *Uhc
  )
{
  UHCI_ASYNC_REQUEST  *Req;
  UHCI_ASYNC_REQUEST  *Next;

  Req = Uhc->Recycle;

  while (Req != NULL) {
    Next = Req->Recycle;
    UhciFreeAsyncReq (Uhc, Req);
    Req = Next;
  }

  Uhc->Recycle     = Uhc->RecycleWait;
  Uhc->RecycleWait = NULL;
}

/**
  Release all the asynchronous transfers on the lsit.

  @param  Uhc                    The UHCI device.

**/
VOID
UhciFreeAllAsyncReq (
  IN USB_HC_DEV  *Uhc
  )
{
  LIST_ENTRY          *Head;
  UHCI_ASYNC_REQUEST  *AsyncReq;

  //
  // Call UhciRecycleAsyncReq twice. The requests on Recycle
  // will be released at the first call; The requests on
  // RecycleWait will be released at the second call.
  //
  UhciRecycleAsyncReq (Uhc);
  UhciRecycleAsyncReq (Uhc);

  Head = &(Uhc->AsyncIntList);

  if (IsListEmpty (Head)) {
    return;
  }

  while (!IsListEmpty (Head)) {
    AsyncReq = UHCI_ASYNC_INT_FROM_LINK (Head->ForwardLink);
    UhciUnlinkAsyncReq (Uhc, AsyncReq, TRUE);
  }
}

/**
  Interrupt transfer periodic check handler.

  @param  Event                  The event of the time.
  @param  Context                Context of the event, pointer to USB_HC_DEV.

**/
VOID
EFIAPI
UhciMonitorAsyncReqList (
  IN EFI_EVENT  Event,
  IN VOID       *Context
  )
{
  UHCI_ASYNC_REQUEST  *AsyncReq;
  LIST_ENTRY          *Link;
  USB_HC_DEV          *Uhc;
  VOID                *Data;
  BOOLEAN             Finished;
  UHCI_QH_RESULT      QhResult;

  Uhc = (USB_HC_DEV *)Context;

  //
  // Recycle the asynchronous requests expired, and promote
  // requests waiting to be recycled the next time when this
  // timer expires
  //
  UhciRecycleAsyncReq (Uhc);

  if (IsListEmpty (&(Uhc->AsyncIntList))) {
    return;
  }

  //
  // This loop must be delete safe
  //
  Link = Uhc->AsyncIntList.ForwardLink;

  do {
    AsyncReq = UHCI_ASYNC_INT_FROM_LINK (Link);
    Link     = Link->ForwardLink;

    Finished = UhciCheckTdStatus (Uhc, AsyncReq->FirstTd, AsyncReq->IsLow, &QhResult);

    if (!Finished) {
      continue;
    }

    //
    // Copy the data to temporary buffer if there are some
    // data transferred. We may have zero-length packet.
    // Make sure the data received from HW is no more than expected.
    //
    Data = NULL;

    if ((QhResult.Complete != 0) && (QhResult.Complete <= AsyncReq->DataLen)) {
      Data = AllocatePool (QhResult.Complete);

      if (Data == NULL) {
        return;
      }

      CopyMem (Data, AsyncReq->FirstTd->Data, QhResult.Complete);
    }

    UhciUpdateAsyncReq (Uhc, AsyncReq, QhResult.Result, QhResult.NextToggle);

    //
    // Now, either transfer is SUCCESS or met errors since
    // we have skipped to next transfer earlier if current
    // transfer is still active.
    //
    if (QhResult.Result == EFI_USB_NOERROR) {
      AsyncReq->Callback (Data, QhResult.Complete, AsyncReq->Context, QhResult.Result);
    } else {
      //
      // Leave error recovery to its related device driver.
      // A common case of the error recovery is to re-submit
      // the interrupt transfer. When an interrupt transfer
      // is re-submitted, its position in the linked list is
      // changed. It is inserted to the head of the linked
      // list, while this function scans the whole list from
      // head to tail. Thus, the re-submitted interrupt transfer's
      // callback function will not be called again in this round.
      //
      AsyncReq->Callback (NULL, 0, AsyncReq->Context, QhResult.Result);
    }

    if (Data != NULL) {
      gBS->FreePool (Data);
    }
  } while (Link != &(Uhc->AsyncIntList));
}
