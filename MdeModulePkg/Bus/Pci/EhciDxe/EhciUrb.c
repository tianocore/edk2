/** @file

    This file contains URB request, each request is warpped in a
    URB (Usb Request Block).

Copyright (c) 2007 - 2010, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ehci.h"

/**
  Create a single QTD to hold the data.

  @param  Ehc                   The EHCI device.
  @param  Data                  The cpu memory address of current data not associated with a QTD.
  @param  DataPhy               The pci bus address of current data not associated with a QTD.
  @param  DataLen               The length of the data.
  @param  PktId                 Packet ID to use in the QTD.
  @param  Toggle                Data toggle to use in the QTD.
  @param  MaxPacket             Maximu packet length of the endpoint.

  @return Created QTD or NULL if failed to create one.

**/
EHC_QTD *
EhcCreateQtd (
  IN USB2_HC_DEV  *Ehc,
  IN UINT8        *Data,
  IN UINT8        *DataPhy,
  IN UINTN        DataLen,
  IN UINT8        PktId,
  IN UINT8        Toggle,
  IN UINTN        MaxPacket
  )
{
  EHC_QTD  *Qtd;
  QTD_HW   *QtdHw;
  UINTN    Index;
  UINTN    Len;
  UINTN    ThisBufLen;

  ASSERT (Ehc != NULL);

  Qtd = UsbHcAllocateMem (Ehc->MemPool, sizeof (EHC_QTD));

  if (Qtd == NULL) {
    return NULL;
  }

  Qtd->Signature = EHC_QTD_SIG;
  Qtd->Data      = Data;
  Qtd->DataLen   = 0;

  InitializeListHead (&Qtd->QtdList);

  QtdHw             = &Qtd->QtdHw;
  QtdHw->NextQtd    = QTD_LINK (NULL, TRUE);
  QtdHw->AltNext    = QTD_LINK (NULL, TRUE);
  QtdHw->Status     = QTD_STAT_ACTIVE;
  QtdHw->Pid        = PktId;
  QtdHw->ErrCnt     = QTD_MAX_ERR;
  QtdHw->Ioc        = 0;
  QtdHw->TotalBytes = 0;
  QtdHw->DataToggle = Toggle;

  //
  // Fill in the buffer points
  //
  if (Data != NULL) {
    Len = 0;

    for (Index = 0; Index <= QTD_MAX_BUFFER; Index++) {
      //
      // Set the buffer point (Check page 41 EHCI Spec 1.0). No need to
      // compute the offset and clear Reserved fields. This is already
      // done in the data point.
      //
      QtdHw->Page[Index]     = EHC_LOW_32BIT (DataPhy);
      QtdHw->PageHigh[Index] = EHC_HIGH_32BIT (DataPhy);

      ThisBufLen = QTD_BUF_LEN - (EHC_LOW_32BIT (DataPhy) & QTD_BUF_MASK);

      if (Len + ThisBufLen >= DataLen) {
        Len = DataLen;
        break;
      }

      Len     += ThisBufLen;
      Data    += ThisBufLen;
      DataPhy += ThisBufLen;
    }

    //
    // Need to fix the last pointer if the Qtd can't hold all the
    // user's data to make sure that the length is in the unit of
    // max packets. If it can hold all the data, there is no such
    // need.
    //
    if (Len < DataLen) {
      Len = Len - Len % MaxPacket;
    }

    QtdHw->TotalBytes = (UINT32)Len;
    Qtd->DataLen      = Len;
  }

  return Qtd;
}

/**
  Initialize the queue head for interrupt transfer,
  that is, initialize the following three fields:
  1. SplitXState in the Status field
  2. Microframe S-mask
  3. Microframe C-mask

  @param  Ep                    The queue head's related endpoint.
  @param  QhHw                  The queue head to initialize.

**/
VOID
EhcInitIntQh (
  IN USB_ENDPOINT  *Ep,
  IN QH_HW         *QhHw
  )
{
  //
  // Because UEFI interface can't utilitize an endpoint with
  // poll rate faster than 1ms, only need to set one bit in
  // the queue head. simple. But it may be changed later. If
  // sub-1ms interrupt is supported, need to update the S-Mask
  // here
  //
  if (Ep->DevSpeed == EFI_USB_SPEED_HIGH) {
    QhHw->SMask = QH_MICROFRAME_0;
    return;
  }

  //
  // For low/full speed device, the transfer must go through
  // the split transaction. Need to update three fields
  // 1. SplitXState in the status
  // 2. Microframe S-Mask
  // 3. Microframe C-Mask
  // UEFI USB doesn't exercise admission control. It simplely
  // schedule the high speed transactions in microframe 0, and
  // full/low speed transactions at microframe 1. This also
  // avoid the use of FSTN.
  //
  QhHw->SMask = QH_MICROFRAME_1;
  QhHw->CMask = QH_MICROFRAME_3 | QH_MICROFRAME_4 | QH_MICROFRAME_5;
}

/**
  Allocate and initialize a EHCI queue head.

  @param  Ehci                  The EHCI device.
  @param  Ep                    The endpoint to create queue head for.

  @return Created queue head or NULL if failed to create one.

**/
EHC_QH *
EhcCreateQh (
  IN USB2_HC_DEV   *Ehci,
  IN USB_ENDPOINT  *Ep
  )
{
  EHC_QH  *Qh;
  QH_HW   *QhHw;

  Qh = UsbHcAllocateMem (Ehci->MemPool, sizeof (EHC_QH));

  if (Qh == NULL) {
    return NULL;
  }

  Qh->Signature = EHC_QH_SIG;
  Qh->NextQh    = NULL;
  Qh->Interval  = Ep->PollRate;

  InitializeListHead (&Qh->Qtds);

  QhHw               = &Qh->QhHw;
  QhHw->HorizonLink  = QH_LINK (NULL, 0, TRUE);
  QhHw->DeviceAddr   = Ep->DevAddr;
  QhHw->Inactive     = 0;
  QhHw->EpNum        = Ep->EpAddr;
  QhHw->EpSpeed      = Ep->DevSpeed;
  QhHw->DtCtrl       = 0;
  QhHw->ReclaimHead  = 0;
  QhHw->MaxPacketLen = (UINT32)Ep->MaxPacket;
  QhHw->CtrlEp       = 0;
  QhHw->NakReload    = QH_NAK_RELOAD;
  QhHw->HubAddr      = Ep->HubAddr;
  QhHw->PortNum      = Ep->HubPort;
  QhHw->Multiplier   = 1;
  QhHw->DataToggle   = Ep->Toggle;

  if (Ep->DevSpeed != EFI_USB_SPEED_HIGH) {
    QhHw->Status |= QTD_STAT_DO_SS;
  }

  switch (Ep->Type) {
    case EHC_CTRL_TRANSFER:
      //
      // Special initialization for the control transfer:
      // 1. Control transfer initialize data toggle from each QTD
      // 2. Set the Control Endpoint Flag (C) for low/full speed endpoint.
      //
      QhHw->DtCtrl = 1;

      if (Ep->DevSpeed != EFI_USB_SPEED_HIGH) {
        QhHw->CtrlEp = 1;
      }

      break;

    case EHC_INT_TRANSFER_ASYNC:
    case EHC_INT_TRANSFER_SYNC:
      //
      // Special initialization for the interrupt transfer
      // to set the S-Mask and C-Mask
      //
      QhHw->NakReload = 0;
      EhcInitIntQh (Ep, QhHw);
      break;

    case EHC_BULK_TRANSFER:
      if ((Ep->DevSpeed == EFI_USB_SPEED_HIGH) && (Ep->Direction == EfiUsbDataOut)) {
        QhHw->Status |= QTD_STAT_DO_PING;
      }

      break;
  }

  return Qh;
}

/**
  Convert the poll interval from application to that
  be used by EHCI interface data structure. Only need
  to get the max 2^n that is less than interval. UEFI
  can't support high speed endpoint with a interval less
  than 8 microframe because interval is specified in
  the unit of ms (millisecond).

  @param  Interval              The interval to convert.

  @return The converted interval.

**/
UINTN
EhcConvertPollRate (
  IN  UINTN  Interval
  )
{
  UINTN  BitCount;

  if (Interval == 0) {
    return 1;
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
  Free a list of QTDs.

  @param  Ehc                   The EHCI device.
  @param  Qtds                  The list head of the QTD.

**/
VOID
EhcFreeQtds (
  IN USB2_HC_DEV  *Ehc,
  IN LIST_ENTRY   *Qtds
  )
{
  LIST_ENTRY  *Entry;
  LIST_ENTRY  *Next;
  EHC_QTD     *Qtd;

  BASE_LIST_FOR_EACH_SAFE (Entry, Next, Qtds) {
    Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);

    RemoveEntryList (&Qtd->QtdList);
    UsbHcFreeMem (Ehc->MemPool, Qtd, sizeof (EHC_QTD));
  }
}

/**
  Free an allocated URB. It is possible for it to be partially inited.

  @param  Ehc                   The EHCI device.
  @param  Urb                   The URB to free.

**/
VOID
EhcFreeUrb (
  IN USB2_HC_DEV  *Ehc,
  IN URB          *Urb
  )
{
  EFI_PCI_IO_PROTOCOL  *PciIo;

  PciIo = Ehc->PciIo;

  if (Urb->RequestPhy != NULL) {
    PciIo->Unmap (PciIo, Urb->RequestMap);
  }

  if (Urb->DataMap != NULL) {
    PciIo->Unmap (PciIo, Urb->DataMap);
  }

  if (Urb->Qh != NULL) {
    //
    // Ensure that this queue head has been unlinked from the
    // schedule data structures. Free all the associated QTDs
    //
    EhcFreeQtds (Ehc, &Urb->Qh->Qtds);
    UsbHcFreeMem (Ehc->MemPool, Urb->Qh, sizeof (EHC_QH));
  }

  gBS->FreePool (Urb);
}

/**
  Create a list of QTDs for the URB.

  @param  Ehc                   The EHCI device.
  @param  Urb                   The URB to create QTDs for.

  @retval EFI_OUT_OF_RESOURCES  Failed to allocate resource for QTD.
  @retval EFI_SUCCESS           The QTDs are allocated for the URB.

**/
EFI_STATUS
EhcCreateQtds (
  IN USB2_HC_DEV  *Ehc,
  IN URB          *Urb
  )
{
  USB_ENDPOINT          *Ep;
  EHC_QH                *Qh;
  EHC_QTD               *Qtd;
  EHC_QTD               *StatusQtd;
  EHC_QTD               *NextQtd;
  LIST_ENTRY            *Entry;
  UINT32                AlterNext;
  UINT8                 Toggle;
  UINTN                 Len;
  UINT8                 Pid;
  EFI_PHYSICAL_ADDRESS  PhyAddr;

  ASSERT ((Urb != NULL) && (Urb->Qh != NULL));

  //
  // EHCI follows the alternet next QTD pointer if it meets
  // a short read and the AlterNext pointer is valid. UEFI
  // EHCI driver should terminate the transfer except the
  // control transfer.
  //
  Toggle    = 0;
  Qh        = Urb->Qh;
  Ep        = &Urb->Ep;
  StatusQtd = NULL;
  AlterNext = QTD_LINK (NULL, TRUE);

  PhyAddr = UsbHcGetPciAddressForHostMem (Ehc->MemPool, Ehc->ShortReadStop, sizeof (EHC_QTD));
  if (Ep->Direction == EfiUsbDataIn) {
    AlterNext = QTD_LINK (PhyAddr, FALSE);
  }

  //
  // Build the Setup and status packets for control transfer
  //
  if (Urb->Ep.Type == EHC_CTRL_TRANSFER) {
    Len = sizeof (EFI_USB_DEVICE_REQUEST);
    Qtd = EhcCreateQtd (Ehc, (UINT8 *)Urb->Request, (UINT8 *)Urb->RequestPhy, Len, QTD_PID_SETUP, 0, Ep->MaxPacket);

    if (Qtd == NULL) {
      return EFI_OUT_OF_RESOURCES;
    }

    InsertTailList (&Qh->Qtds, &Qtd->QtdList);

    //
    // Create the status packet now. Set the AlterNext to it. So, when
    // EHCI meets a short control read, it can resume at the status stage.
    // Use the opposite direction of the data stage, or IN if there is
    // no data stage.
    //
    if (Ep->Direction == EfiUsbDataIn) {
      Pid = QTD_PID_OUTPUT;
    } else {
      Pid = QTD_PID_INPUT;
    }

    StatusQtd = EhcCreateQtd (Ehc, NULL, NULL, 0, Pid, 1, Ep->MaxPacket);

    if (StatusQtd == NULL) {
      goto ON_ERROR;
    }

    if (Ep->Direction == EfiUsbDataIn) {
      PhyAddr   = UsbHcGetPciAddressForHostMem (Ehc->MemPool, StatusQtd, sizeof (EHC_QTD));
      AlterNext = QTD_LINK (PhyAddr, FALSE);
    }

    Toggle = 1;
  }

  //
  // Build the data packets for all the transfers
  //
  if (Ep->Direction == EfiUsbDataIn) {
    Pid = QTD_PID_INPUT;
  } else {
    Pid = QTD_PID_OUTPUT;
  }

  Qtd = NULL;
  Len = 0;

  while (Len < Urb->DataLen) {
    Qtd = EhcCreateQtd (
            Ehc,
            (UINT8 *)Urb->Data + Len,
            (UINT8 *)Urb->DataPhy + Len,
            Urb->DataLen - Len,
            Pid,
            Toggle,
            Ep->MaxPacket
            );

    if (Qtd == NULL) {
      goto ON_ERROR;
    }

    Qtd->QtdHw.AltNext = AlterNext;
    InsertTailList (&Qh->Qtds, &Qtd->QtdList);

    //
    // Switch the Toggle bit if odd number of packets are included in the QTD.
    //
    if (((Qtd->DataLen + Ep->MaxPacket - 1) / Ep->MaxPacket) % 2) {
      Toggle = (UINT8)(1 - Toggle);
    }

    Len += Qtd->DataLen;
  }

  //
  // Insert the status packet for control transfer
  //
  if (Ep->Type == EHC_CTRL_TRANSFER) {
    InsertTailList (&Qh->Qtds, &StatusQtd->QtdList);
  }

  //
  // OK, all the QTDs needed are created. Now, fix the NextQtd point
  //
  BASE_LIST_FOR_EACH (Entry, &Qh->Qtds) {
    Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);

    //
    // break if it is the last entry on the list
    //
    if (Entry->ForwardLink == &Qh->Qtds) {
      break;
    }

    NextQtd            = EFI_LIST_CONTAINER (Entry->ForwardLink, EHC_QTD, QtdList);
    PhyAddr            = UsbHcGetPciAddressForHostMem (Ehc->MemPool, NextQtd, sizeof (EHC_QTD));
    Qtd->QtdHw.NextQtd = QTD_LINK (PhyAddr, FALSE);
  }

  //
  // Link the QTDs to the queue head
  //
  NextQtd          = EFI_LIST_CONTAINER (Qh->Qtds.ForwardLink, EHC_QTD, QtdList);
  PhyAddr          = UsbHcGetPciAddressForHostMem (Ehc->MemPool, NextQtd, sizeof (EHC_QTD));
  Qh->QhHw.NextQtd = QTD_LINK (PhyAddr, FALSE);
  return EFI_SUCCESS;

ON_ERROR:
  EhcFreeQtds (Ehc, &Qh->Qtds);
  return EFI_OUT_OF_RESOURCES;
}

/**
  Create a new URB and its associated QTD.

  @param  Ehc                   The EHCI device.
  @param  DevAddr               The device address.
  @param  EpAddr                Endpoint addrress & its direction.
  @param  DevSpeed              The device speed.
  @param  Toggle                Initial data toggle to use.
  @param  MaxPacket             The max packet length of the endpoint.
  @param  Hub                   The transaction translator to use.
  @param  Type                  The transaction type.
  @param  Request               The standard USB request for control transfer.
  @param  Data                  The user data to transfer.
  @param  DataLen               The length of data buffer.
  @param  Callback              The function to call when data is transferred.
  @param  Context               The context to the callback.
  @param  Interval              The interval for interrupt transfer.

  @return Created URB or NULL.

**/
URB *
EhcCreateUrb (
  IN USB2_HC_DEV                         *Ehc,
  IN UINT8                               DevAddr,
  IN UINT8                               EpAddr,
  IN UINT8                               DevSpeed,
  IN UINT8                               Toggle,
  IN UINTN                               MaxPacket,
  IN EFI_USB2_HC_TRANSACTION_TRANSLATOR  *Hub,
  IN UINTN                               Type,
  IN EFI_USB_DEVICE_REQUEST              *Request,
  IN VOID                                *Data,
  IN UINTN                               DataLen,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK     Callback,
  IN VOID                                *Context,
  IN UINTN                               Interval
  )
{
  USB_ENDPOINT                   *Ep;
  EFI_PHYSICAL_ADDRESS           PhyAddr;
  EFI_PCI_IO_PROTOCOL_OPERATION  MapOp;
  EFI_PCI_IO_PROTOCOL            *PciIo;
  EFI_STATUS                     Status;
  UINTN                          Len;
  URB                            *Urb;
  VOID                           *Map;

  Urb = AllocateZeroPool (sizeof (URB));

  if (Urb == NULL) {
    return NULL;
  }

  Urb->Signature = EHC_URB_SIG;
  InitializeListHead (&Urb->UrbList);

  Ep            = &Urb->Ep;
  Ep->DevAddr   = DevAddr;
  Ep->EpAddr    = (UINT8)(EpAddr & 0x0F);
  Ep->Direction = (((EpAddr & 0x80) != 0) ? EfiUsbDataIn : EfiUsbDataOut);
  Ep->DevSpeed  = DevSpeed;
  Ep->MaxPacket = MaxPacket;

  Ep->HubAddr = 0;
  Ep->HubPort = 0;

  if (DevSpeed != EFI_USB_SPEED_HIGH) {
    ASSERT (Hub != NULL);

    Ep->HubAddr = Hub->TranslatorHubAddress;
    Ep->HubPort = Hub->TranslatorPortNumber;
  }

  Ep->Toggle   = Toggle;
  Ep->Type     = Type;
  Ep->PollRate = EhcConvertPollRate (Interval);

  Urb->Request  = Request;
  Urb->Data     = Data;
  Urb->DataLen  = DataLen;
  Urb->Callback = Callback;
  Urb->Context  = Context;

  PciIo   = Ehc->PciIo;
  Urb->Qh = EhcCreateQh (Ehc, &Urb->Ep);

  if (Urb->Qh == NULL) {
    goto ON_ERROR;
  }

  //
  // Map the request and user data
  //
  if (Request != NULL) {
    Len    = sizeof (EFI_USB_DEVICE_REQUEST);
    MapOp  = EfiPciIoOperationBusMasterRead;
    Status = PciIo->Map (PciIo, MapOp, Request, &Len, &PhyAddr, &Map);

    if (EFI_ERROR (Status) || (Len != sizeof (EFI_USB_DEVICE_REQUEST))) {
      goto ON_ERROR;
    }

    Urb->RequestPhy = (VOID *)((UINTN)PhyAddr);
    Urb->RequestMap = Map;
  }

  if (Data != NULL) {
    Len = DataLen;

    if (Ep->Direction == EfiUsbDataIn) {
      MapOp = EfiPciIoOperationBusMasterWrite;
    } else {
      MapOp = EfiPciIoOperationBusMasterRead;
    }

    Status = PciIo->Map (PciIo, MapOp, Data, &Len, &PhyAddr, &Map);

    if (EFI_ERROR (Status) || (Len != DataLen)) {
      goto ON_ERROR;
    }

    Urb->DataPhy = (VOID *)((UINTN)PhyAddr);
    Urb->DataMap = Map;
  }

  Status = EhcCreateQtds (Ehc, Urb);

  if (EFI_ERROR (Status)) {
    goto ON_ERROR;
  }

  return Urb;

ON_ERROR:
  EhcFreeUrb (Ehc, Urb);
  return NULL;
}
