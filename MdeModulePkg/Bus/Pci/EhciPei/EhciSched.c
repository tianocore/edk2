/** @file
PEIM to produce gPeiUsb2HostControllerPpiGuid based on gPeiUsbControllerPpiGuid
which is used to enable recovery function from USB Drivers.

Copyright (c) 2010 - 2017, Intel Corporation. All rights reserved.<BR>
  
This program and the accompanying materials
are licensed and made available under the terms and conditions
of the BSD License which accompanies this distribution.  The
full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "EhcPeim.h"

/**
  Create helper QTD/QH for the EHCI device.
  
  @param  Ehc         The EHCI device.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for helper QTD/QH.
  @retval EFI_SUCCESS           Helper QH/QTD are created.

**/
EFI_STATUS
EhcCreateHelpQ (
  IN PEI_USB2_HC_DEV      *Ehc
  )
{
  USB_ENDPOINT            Ep;
  PEI_EHC_QH              *Qh;
  QH_HW                   *QhHw;
  PEI_EHC_QTD             *Qtd;

  //
  // Create an inactive Qtd to terminate the short packet read.
  //
  Qtd = EhcCreateQtd (Ehc, NULL, 0, QTD_PID_INPUT, 0, 64);

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

  QhHw              = &Qh->QhHw;
  QhHw->HorizonLink = QH_LINK (QhHw, EHC_TYPE_QH, FALSE);
  QhHw->Status      = QTD_STAT_HALTED;
  QhHw->ReclaimHead = 1;
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
  
  @param  Ehc                   The EHCI device to init schedule data for.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource to init schedule data.
  @retval EFI_SUCCESS           The schedule data is initialized.

**/
EFI_STATUS
EhcInitSched (
  IN PEI_USB2_HC_DEV      *Ehc
  )
{
  VOID                  *Buf;
  EFI_PHYSICAL_ADDRESS  PhyAddr;
  VOID                  *Map;
  UINTN                 Index;
  UINT32                *Desc;
  EFI_STATUS            Status;
  EFI_PHYSICAL_ADDRESS  PciAddr;

  //
  // First initialize the periodical schedule data:
  // 1. Allocate and map the memory for the frame list
  // 2. Create the help QTD/QH
  // 3. Initialize the frame entries
  // 4. Set the frame list register
  //
  //
  // The Frame List ocupies 4K bytes,
  // and must be aligned on 4-Kbyte boundaries.
  //
  Status = IoMmuAllocateBuffer (
             Ehc->IoMmu,
             1,
             &Buf,
             &PhyAddr,
             &Map
             );

  if (EFI_ERROR (Status) || (Buf == NULL)) {
    return EFI_OUT_OF_RESOURCES;
  }

  Ehc->PeriodFrame      = Buf;
  Ehc->PeriodFrameMap   = Map;
  Ehc->High32bitAddr    = EHC_HIGH_32BIT (PhyAddr);

  //
  // Init memory pool management then create the helper
  // QTD/QH. If failed, previously allocated resources
  // will be freed by EhcFreeSched
  //
  Ehc->MemPool = UsbHcInitMemPool (
                   Ehc,
                   EHC_BIT_IS_SET (Ehc->HcCapParams, HCCP_64BIT),
                   Ehc->High32bitAddr
                   );

  if (Ehc->MemPool == NULL) {
    return EFI_OUT_OF_RESOURCES;
  }

  Status = EhcCreateHelpQ (Ehc);

  if (EFI_ERROR (Status)) {
    return Status;
  }
  
  //
  // Initialize the frame list entries then set the registers
  //
  Desc = (UINT32 *) Ehc->PeriodFrame;
  PciAddr  = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Ehc->PeriodOne, sizeof (PEI_EHC_QH));
  for (Index = 0; Index < EHC_FRAME_LEN; Index++) {
    Desc[Index] = QH_LINK (PciAddr, EHC_TYPE_QH, FALSE);
  }

  EhcWriteOpReg (Ehc, EHC_FRAME_BASE_OFFSET, EHC_LOW_32BIT (PhyAddr));

  //
  // Second initialize the asynchronous schedule:
  // Only need to set the AsynListAddr register to
  // the reclamation header
  //
  PciAddr  = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Ehc->ReclaimHead, sizeof (PEI_EHC_QH));
  EhcWriteOpReg (Ehc, EHC_ASYNC_HEAD_OFFSET, EHC_LOW_32BIT (PciAddr));
  return EFI_SUCCESS;
}

/**
  Free the schedule data. It may be partially initialized.
  
  @param  Ehc   The EHCI device. 

**/
VOID
EhcFreeSched (
  IN PEI_USB2_HC_DEV      *Ehc
  )
{
  EhcWriteOpReg (Ehc, EHC_FRAME_BASE_OFFSET, 0);
  EhcWriteOpReg (Ehc, EHC_ASYNC_HEAD_OFFSET, 0);

  if (Ehc->PeriodOne != NULL) {
    UsbHcFreeMem (Ehc, Ehc->MemPool, Ehc->PeriodOne, sizeof (PEI_EHC_QH));
    Ehc->PeriodOne = NULL;
  }

  if (Ehc->ReclaimHead != NULL) {
    UsbHcFreeMem (Ehc, Ehc->MemPool, Ehc->ReclaimHead, sizeof (PEI_EHC_QH));
    Ehc->ReclaimHead = NULL;
  }

  if (Ehc->ShortReadStop != NULL) {
    UsbHcFreeMem (Ehc, Ehc->MemPool, Ehc->ShortReadStop, sizeof (PEI_EHC_QTD));
    Ehc->ShortReadStop = NULL;
  }

  if (Ehc->MemPool != NULL) {
    UsbHcFreeMemPool (Ehc, Ehc->MemPool);
    Ehc->MemPool = NULL;
  }

  if (Ehc->PeriodFrame != NULL) {
    IoMmuFreeBuffer (Ehc->IoMmu, 1, Ehc->PeriodFrame, Ehc->PeriodFrameMap);
    Ehc->PeriodFrame = NULL;
  }
}

/**
  Link the queue head to the asynchronous schedule list.
  UEFI only supports one CTRL/BULK transfer at a time
  due to its interfaces. This simplifies the AsynList
  management: A reclamation header is always linked to
  the AsyncListAddr, the only active QH is appended to it.
  
  @param  Ehc   The EHCI device.
  @param  Qh    The queue head to link.

**/
VOID
EhcLinkQhToAsync (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN PEI_EHC_QH           *Qh
  )
{
  PEI_EHC_QH               *Head;

  //
  // Append the queue head after the reclaim header, then
  // fix the hardware visiable parts (EHCI R1.0 page 72).
  // ReclaimHead is always linked to the EHCI's AsynListAddr.
  //
  Head                    = Ehc->ReclaimHead;

  Qh->NextQh              = Head->NextQh;
  Head->NextQh            = Qh;

  Qh->QhHw.HorizonLink    = QH_LINK (Head, EHC_TYPE_QH, FALSE);;
  Head->QhHw.HorizonLink  = QH_LINK (Qh, EHC_TYPE_QH, FALSE);
}

/**
  Unlink a queue head from the asynchronous schedule list.
  Need to synchronize with hardware.
  
  @param  Ehc   The EHCI device.
  @param  Qh    The queue head to unlink.

**/
VOID
EhcUnlinkQhFromAsync (
  IN PEI_USB2_HC_DEV      *Ehc,
  IN PEI_EHC_QH           *Qh
  )
{
  PEI_EHC_QH              *Head;

  ASSERT (Ehc->ReclaimHead->NextQh == Qh);

  //
  // Remove the QH from reclamation head, then update the hardware
  // visiable part: Only need to loopback the ReclaimHead. The Qh
  // is pointing to ReclaimHead (which is staill in the list).
  //
  Head                    = Ehc->ReclaimHead;

  Head->NextQh            = Qh->NextQh;
  Qh->NextQh              = NULL;

  Head->QhHw.HorizonLink  = QH_LINK (Head, EHC_TYPE_QH, FALSE);

  //
  // Set and wait the door bell to synchronize with the hardware
  //
  EhcSetAndWaitDoorBell (Ehc, EHC_GENERIC_TIMEOUT);
  
  return;
}

/**
  Check the URB's execution result and update the URB's
  result accordingly. 

  @param Ehc   The EHCI device.
  @param Urb   The URB to check result.

  @retval TRUE    URB transfer is finialized.
  @retval FALSE   URB transfer is not finialized.

**/
BOOLEAN
EhcCheckUrbResult (
  IN  PEI_USB2_HC_DEV     *Ehc,
  IN  PEI_URB             *Urb
  )
{
  EFI_LIST_ENTRY          *Entry;
  PEI_EHC_QTD             *Qtd;
  QTD_HW                  *QtdHw;
  UINT8                   State;
  BOOLEAN                 Finished;

  ASSERT ((Ehc != NULL) && (Urb != NULL) && (Urb->Qh != NULL));

  Finished        = TRUE;
  Urb->Completed  = 0;

  Urb->Result     = EFI_USB_NOERROR;

  if (EhcIsHalt (Ehc) || EhcIsSysError (Ehc)) {
    Urb->Result |= EFI_USB_ERR_SYSTEM;
    goto ON_EXIT;
  }

  EFI_LIST_FOR_EACH (Entry, &Urb->Qh->Qtds) {
    Qtd   = EFI_LIST_CONTAINER (Entry, PEI_EHC_QTD, QtdList);
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
        //EHC_DUMP_QH ((Urb->Qh, "Short packet read", FALSE));

        //
        // Short packet read condition. If it isn't a setup transfer,
        // no need to check furthur: the queue head will halt at the
        // ShortReadStop. If it is a setup transfer, need to check the
        // Status Stage of the setup transfer to get the finial result
        //
        if (QtdHw->AltNext == QTD_LINK (Ehc->ShortReadStop, FALSE)) {
          
          Finished = TRUE;
          goto ON_EXIT;
        }
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

  @retval EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @retval EFI_TIMEOUT       The transfer failed due to time out.
  @retval EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
EhcExecTransfer (
  IN  PEI_USB2_HC_DEV     *Ehc,
  IN  PEI_URB             *Urb,
  IN  UINTN               TimeOut
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   Loop;
  BOOLEAN                 Finished;
  BOOLEAN                 InfiniteLoop;

  Status    = EFI_SUCCESS;
  Loop      = TimeOut * EHC_1_MILLISECOND;
  Finished     = FALSE;
  InfiniteLoop = FALSE;

  //
  // If Timeout is 0, then the caller must wait for the function to be completed
  // until EFI_SUCCESS or EFI_DEVICE_ERROR is returned.
  //
  if (TimeOut == 0) {
    InfiniteLoop = TRUE;
  }

  for (Index = 0; InfiniteLoop || (Index < Loop); Index++) {
    Finished = EhcCheckUrbResult (Ehc, Urb);

    if (Finished) {
      break;
    }

    MicroSecondDelay (EHC_1_MICROSECOND);
  }

  if (!Finished) {
    Status = EFI_TIMEOUT;
  } else if (Urb->Result != EFI_USB_NOERROR) {
    Status = EFI_DEVICE_ERROR;
  }

  return Status;
}

