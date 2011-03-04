/** @file

  EHCI transfer scheduling routines.

Copyright (c) 2007 - 2011, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Ehci.h"


/**
  Create helper QTD/QH for the EHCI device.

  @param  Ehc                   The EHCI device.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for helper QTD/QH.
  @retval EFI_SUCCESS           Helper QH/QTD are created.

**/
EFI_STATUS
EhcCreateHelpQ (
  IN USB2_HC_DEV          *Ehc
  )
{
  USB_ENDPOINT            Ep;
  EHC_QH                  *Qh;
  QH_HW                   *QhHw;
  EHC_QTD                 *Qtd;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  //
  // Create an inactive Qtd to terminate the short packet read.
  //
  Qtd = EhcCreateQtd (Ehc, NULL, NULL, 0, QTD_PID_INPUT, 0, 64);

  if (Qtd == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Qtd->QtdHw.Status   = QTD_STAT_HALTED;
  Ehc->ShortReadStop  = Qtd;

  //
  // Create a QH to act as the EHC reclamation header.
  // Set the header to loopback to itself.
  //
  Ep.DevAddr    = 0;
  Ep.EpAddr     = 1;
  Ep.Direction  = EfiUsbDataIn;
  Ep.DevSpeed   = EFI_USB_SPEED_HIGH;
  Ep.MaxPacket  = 64;
  Ep.HubAddr    = 0;
  Ep.HubPort    = 0;
  Ep.Toggle     = 0;
  Ep.Type       = EHC_BULK_TRANSFER;
  Ep.PollRate   = 1;

  Qh            = EhcCreateQh (Ehc, &Ep);

  if (Qh == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  PciAddr           = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Qh, sizeof (EHC_QH));
  QhHw              = &Qh->QhHw;
  QhHw->HorizonLink = QH_LINK (PciAddr + OFFSET_OF(EHC_QH, QhHw), EHC_TYPE_QH, FALSE);
  QhHw->Status      = QTD_STAT_HALTED;
  QhHw->ReclaimHead = 1;
  Qh->NextQh        = Qh;
  Ehc->ReclaimHead  = Qh;

  //
  // Create a dummy QH to act as the terminator for periodical schedule
  //
  Ep.EpAddr   = 2;
  Ep.Type     = EHC_INT_TRANSFER_SYNC;

  Qh          = EhcCreateQh (Ehc, &Ep);

  if (Qh == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Qh->QhHw.Status = QTD_STAT_HALTED;
  Ehc->PeriodOne  = Qh;

  return EFI_SUCCESS;
}


/**
  Initialize the schedule data structure such as frame list.

  @param  Ehc                   The EHCI device to init schedule data.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to init schedule data.
  @retval EFI_SUCCESS           The schedule data is initialized.

**/
EFI_STATUS
EhcInitSched (
  IN USB2_HC_DEV          *Ehc
  )
{
  EFI_PCI_IO_PROTOCOL   *PciIo;
  VOID                  *Buf;
  EFI_PHYSICAL_ADDRESS  PhyAddr;
  VOID                  *Map;
  UINTN                 Pages;
  UINTN                 Bytes;
  UINTN                 Index;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PciAddr;

  //
  // First initialize the periodical schedule data:
  // 1. Allocate and map the memory for the frame list
  // 2. Create the help QTD/QH
  // 3. Initialize the frame entries
  // 4. Set the frame list register
  //
  PciIo = Ehc->PciIo;

  Bytes = 4096;
  Pages = EFI_SIZE_TO_PAGES (Bytes);

  Status = PciIo->AllocateBuffer (
                    PciIo,
                    AllocateAnyPages,
                    EfiBootServicesData,
                    Pages,
                    &Buf,
                    0
                    );

  if (EFI_ERROR (Status)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = PciIo->Map (
                    PciIo,
                    EfiPciIoOperationBusMasterCommonBuffer,
                    Buf,
                    &Bytes,
                    &PhyAddr,
                    &Map
                    );

  if (EFI_ERROR (Status) || (Bytes != 4096)) {
    PciIo->FreeBuffer (PciIo, Pages, Buf);
    return EFI_OUT_OF_RESOURCES;
  }

  Ehc->PeriodFrame      = Buf;
  Ehc->PeriodFrameMap   = Map;

  //
  // Program the FRAMELISTBASE register with the low 32 bit addr
  //
  EhcWriteOpReg (Ehc, EHC_FRAME_BASE_OFFSET, EHC_LOW_32BIT (PhyAddr));
  //
  // Program the CTRLDSSEGMENT register with the high 32 bit addr
  //
  EhcWriteOpReg (Ehc, EHC_CTRLDSSEG_OFFSET, EHC_HIGH_32BIT (PhyAddr));

  //
  // Init memory pool management then create the helper
  // QTD/QH. If failed, previously allocated resources
  // will be freed by EhcFreeSched
  //
  Ehc->MemPool = UsbHcInitMemPool (
                   PciIo,
                   EHC_BIT_IS_SET (Ehc->HcCapParams, HCCP_64BIT),
                   EHC_HIGH_32BIT (PhyAddr)
                   );

  if (Ehc->MemPool == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit1;
  }

  Status = EhcCreateHelpQ (Ehc);

  if (EFI_ERROR (Status)) {
    goto ErrorExit;
  }

  //
  // Initialize the frame list entries then set the registers
  //
  Ehc->PeriodFrameHost      = AllocateZeroPool (EHC_FRAME_LEN * sizeof (UINTN));
  if (Ehc->PeriodFrameHost == NULL) {
    Status = EFI_OUT_OF_RESOURCES;
    goto ErrorExit;
  }

  PciAddr  = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Ehc->PeriodOne, sizeof (EHC_QH));

  for (Index = 0; Index < EHC_FRAME_LEN; Index++) {
    //
    // Store the pci bus address of the QH in period frame list which will be accessed by pci bus master.
    //
    ((UINT32 *)(Ehc->PeriodFrame))[Index] = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
    //
    // Store the host address of the QH in period frame list which will be accessed by host.
    //
    ((UINTN *)(Ehc->PeriodFrameHost))[Index] = (UINTN)Ehc->PeriodOne;
  }

  //
  // Second initialize the asynchronous schedule:
  // Only need to set the AsynListAddr register to
  // the reclamation header
  //
  PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Ehc->ReclaimHead, sizeof (EHC_QH));
  EhcWriteOpReg (Ehc, EHC_ASYNC_HEAD_OFFSET, EHC_LOW_32BIT (PciAddr));
  return EFI_SUCCESS;

ErrorExit:
  if (Ehc->PeriodOne != NULL) {
    UsbHcFreeMem (Ehc->MemPool, Ehc->PeriodOne, sizeof (EHC_QH));
    Ehc->PeriodOne = NULL;
  }

  if (Ehc->ReclaimHead != NULL) {
    UsbHcFreeMem (Ehc->MemPool, Ehc->ReclaimHead, sizeof (EHC_QH));
    Ehc->ReclaimHead = NULL;
  }

  if (Ehc->ShortReadStop != NULL) {
    UsbHcFreeMem (Ehc->MemPool, Ehc->ShortReadStop, sizeof (EHC_QTD));
    Ehc->ShortReadStop = NULL;
  }

ErrorExit1:
  PciIo->FreeBuffer (PciIo, Pages, Buf);
  PciIo->Unmap (PciIo, Map);

  return Status;
}


/**
  Free the schedule data. It may be partially initialized.

  @param  Ehc                   The EHCI device.

**/
VOID
EhcFreeSched (
  IN USB2_HC_DEV          *Ehc
  )
{
  EFI_PCI_IO_PROTOCOL     *PciIo;

  EhcWriteOpReg (Ehc, EHC_FRAME_BASE_OFFSET, 0);
  EhcWriteOpReg (Ehc, EHC_ASYNC_HEAD_OFFSET, 0);

  if (Ehc->PeriodOne != NULL) {
    UsbHcFreeMem (Ehc->MemPool, Ehc->PeriodOne, sizeof (EHC_QH));
    Ehc->PeriodOne = NULL;
  }

  if (Ehc->ReclaimHead != NULL) {
    UsbHcFreeMem (Ehc->MemPool, Ehc->ReclaimHead, sizeof (EHC_QH));
    Ehc->ReclaimHead = NULL;
  }

  if (Ehc->ShortReadStop != NULL) {
    UsbHcFreeMem (Ehc->MemPool, Ehc->ShortReadStop, sizeof (EHC_QTD));
    Ehc->ShortReadStop = NULL;
  }

  if (Ehc->MemPool != NULL) {
    UsbHcFreeMemPool (Ehc->MemPool);
    Ehc->MemPool = NULL;
  }

  if (Ehc->PeriodFrame != NULL) {
    PciIo = Ehc->PciIo;
    ASSERT (PciIo != NULL);

    PciIo->Unmap (PciIo, Ehc->PeriodFrameMap);

    PciIo->FreeBuffer (
             PciIo,
             EFI_SIZE_TO_PAGES (EFI_PAGE_SIZE),
             Ehc->PeriodFrame
             );

    Ehc->PeriodFrame = NULL;
  }

  if (Ehc->PeriodFrameHost != NULL) {
    FreePool (Ehc->PeriodFrameHost);
    Ehc->PeriodFrameHost = NULL;
  }
}


/**
  Link the queue head to the asynchronous schedule list.
  UEFI only supports one CTRL/BULK transfer at a time
  due to its interfaces. This simplifies the AsynList
  management: A reclamation header is always linked to
  the AsyncListAddr, the only active QH is appended to it.

  @param  Ehc                   The EHCI device.
  @param  Qh                    The queue head to link.

**/
VOID
EhcLinkQhToAsync (
  IN USB2_HC_DEV          *Ehc,
  IN EHC_QH               *Qh
  )
{
  EHC_QH                  *Head;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  //
  // Append the queue head after the reclaim header, then
  // fix the hardware visiable parts (EHCI R1.0 page 72).
  // ReclaimHead is always linked to the EHCI's AsynListAddr.
  //
  Head                    = Ehc->ReclaimHead;

  Qh->NextQh              = Head->NextQh;
  Head->NextQh            = Qh;

  PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Qh->NextQh, sizeof (EHC_QH));
  Qh->QhHw.HorizonLink    = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
  PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Head->NextQh, sizeof (EHC_QH));
  Head->QhHw.HorizonLink  = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
}


/**
  Unlink a queue head from the asynchronous schedule list.
  Need to synchronize with hardware.

  @param  Ehc                   The EHCI device.
  @param  Qh                    The queue head to unlink.

**/
VOID
EhcUnlinkQhFromAsync (
  IN USB2_HC_DEV          *Ehc,
  IN EHC_QH               *Qh
  )
{
  EHC_QH                  *Head;
  EFI_STATUS              Status;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  ASSERT (Ehc->ReclaimHead->NextQh == Qh);

  //
  // Remove the QH from reclamation head, then update the hardware
  // visiable part: Only need to loopback the ReclaimHead. The Qh
  // is pointing to ReclaimHead (which is staill in the list).
  //
  Head                    = Ehc->ReclaimHead;

  Head->NextQh            = Qh->NextQh;
  Qh->NextQh              = NULL;

  PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Head->NextQh, sizeof (EHC_QH));
  Head->QhHw.HorizonLink  = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);

  //
  // Set and wait the door bell to synchronize with the hardware
  //
  Status = EhcSetAndWaitDoorBell (Ehc, EHC_GENERIC_TIMEOUT);

  if (EFI_ERROR (Status)) {
    DEBUG ((EFI_D_ERROR, "EhcUnlinkQhFromAsync: Failed to synchronize with doorbell\n"));
  }
}


/**
  Link a queue head for interrupt transfer to the periodic
  schedule frame list. This code is very much the same as
  that in UHCI.

  @param  Ehc                   The EHCI device.
  @param  Qh                    The queue head to link.

**/
VOID
EhcLinkQhToPeriod (
  IN USB2_HC_DEV          *Ehc,
  IN EHC_QH               *Qh
  )
{
  UINTN                   Index;
  EHC_QH                  *Prev;
  EHC_QH                  *Next;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  for (Index = 0; Index < EHC_FRAME_LEN; Index += Qh->Interval) {
    //
    // First QH can't be NULL because we always keep PeriodOne
    // heads on the frame list
    //
    ASSERT (!EHC_LINK_TERMINATED (((UINT32*)Ehc->PeriodFrame)[Index]));
    Next  = (EHC_QH*)((UINTN*)Ehc->PeriodFrameHost)[Index];
    Prev  = NULL;

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
    while (Next->Interval > Qh->Interval) {
      Prev  = Next;
      Next  = Next->NextQh;
    }

    ASSERT (Next != NULL);

    //
    // The entry may have been linked into the frame by early insertation.
    // For example: if insert a Qh with Qh.Interval == 4, and there is a Qh
    // with Qh.Interval == 8 on the frame. If so, we are done with this frame.
    // It isn't necessary to compare all the QH with the same interval to
    // Qh. This is because if there is other QH with the same interval, Qh
    // should has been inserted after that at Frames[0] and at Frames[0] it is
    // impossible for (Next == Qh)
    //
    if (Next == Qh) {
      continue;
    }

    if (Next->Interval == Qh->Interval) {
      //
      // If there is a QH with the same interval, it locates at
      // Frames[0], and we can simply insert it after this QH. We
      // are all done.
      //
      ASSERT ((Index == 0) && (Qh->NextQh == NULL));

      Prev                    = Next;
      Next                    = Next->NextQh;

      Qh->NextQh              = Next;
      Prev->NextQh            = Qh;

      Qh->QhHw.HorizonLink    = Prev->QhHw.HorizonLink;
      PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Qh, sizeof (EHC_QH));
      Prev->QhHw.HorizonLink  = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
      break;
    }

    //
    // OK, find the right position, insert it in. If Qh's next
    // link has already been set, it is in position. This is
    // guarranted by 2^n polling interval.
    //
    if (Qh->NextQh == NULL) {
      Qh->NextQh              = Next;
      PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Next, sizeof (EHC_QH));
      Qh->QhHw.HorizonLink    = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
    }

    PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Qh, sizeof (EHC_QH));

    if (Prev == NULL) {
      ((UINT32*)Ehc->PeriodFrame)[Index]     = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
      ((UINTN*)Ehc->PeriodFrameHost)[Index]  = (UINTN)Qh;
    } else {
      Prev->NextQh            = Qh;
      Prev->QhHw.HorizonLink  = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
    }
  }
}


/**
  Unlink an interrupt queue head from the periodic
  schedule frame list.

  @param  Ehc                   The EHCI device.
  @param  Qh                    The queue head to unlink.

**/
VOID
EhcUnlinkQhFromPeriod (
  IN USB2_HC_DEV          *Ehc,
  IN EHC_QH               *Qh
  )
{
  UINTN                   Index;
  EHC_QH                  *Prev;
  EHC_QH                  *This;

  for (Index = 0; Index < EHC_FRAME_LEN; Index += Qh->Interval) {
    //
    // Frame link can't be NULL because we always keep PeroidOne
    // on the frame list
    //
    ASSERT (!EHC_LINK_TERMINATED (((UINT32*)Ehc->PeriodFrame)[Index]));
    This  = (EHC_QH*)((UINTN*)Ehc->PeriodFrameHost)[Index];
    Prev  = NULL;

    //
    // Walk through the frame's QH list to find the
    // queue head to remove
    //
    while ((This != NULL) && (This != Qh)) {
      Prev  = This;
      This  = This->NextQh;
    }

    //
    // Qh may have already been unlinked from this frame
    // by early action. See the comments in EhcLinkQhToPeriod.
    //
    if (This == NULL) {
      continue;
    }

    if (Prev == NULL) {
      //
      // Qh is the first entry in the frame
      //
      ((UINT32*)Ehc->PeriodFrame)[Index] = Qh->QhHw.HorizonLink;
      ((UINTN*)Ehc->PeriodFrameHost)[Index] = (UINTN)Qh->NextQh;
    } else {
      Prev->NextQh            = Qh->NextQh;
      Prev->QhHw.HorizonLink  = Qh->QhHw.HorizonLink;
    }
  }
}


/**
  Check the URB's execution result and update the URB's
  result accordingly.

  @param  Ehc                   The EHCI device.
  @param  Urb                   The URB to check result.

  @return Whether the result of URB transfer is finialized.

**/
BOOLEAN
EhcCheckUrbResult (
  IN  USB2_HC_DEV         *Ehc,
  IN  URB                 *Urb
  )
{
  LIST_ENTRY              *Entry;
  EHC_QTD                 *Qtd;
  QTD_HW                  *QtdHw;
  UINT8                   State;
  BOOLEAN                 Finished;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  ASSERT ((Ehc != NULL) && (Urb != NULL) && (Urb->Qh != NULL));

  Finished        = TRUE;
  Urb->Completed  = 0;

  Urb->Result     = EFI_USB_NOERROR;

  if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
    Urb->Result |= EFI_USB_ERR_SYSTEM;
    goto ON_EXIT;
  }

  EFI_LIST_FOR_EACH (Entry, &Urb->Qh->Qtds) {
    Qtd   = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);
    QtdHw = &Qtd->QtdHw;
    State = (UINT8) QtdHw->Status;

    if (EHC_BIT_IS_SET (State, QTD_STAT_HALTED)) {
      //
      // EHCI will halt the queue head when met some error.
      // If it is halted, the result of URB is finialized.
      //
      if ((State & QTD_STAT_ERR_MASK) == 0) {
        Urb->Result |= EFI_USB_ERR_STALL;
      }

      if (EHC_BIT_IS_SET (State, QTD_STAT_BABBLE_ERR)) {
        Urb->Result |= EFI_USB_ERR_BABBLE;
      }

      if (EHC_BIT_IS_SET (State, QTD_STAT_BUFF_ERR)) {
        Urb->Result |= EFI_USB_ERR_BUFFER;
      }

      if (EHC_BIT_IS_SET (State, QTD_STAT_TRANS_ERR) && (QtdHw->ErrCnt == 0)) {
        Urb->Result |= EFI_USB_ERR_TIMEOUT;
      }

      Finished = TRUE;
      goto ON_EXIT;

    } else if (EHC_BIT_IS_SET (State, QTD_STAT_ACTIVE)) {
      //
      // The QTD is still active, no need to check furthur.
      //
      Urb->Result |= EFI_USB_ERR_NOTEXECUTE;

      Finished = FALSE;
      goto ON_EXIT;

    } else {
      //
      // This QTD is finished OK or met short packet read. Update the
      // transfer length if it isn't a setup.
      //
      if (QtdHw->Pid != QTD_PID_SETUP) {
        Urb->Completed += Qtd->DataLen - QtdHw->TotalBytes;
      }

      if ((QtdHw->TotalBytes != 0) && (QtdHw->Pid == QTD_PID_INPUT)) {
        EhcDumpQh (Urb->Qh, "Short packet read", FALSE);

        //
        // Short packet read condition. If it isn't a setup transfer,
        // no need to check furthur: the queue head will halt at the
        // ShortReadStop. If it is a setup transfer, need to check the
        // Status Stage of the setup transfer to get the finial result
        //
        PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Ehc->ShortReadStop, sizeof (EHC_QTD));
        if (QtdHw->AltNext == QTD_LINK (PciAddr, FALSE)) {
          DEBUG ((EFI_D_INFO, "EhcCheckUrbResult: Short packet read, break\n"));

          Finished = TRUE;
          goto ON_EXIT;
        }

        DEBUG ((EFI_D_INFO, "EhcCheckUrbResult: Short packet read, continue\n"));
      }
    }
  }

ON_EXIT:
  //
  // Return the data toggle set by EHCI hardware, bulk and interrupt
  // transfer will use this to initialize the next transaction. For
  // Control transfer, it always start a new data toggle sequence for
  // new transfer.
  //
  // NOTICE: don't move DT update before the loop, otherwise there is
  // a race condition that DT is wrong.
  //
  Urb->DataToggle = (UINT8) Urb->Qh->QhHw.DataToggle;

  return Finished;
}


/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Ehc               The EHCI device.
  @param  Urb               The URB to execute.
  @param  TimeOut           The time to wait before abort, in millisecond.

  @return EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @return EFI_TIMEOUT       The transfer failed due to time out.
  @return EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
EhcExecTransfer (
  IN  USB2_HC_DEV         *Ehc,
  IN  URB                 *Urb,
  IN  UINTN               TimeOut
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   Loop;
  BOOLEAN                 Finished;
  BOOLEAN                 InfiniteLoop;

  Status       = EFI_SUCCESS;
  Loop         = (TimeOut * EHC_1_MILLISECOND / EHC_SYNC_POLL_INTERVAL) + 1;
  Finished     = FALSE;
  InfiniteLoop = FALSE;

  //
  // According to UEFI spec section 16.2.4, If Timeout is 0, then the caller
  // must wait for the function to be completed until EFI_SUCCESS or EFI_DEVICE_ERROR
  // is returned.
  //
  if (TimeOut == 0) {
    InfiniteLoop = TRUE;
  }

  for (Index = 0; InfiniteLoop || (Index < Loop); Index++) {
    Finished = EhcCheckUrbResult (Ehc, Urb);

    if (Finished) {
      break;
    }

    gBS->Stall (EHC_SYNC_POLL_INTERVAL);
  }

  if (!Finished) {
    DEBUG ((EFI_D_ERROR, "EhcExecTransfer: transfer not finished in %dms\n", (UINT32)TimeOut));
    EhcDumpQh (Urb->Qh, NULL, FALSE);

    Status = EFI_TIMEOUT;

  } else if (Urb->Result != EFI_USB_NOERROR) {
    DEBUG ((EFI_D_ERROR, "EhcExecTransfer: transfer failed with %x\n", Urb->Result));
    EhcDumpQh (Urb->Qh, NULL, FALSE);

    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}


/**
  Delete a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Ehc                   The EHCI device.
  @param  DevAddr               The address of the target device.
  @param  EpNum                 The endpoint of the target.
  @param  DataToggle            Return the next data toggle to use.

  @retval EFI_SUCCESS           An asynchronous transfer is removed.
  @retval EFI_NOT_FOUND         No transfer for the device is found.

**/
EFI_STATUS
EhciDelAsyncIntTransfer (
  IN  USB2_HC_DEV         *Ehc,
  IN  UINT8               DevAddr,
  IN  UINT8               EpNum,
  OUT UINT8               *DataToggle
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  URB                     *Urb;
  EFI_USB_DATA_DIRECTION  Direction;

  Direction = (((EpNum & 0x80) != 0) ? EfiUsbDataIn : EfiUsbDataOut);
  EpNum    &= 0x0F;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Ehc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    if ((Urb->Ep.DevAddr == DevAddr) && (Urb->Ep.EpAddr == EpNum) &&
        (Urb->Ep.Direction == Direction)) {
      //
      // Check the URB status to retrieve the next data toggle
      // from the associated queue head.
      //
      EhcCheckUrbResult (Ehc, Urb);
      *DataToggle = Urb->DataToggle;

      EhcUnlinkQhFromPeriod (Ehc, Urb->Qh);
      RemoveEntryList (&Urb->UrbList);

      gBS->FreePool (Urb->Data);
      EhcFreeUrb (Ehc, Urb);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}


/**
  Remove all the asynchronous interrutp transfers.

  @param  Ehc                   The EHCI device.

**/
VOID
EhciDelAllAsyncIntTransfers (
  IN USB2_HC_DEV          *Ehc
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  URB                     *Urb;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Ehc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    EhcUnlinkQhFromPeriod (Ehc, Urb->Qh);
    RemoveEntryList (&Urb->UrbList);

    gBS->FreePool (Urb->Data);
    EhcFreeUrb (Ehc, Urb);
  }
}


/**
  Flush data from PCI controller specific address to mapped system
  memory address.

  @param  Ehc                The EHCI device.
  @param  Urb                The URB to unmap.

  @retval EFI_SUCCESS        Success to flush data to mapped system memory.
  @retval EFI_DEVICE_ERROR   Fail to flush data to mapped system memory.

**/
EFI_STATUS
EhcFlushAsyncIntMap (
  IN  USB2_HC_DEV         *Ehc,
  IN  URB                 *Urb
  )
{
  EFI_STATUS                    Status;
  EFI_PHYSICAL_ADDRESS          PhyAddr;
  EFI_PCI_IO_PROTOCOL_OPERATION MapOp;
  EFI_PCI_IO_PROTOCOL           *PciIo;
  UINTN                         Len;
  VOID                          *Map;

  PciIo = Ehc->PciIo;
  Len   = Urb->DataLen;

  if (Urb->Ep.Direction == EfiUsbDataIn) {
    MapOp = EfiPciIoOperationBusMasterWrite;
  } else {
    MapOp = EfiPciIoOperationBusMasterRead;
  }

  Status = PciIo->Unmap (PciIo, Urb->DataMap);
  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  Urb->DataMap = NULL;

  Status = PciIo->Map (PciIo, MapOp, Urb->Data, &Len, &PhyAddr, &Map);
  if (EFI_ERROR (Status) || (Len != Urb->DataLen)) {
    goto ON_ERROR;
  }

  Urb->DataPhy  = (VOID *) ((UINTN) PhyAddr);
  Urb->DataMap  = Map;
  return EFI_SUCCESS;

ON_ERROR:
  return EFI_DEVICE_ERROR;
}


/**
  Update the queue head for next round of asynchronous transfer.

  @param  Ehc                   The EHCI device.
  @param  Urb                   The URB to update.

**/
VOID
EhcUpdateAsyncRequest (
  IN  USB2_HC_DEV         *Ehc,
  IN URB                  *Urb
  )
{
  LIST_ENTRY              *Entry;
  EHC_QTD                 *FirstQtd;
  QH_HW                   *QhHw;
  EHC_QTD                 *Qtd;
  QTD_HW                  *QtdHw;
  UINTN                   Index;
  EFI_PHYSICAL_ADDRESS    PciAddr;

  Qtd = NULL;

  if (Urb->Result == EFI_USB_NOERROR) {
    FirstQtd = NULL;

    EFI_LIST_FOR_EACH (Entry, &Urb->Qh->Qtds) {
      Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);

      if (FirstQtd == NULL) {
        FirstQtd = Qtd;
      }

      //
      // Update the QTD for next round of transfer. Host control
      // may change dt/Total Bytes to Transfer/C_Page/Cerr/Status/
      // Current Offset. These fields need to be updated. DT isn't
      // used by interrupt transfer. It uses DT in queue head.
      // Current Offset is in Page[0], only need to reset Page[0]
      // to initial data buffer.
      //
      QtdHw             = &Qtd->QtdHw;
      QtdHw->Status     = QTD_STAT_ACTIVE;
      QtdHw->ErrCnt     = QTD_MAX_ERR;
      QtdHw->CurPage    = 0;
      QtdHw->TotalBytes = (UINT32) Qtd->DataLen;
      //
      // calculate physical address by offset.
      //
      PciAddr = (UINTN)Urb->DataPhy + ((UINTN)Qtd->Data - (UINTN)Urb->Data); 
      QtdHw->Page[0]    = EHC_LOW_32BIT (PciAddr);
      QtdHw->PageHigh[0]= EHC_HIGH_32BIT (PciAddr);
    }

    //
    // Update QH for next round of transfer. Host control only
    // touch the fields in transfer overlay area. Only need to
    // zero out the overlay area and set NextQtd to the first
    // QTD. DateToggle bit is left untouched.
    //
    QhHw              = &Urb->Qh->QhHw;
    QhHw->CurQtd      = QTD_LINK (0, TRUE);
    QhHw->AltQtd      = 0;

    QhHw->Status      = 0;
    QhHw->Pid         = 0;
    QhHw->ErrCnt      = 0;
    QhHw->CurPage     = 0;
    QhHw->Ioc         = 0;
    QhHw->TotalBytes  = 0;

    for (Index = 0; Index < 5; Index++) {
      QhHw->Page[Index]     = 0;
      QhHw->PageHigh[Index] = 0;
    }

    PciAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, FirstQtd, sizeof (EHC_QTD));
    QhHw->NextQtd = QTD_LINK (PciAddr, FALSE);
  }

  return ;
}


/**
  Interrupt transfer periodic check handler.

  @param  Event                 Interrupt event.
  @param  Context               Pointer to USB2_HC_DEV.

**/
VOID
EFIAPI
EhcMonitorAsyncRequests (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  USB2_HC_DEV             *Ehc;
  EFI_TPL                 OldTpl;
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  BOOLEAN                 Finished;
  UINT8                   *ProcBuf;
  URB                     *Urb;
  EFI_STATUS              Status;

  OldTpl  = gBS->RaiseTPL (EHC_TPL);
  Ehc     = (USB2_HC_DEV *) Context;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Ehc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    //
    // Check the result of URB execution. If it is still
    // active, check the next one.
    //
    Finished = EhcCheckUrbResult (Ehc, Urb);

    if (!Finished) {
      continue;
    }

    //
    // Flush any PCI posted write transactions from a PCI host
    // bridge to system memory.
    //
    Status = EhcFlushAsyncIntMap (Ehc, Urb);
    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "EhcMonitorAsyncRequests: Fail to Flush AsyncInt Mapped Memeory\n"));
    }

    //
    // Allocate a buffer then copy the transferred data for user.
    // If failed to allocate the buffer, update the URB for next
    // round of transfer. Ignore the data of this round.
    //
    ProcBuf = NULL;

    if (Urb->Result == EFI_USB_NOERROR) {
      ASSERT (Urb->Completed <= Urb->DataLen);

      ProcBuf = AllocatePool (Urb->Completed);

      if (ProcBuf == NULL) {
        EhcUpdateAsyncRequest (Ehc, Urb);
        continue;
      }

      CopyMem (ProcBuf, Urb->Data, Urb->Completed);
    }

    EhcUpdateAsyncRequest (Ehc, Urb);

    //
    // Leave error recovery to its related device driver. A
    // common case of the error recovery is to re-submit the
    // interrupt transfer which is linked to the head of the
    // list. This function scans from head to tail. So the
    // re-submitted interrupt transfer's callback function
    // will not be called again in this round. Don't touch this
    // URB after the callback, it may have been removed by the
    // callback.
    //
    if (Urb->Callback != NULL) {
      //
      // Restore the old TPL, USB bus maybe connect device in
      // his callback. Some drivers may has a lower TPL restriction.
      //
      gBS->RestoreTPL (OldTpl);
      (Urb->Callback) (ProcBuf, Urb->Completed, Urb->Context, Urb->Result);
      OldTpl = gBS->RaiseTPL (EHC_TPL);
    }

    if (ProcBuf != NULL) {
      FreePool (ProcBuf);
    }
  }

  gBS->RestoreTPL (OldTpl);
}
