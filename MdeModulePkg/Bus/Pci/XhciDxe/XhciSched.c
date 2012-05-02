/** @file

  XHCI transfer scheduling routines.

Copyright (c) 2011 - 2012, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Xhci.h"

/**
  Create a command transfer TRB to support XHCI command interfaces.

  @param  Xhc       The XHCI Instance.
  @param  CmdTrb    The cmd TRB to be executed.

  @return Created URB or NULL.

**/
URB*
XhcCreateCmdTrb (
  IN USB_XHCI_INSTANCE  *Xhc,
  IN TRB_TEMPLATE       *CmdTrb
  )
{
  URB    *Urb;

  Urb = AllocateZeroPool (sizeof (URB));
  if (Urb == NULL) {
    return NULL;
  }

  Urb->Signature  = XHC_URB_SIG;

  Urb->Ring       = &Xhc->CmdRing;
  XhcSyncTrsRing (Xhc, Urb->Ring);
  Urb->TrbNum     = 1;
  Urb->TrbStart   = Urb->Ring->RingEnqueue;
  CopyMem (Urb->TrbStart, CmdTrb, sizeof (TRB_TEMPLATE));
  Urb->TrbStart->CycleBit = Urb->Ring->RingPCS & BIT0;
  Urb->TrbEnd             = Urb->TrbStart;

  return Urb;
}

/**
  Execute a XHCI cmd TRB pointed by CmdTrb.

  @param  Xhc                   The XHCI Instance.
  @param  CmdTrb                The cmd TRB to be executed.
  @param  Timeout               Indicates the maximum time, in millisecond, which the
                                transfer is allowed to complete.
  @param  EvtTrb                The event TRB corresponding to the cmd TRB.

  @retval EFI_SUCCESS           The transfer was completed successfully.
  @retval EFI_INVALID_PARAMETER Some parameters are invalid.
  @retval EFI_TIMEOUT           The transfer failed due to timeout.
  @retval EFI_DEVICE_ERROR      The transfer failed due to host controller error.

**/
EFI_STATUS
EFIAPI
XhcCmdTransfer (
  IN  USB_XHCI_INSTANCE     *Xhc,
  IN  TRB_TEMPLATE          *CmdTrb,
  IN  UINTN                 Timeout,
  OUT TRB_TEMPLATE          **EvtTrb
  )
{
  EFI_STATUS      Status;
  URB             *Urb;

  //
  // Validate the parameters
  //
  if ((Xhc == NULL) || (CmdTrb == NULL)) {
    return EFI_INVALID_PARAMETER;
  }

  Status = EFI_DEVICE_ERROR;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    DEBUG ((EFI_D_ERROR, "XhcCmdTransfer: HC is halted\n"));
    goto ON_EXIT;
  }

  //
  // Create a new URB, then poll the execution status.
  //
  Urb = XhcCreateCmdTrb (Xhc, CmdTrb);

  if (Urb == NULL) {
    DEBUG ((EFI_D_ERROR, "XhcCmdTransfer: failed to create URB\n"));
    Status = EFI_OUT_OF_RESOURCES;
    goto ON_EXIT;
  }

  Status  = XhcExecTransfer (Xhc, TRUE, Urb, Timeout);
  *EvtTrb = Urb->EvtTrb;

  if (Urb->Result == EFI_USB_NOERROR) {
    Status = EFI_SUCCESS;
  }

  FreePool (Urb);

ON_EXIT:
  return Status;
}

/**
  Create a new URB for a new transaction.

  @param  Xhc       The XHCI Instance
  @param  BusAddr   The logical device address assigned by UsbBus driver
  @param  EpAddr    Endpoint addrress
  @param  DevSpeed  The device speed
  @param  MaxPacket The max packet length of the endpoint
  @param  Type      The transaction type
  @param  Request   The standard USB request for control transfer
  @param  Data      The user data to transfer
  @param  DataLen   The length of data buffer
  @param  Callback  The function to call when data is transferred
  @param  Context   The context to the callback

  @return Created URB or NULL

**/
URB*
XhcCreateUrb (
  IN USB_XHCI_INSTANCE                  *Xhc,
  IN UINT8                              BusAddr,
  IN UINT8                              EpAddr,
  IN UINT8                              DevSpeed,
  IN UINTN                              MaxPacket,
  IN UINTN                              Type,
  IN EFI_USB_DEVICE_REQUEST             *Request,
  IN VOID                               *Data,
  IN UINTN                              DataLen,
  IN EFI_ASYNC_USB_TRANSFER_CALLBACK    Callback,
  IN VOID                               *Context
  )
{
  USB_ENDPOINT                  *Ep;
  EFI_STATUS                    Status;
  URB                           *Urb;

  Urb = AllocateZeroPool (sizeof (URB));
  if (Urb == NULL) {
    return NULL;
  }

  Urb->Signature = XHC_URB_SIG;
  InitializeListHead (&Urb->UrbList);

  Ep            = &Urb->Ep;
  Ep->BusAddr   = BusAddr;
  Ep->EpAddr    = (UINT8)(EpAddr & 0x0F);
  Ep->Direction = ((EpAddr & 0x80) != 0) ? EfiUsbDataIn : EfiUsbDataOut;
  Ep->DevSpeed  = DevSpeed;
  Ep->MaxPacket = MaxPacket;
  Ep->Type      = Type;

  Urb->Request  = Request;
  Urb->Data     = Data;
  Urb->DataLen  = DataLen;
  Urb->Callback = Callback;
  Urb->Context  = Context;

  Status = XhcCreateTransferTrb (Xhc, Urb);
  ASSERT_EFI_ERROR (Status);

  return Urb;
}

/**
  Create a transfer TRB.

  @param  Xhc     The XHCI Instance
  @param  Urb     The urb used to construct the transfer TRB.

  @return Created TRB or NULL

**/
EFI_STATUS
XhcCreateTransferTrb (
  IN USB_XHCI_INSTANCE          *Xhc,
  IN URB                        *Urb
  )
{
  VOID                          *OutputContext;
  TRANSFER_RING                 *EPRing;
  UINT8                         EPType;
  UINT8                         SlotId;
  UINT8                         Dci;
  TRB                           *TrbStart;
  UINTN                         TotalLen;
  UINTN                         Len;
  UINTN                         TrbNum;

  SlotId = XhcBusDevAddrToSlotId (Xhc, Urb->Ep.BusAddr);
  if (SlotId == 0) {
    return EFI_DEVICE_ERROR;
  }

  Urb->Finished  = FALSE;
  Urb->StartDone = FALSE;
  Urb->EndDone   = FALSE;
  Urb->Completed = 0;
  Urb->Result    = EFI_USB_NOERROR;

  Dci       = XhcEndpointToDci (Urb->Ep.EpAddr, (UINT8)(Urb->Ep.Direction));
  ASSERT (Dci < 32);
  EPRing    = (TRANSFER_RING *)(UINTN) Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1];
  Urb->Ring = EPRing;
  OutputContext = (VOID *)(UINTN)Xhc->DCBAA[SlotId];
  if (Xhc->HcCParams.Data.Csz == 0) {
    EPType  = (UINT8) ((DEVICE_CONTEXT *)OutputContext)->EP[Dci-1].EPType;
  } else {
    EPType  = (UINT8) ((DEVICE_CONTEXT_64 *)OutputContext)->EP[Dci-1].EPType;
  }

  //
  // Construct the TRB
  //
  XhcSyncTrsRing (Xhc, EPRing);
  Urb->TrbStart = EPRing->RingEnqueue;
  switch (EPType) {
    case ED_CONTROL_BIDIR:
      //
      // For control transfer, create SETUP_STAGE_TRB first.
      //
      TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
      TrbStart->TrbCtrSetup.bmRequestType = Urb->Request->RequestType;
      TrbStart->TrbCtrSetup.bRequest      = Urb->Request->Request;
      TrbStart->TrbCtrSetup.wValue        = Urb->Request->Value;
      TrbStart->TrbCtrSetup.wIndex        = Urb->Request->Index;
      TrbStart->TrbCtrSetup.wLength       = Urb->Request->Length;
      TrbStart->TrbCtrSetup.Lenth         = 8;
      TrbStart->TrbCtrSetup.IntTarget     = 0;
      TrbStart->TrbCtrSetup.IOC           = 1;
      TrbStart->TrbCtrSetup.IDT           = 1;
      TrbStart->TrbCtrSetup.Type          = TRB_TYPE_SETUP_STAGE;
      if (Urb->Ep.Direction == EfiUsbDataIn) {
        TrbStart->TrbCtrSetup.TRT = 3;
      } else if (Urb->Ep.Direction == EfiUsbDataOut) {
        TrbStart->TrbCtrSetup.TRT = 2;
      } else {
        TrbStart->TrbCtrSetup.TRT = 0;
      }
      //
      // Update the cycle bit
      //
      TrbStart->TrbCtrSetup.CycleBit = EPRing->RingPCS & BIT0;
      Urb->TrbNum++;

      //
      // For control transfer, create DATA_STAGE_TRB.
      //
      if (Urb->DataLen > 0) {
        XhcSyncTrsRing (Xhc, EPRing);
        TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
        TrbStart->TrbCtrData.TRBPtrLo  = XHC_LOW_32BIT(Urb->Data);
        TrbStart->TrbCtrData.TRBPtrHi  = XHC_HIGH_32BIT(Urb->Data);
        TrbStart->TrbCtrData.Lenth     = (UINT32) Urb->DataLen;
        TrbStart->TrbCtrData.TDSize    = 0;
        TrbStart->TrbCtrData.IntTarget = 0;
        TrbStart->TrbCtrData.ISP       = 1;
        TrbStart->TrbCtrData.IOC       = 1;
        TrbStart->TrbCtrData.IDT       = 0;
        TrbStart->TrbCtrData.CH        = 0;
        TrbStart->TrbCtrData.Type      = TRB_TYPE_DATA_STAGE;
        if (Urb->Ep.Direction == EfiUsbDataIn) {
          TrbStart->TrbCtrData.DIR = 1;
        } else if (Urb->Ep.Direction == EfiUsbDataOut) {
          TrbStart->TrbCtrData.DIR = 0;
        } else {
          TrbStart->TrbCtrData.DIR = 0;
        }
        //
        // Update the cycle bit
        //
        TrbStart->TrbCtrData.CycleBit = EPRing->RingPCS & BIT0;
        Urb->TrbNum++;
      }
      //
      // For control transfer, create STATUS_STAGE_TRB.
      // Get the pointer to next TRB for status stage use
      //
      XhcSyncTrsRing (Xhc, EPRing);
      TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
      TrbStart->TrbCtrStatus.IntTarget = 0;
      TrbStart->TrbCtrStatus.IOC       = 1;
      TrbStart->TrbCtrStatus.CH        = 0;
      TrbStart->TrbCtrStatus.Type      = TRB_TYPE_STATUS_STAGE;
      if (Urb->Ep.Direction == EfiUsbDataIn) {
        TrbStart->TrbCtrStatus.DIR = 0;
      } else if (Urb->Ep.Direction == EfiUsbDataOut) {
        TrbStart->TrbCtrStatus.DIR = 1;
      } else {
        TrbStart->TrbCtrStatus.DIR = 0;
      }
      //
      // Update the cycle bit
      //
      TrbStart->TrbCtrStatus.CycleBit = EPRing->RingPCS & BIT0;
      //
      // Update the enqueue pointer
      //
      XhcSyncTrsRing (Xhc, EPRing);
      Urb->TrbNum++;
      Urb->TrbEnd = (TRB_TEMPLATE *)(UINTN)TrbStart;

      break;

    case ED_BULK_OUT:
    case ED_BULK_IN:
      TotalLen = 0;
      Len      = 0;
      TrbNum   = 0;
      TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
      while (TotalLen < Urb->DataLen) {
        if ((TotalLen + 0x10000) >= Urb->DataLen) {
          Len = Urb->DataLen - TotalLen;
        } else {
          Len = 0x10000;
        }
        TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
        TrbStart->TrbNormal.TRBPtrLo  = XHC_LOW_32BIT((UINT8 *) Urb->Data + TotalLen);
        TrbStart->TrbNormal.TRBPtrHi  = XHC_HIGH_32BIT((UINT8 *) Urb->Data + TotalLen);
        TrbStart->TrbNormal.Lenth     = (UINT32) Len;
        TrbStart->TrbNormal.TDSize    = 0;
        TrbStart->TrbNormal.IntTarget = 0;
        TrbStart->TrbNormal.ISP       = 1;
        TrbStart->TrbNormal.IOC       = 1;
        TrbStart->TrbNormal.Type      = TRB_TYPE_NORMAL;
        //
        // Update the cycle bit
        //
        TrbStart->TrbNormal.CycleBit = EPRing->RingPCS & BIT0;

        XhcSyncTrsRing (Xhc, EPRing);
        TrbNum++;
        TotalLen += Len;
      }

      Urb->TrbNum = TrbNum;
      Urb->TrbEnd = (TRB_TEMPLATE *)(UINTN)TrbStart;
      break;

    case ED_INTERRUPT_OUT:
    case ED_INTERRUPT_IN:
      TotalLen = 0;
      Len      = 0;
      TrbNum   = 0;
      TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
      while (TotalLen < Urb->DataLen) {
        if ((TotalLen + 0x10000) >= Urb->DataLen) {
          Len = Urb->DataLen - TotalLen;
        } else {
          Len = 0x10000;
        }
        TrbStart = (TRB *)(UINTN)EPRing->RingEnqueue;
        TrbStart->TrbNormal.TRBPtrLo  = XHC_LOW_32BIT((UINT8 *) Urb->Data + TotalLen);
        TrbStart->TrbNormal.TRBPtrHi  = XHC_HIGH_32BIT((UINT8 *) Urb->Data + TotalLen);
        TrbStart->TrbNormal.Lenth     = (UINT32) Len;
        TrbStart->TrbNormal.TDSize    = 0;
        TrbStart->TrbNormal.IntTarget = 0;
        TrbStart->TrbNormal.ISP       = 1;
        TrbStart->TrbNormal.IOC       = 1;
        TrbStart->TrbNormal.Type      = TRB_TYPE_NORMAL;
        //
        // Update the cycle bit
        //
        TrbStart->TrbNormal.CycleBit = EPRing->RingPCS & BIT0;

        XhcSyncTrsRing (Xhc, EPRing);
        TrbNum++;
        TotalLen += Len;
      }

      Urb->TrbNum = TrbNum;
      Urb->TrbEnd = (TRB_TEMPLATE *)(UINTN)TrbStart;
      break;

    default:
      DEBUG ((EFI_D_INFO, "Not supported EPType 0x%x!\n",EPType));
      ASSERT (FALSE);
      break;
  }

  return EFI_SUCCESS;
}


/**
  Initialize the XHCI host controller for schedule.

  @param  Xhc        The XHCI Instance to be initialized.

**/
VOID
XhcInitSched (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  VOID                  *Dcbaa;
  UINT64                CmdRing;
  UINTN                 Entries;
  UINT32                MaxScratchpadBufs;
  UINT64                *ScratchBuf;
  UINT64                *ScratchEntryBuf;
  UINT32                Index;

  //
  // Program the Max Device Slots Enabled (MaxSlotsEn) field in the CONFIG register (5.4.7)
  // to enable the device slots that system software is going to use.
  //
  Xhc->MaxSlotsEn = Xhc->HcSParams1.Data.MaxSlots;
  ASSERT (Xhc->MaxSlotsEn >= 1 && Xhc->MaxSlotsEn <= 255);
  XhcWriteOpReg (Xhc, XHC_CONFIG_OFFSET, Xhc->MaxSlotsEn);

  //
  // The Device Context Base Address Array entry associated with each allocated Device Slot
  // shall contain a 64-bit pointer to the base of the associated Device Context.
  // The Device Context Base Address Array shall contain MaxSlotsEn + 1 entries.
  // Software shall set Device Context Base Address Array entries for unallocated Device Slots to '0'.
  //
  Entries = (Xhc->MaxSlotsEn + 1) * sizeof(UINT64);
  Dcbaa   = AllocatePages (EFI_SIZE_TO_PAGES (Entries));
  ASSERT (Dcbaa != NULL);
  ZeroMem (Dcbaa, Entries);

  //
  // A Scratchpad Buffer is a PAGESIZE block of system memory located on a PAGESIZE boundary.
  // System software shall allocate the Scratchpad Buffer(s) before placing the xHC in to Run
  // mode (Run/Stop(R/S) ='1').
  //
  MaxScratchpadBufs      = ((Xhc->HcSParams2.Data.ScratchBufHi) << 5) | (Xhc->HcSParams2.Data.ScratchBufLo);
  Xhc->MaxScratchpadBufs = MaxScratchpadBufs;
  ASSERT (MaxScratchpadBufs <= 1023);
  if (MaxScratchpadBufs != 0) {
    ScratchBuf = AllocateAlignedPages (EFI_SIZE_TO_PAGES (MaxScratchpadBufs * sizeof (UINT64)), Xhc->PageSize);
    ASSERT (ScratchBuf != NULL);
    ZeroMem (ScratchBuf, MaxScratchpadBufs * sizeof (UINT64));
    Xhc->ScratchBuf = ScratchBuf;

    for (Index = 0; Index < MaxScratchpadBufs; Index++) {
      ScratchEntryBuf = AllocateAlignedPages (EFI_SIZE_TO_PAGES (Xhc->PageSize), Xhc->PageSize);
      ASSERT (ScratchEntryBuf != NULL);
      ZeroMem (ScratchEntryBuf, Xhc->PageSize);
      *ScratchBuf++   = (UINT64)(UINTN)ScratchEntryBuf;
    }

    //
    // The Scratchpad Buffer Array contains pointers to the Scratchpad Buffers. Entry 0 of the
    // Device Context Base Address Array points to the Scratchpad Buffer Array.
    //
    *(UINT64 *)Dcbaa = (UINT64)(UINTN)Xhc->ScratchBuf;
  }

  //
  // Program the Device Context Base Address Array Pointer (DCBAAP) register (5.4.6) with
  // a 64-bit address pointing to where the Device Context Base Address Array is located.
  //
  Xhc->DCBAA = (UINT64 *)(UINTN)Dcbaa;
  //
  // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
  // So divide it to two 32-bytes width register access.
  //
  XhcWriteOpReg (Xhc, XHC_DCBAAP_OFFSET, XHC_LOW_32BIT(Xhc->DCBAA));
  XhcWriteOpReg (Xhc, XHC_DCBAAP_OFFSET + 4, XHC_HIGH_32BIT (Xhc->DCBAA));
  DEBUG ((EFI_D_INFO, "XhcInitSched:DCBAA=0x%x\n", (UINT64)(UINTN)Xhc->DCBAA));

  //
  // Define the Command Ring Dequeue Pointer by programming the Command Ring Control Register
  // (5.4.5) with a 64-bit address pointing to the starting address of the first TRB of the Command Ring.
  // Note: The Command Ring is 64 byte aligned, so the low order 6 bits of the Command Ring Pointer shall
  // always be '0'.
  //
  CreateTransferRing (Xhc, CMD_RING_TRB_NUMBER, &Xhc->CmdRing);
  //
  // The xHC uses the Enqueue Pointer to determine when a Transfer Ring is empty. As it fetches TRBs from a
  // Transfer Ring it checks for a Cycle bit transition. If a transition detected, the ring is empty.
  // So we set RCS as inverted PCS init value to let Command Ring empty
  //
  CmdRing  = (UINT64)(UINTN)Xhc->CmdRing.RingSeg0;
  ASSERT ((CmdRing & 0x3F) == 0);
  CmdRing |= XHC_CRCR_RCS;
  //
  // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
  // So divide it to two 32-bytes width register access.
  //
  XhcWriteOpReg (Xhc, XHC_CRCR_OFFSET, XHC_LOW_32BIT(CmdRing));
  XhcWriteOpReg (Xhc, XHC_CRCR_OFFSET + 4, XHC_HIGH_32BIT (CmdRing));

  DEBUG ((EFI_D_INFO, "XhcInitSched:XHC_CRCR=0x%x\n", Xhc->CmdRing.RingSeg0));

  //
  // Disable the 'interrupter enable' bit in USB_CMD
  // and clear IE & IP bit in all Interrupter X Management Registers.
  //
  XhcClearOpRegBit (Xhc, XHC_USBCMD_OFFSET, XHC_USBCMD_INTE);
  for (Index = 0; Index < (UINT16)(Xhc->HcSParams1.Data.MaxIntrs); Index++) {
    XhcClearRuntimeRegBit (Xhc, XHC_IMAN_OFFSET + (Index * 32), XHC_IMAN_IE);
    XhcSetRuntimeRegBit (Xhc, XHC_IMAN_OFFSET + (Index * 32), XHC_IMAN_IP);
  }

  //
  // Allocate EventRing for Cmd, Ctrl, Bulk, Interrupt, AsynInterrupt transfer
  //
  CreateEventRing (Xhc, &Xhc->EventRing);
  DEBUG ((EFI_D_INFO, "XhcInitSched:XHC_EVENTRING=0x%x\n", Xhc->EventRing.EventRingSeg0));
}

/**
  System software shall use a Reset Endpoint Command (section 4.11.4.7) to remove the Halted
  condition in the xHC. After the successful completion of the Reset Endpoint Command, the Endpoint
  Context is transitioned from the Halted to the Stopped state and the Transfer Ring of the endpoint is
  reenabled. The next write to the Doorbell of the Endpoint will transition the Endpoint Context from the
  Stopped to the Running state.

  @param  Xhc                   The XHCI Instance.
  @param  Urb                   The urb which makes the endpoint halted.

  @retval EFI_SUCCESS           The recovery is successful.
  @retval Others                Failed to recovery halted endpoint.

**/
EFI_STATUS
EFIAPI
XhcRecoverHaltedEndpoint (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  URB                 *Urb
  )
{
  EFI_STATUS                  Status;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  CMD_TRB_RESET_ENDPOINT      CmdTrbResetED;
  CMD_SET_TR_DEQ_POINTER      CmdSetTRDeq;
  UINT8                       Dci;
  UINT8                       SlotId;

  Status = EFI_SUCCESS;
  SlotId = XhcBusDevAddrToSlotId (Xhc, Urb->Ep.BusAddr);
  if (SlotId == 0) {
    return EFI_DEVICE_ERROR;
  }
  Dci = XhcEndpointToDci (Urb->Ep.EpAddr, (UINT8)(Urb->Ep.Direction));
  ASSERT (Dci < 32);
  
  DEBUG ((EFI_D_INFO, "Recovery Halted Slot = %x,Dci = %x\n", SlotId, Dci));

  //
  // 1) Send Reset endpoint command to transit from halt to stop state
  //
  ZeroMem (&CmdTrbResetED, sizeof (CmdTrbResetED));
  CmdTrbResetED.CycleBit = 1;
  CmdTrbResetED.Type     = TRB_TYPE_RESET_ENDPOINT;
  CmdTrbResetED.EDID     = Dci;
  CmdTrbResetED.SlotId   = SlotId;
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbResetED,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT (!EFI_ERROR(Status));

  //
  // 2)Set dequeue pointer
  //
  ZeroMem (&CmdSetTRDeq, sizeof (CmdSetTRDeq));
  CmdSetTRDeq.PtrLo    = XHC_LOW_32BIT (Urb->Ring->RingEnqueue) | Urb->Ring->RingPCS;
  CmdSetTRDeq.PtrHi    = XHC_HIGH_32BIT (Urb->Ring->RingEnqueue);
  CmdSetTRDeq.CycleBit = 1;
  CmdSetTRDeq.Type     = TRB_TYPE_SET_TR_DEQUE;
  CmdSetTRDeq.Endpoint = Dci;
  CmdSetTRDeq.SlotId   = SlotId;
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdSetTRDeq,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT (!EFI_ERROR(Status));

  //
  // 3)Ring the doorbell to transit from stop to active
  //
  XhcRingDoorBell (Xhc, SlotId, Dci);

  return Status;
}

/**
  Create XHCI event ring.

  @param  Xhc                 The XHCI Instance.
  @param  EventRing           The created event ring.

**/
VOID
CreateEventRing (
  IN  USB_XHCI_INSTANCE     *Xhc,
  OUT EVENT_RING            *EventRing
  )
{
  VOID                        *Buf;
  EVENT_RING_SEG_TABLE_ENTRY  *ERSTBase;

  ASSERT (EventRing != NULL);

  Buf = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (TRB_TEMPLATE) * EVENT_RING_TRB_NUMBER));
  ASSERT (Buf != NULL);
  ASSERT (((UINTN) Buf & 0x3F) == 0);
  ZeroMem (Buf, sizeof (TRB_TEMPLATE) * EVENT_RING_TRB_NUMBER);

  EventRing->EventRingSeg0    = Buf;
  EventRing->TrbNumber        = EVENT_RING_TRB_NUMBER;
  EventRing->EventRingDequeue = (TRB_TEMPLATE *) EventRing->EventRingSeg0;
  EventRing->EventRingEnqueue = (TRB_TEMPLATE *) EventRing->EventRingSeg0;
  //
  // Software maintains an Event Ring Consumer Cycle State (CCS) bit, initializing it to '1'
  // and toggling it every time the Event Ring Dequeue Pointer wraps back to the beginning of the Event Ring.
  //
  EventRing->EventRingCCS = 1;

  Buf = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (EVENT_RING_SEG_TABLE_ENTRY) * ERST_NUMBER));
  ASSERT (Buf != NULL);
  ASSERT (((UINTN) Buf & 0x3F) == 0);
  ZeroMem (Buf, sizeof (EVENT_RING_SEG_TABLE_ENTRY) * ERST_NUMBER);

  ERSTBase              = (EVENT_RING_SEG_TABLE_ENTRY *) Buf;
  EventRing->ERSTBase   = ERSTBase;
  ERSTBase->PtrLo       = XHC_LOW_32BIT (EventRing->EventRingSeg0);
  ERSTBase->PtrHi       = XHC_HIGH_32BIT (EventRing->EventRingSeg0);
  ERSTBase->RingTrbSize = EVENT_RING_TRB_NUMBER;

  //
  // Program the Interrupter Event Ring Segment Table Size (ERSTSZ) register (5.5.2.3.1)
  //
  XhcWriteRuntimeReg (
    Xhc,
    XHC_ERSTSZ_OFFSET,
    ERST_NUMBER
    );
  //
  // Program the Interrupter Event Ring Dequeue Pointer (ERDP) register (5.5.2.3.3)
  //
  // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
  // So divide it to two 32-bytes width register access.
  //
  XhcWriteRuntimeReg (
    Xhc,
    XHC_ERDP_OFFSET,
    XHC_LOW_32BIT((UINT64)(UINTN)EventRing->EventRingDequeue)
    );
  XhcWriteRuntimeReg (
    Xhc,
    XHC_ERDP_OFFSET + 4,
    XHC_HIGH_32BIT((UINT64)(UINTN)EventRing->EventRingDequeue)
    );
  //
  // Program the Interrupter Event Ring Segment Table Base Address (ERSTBA) register(5.5.2.3.2)
  //
  // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
  // So divide it to two 32-bytes width register access.
  //
  XhcWriteRuntimeReg (
    Xhc,
    XHC_ERSTBA_OFFSET,
    XHC_LOW_32BIT((UINT64)(UINTN)ERSTBase)
    );
  XhcWriteRuntimeReg (
    Xhc,
    XHC_ERSTBA_OFFSET + 4,
    XHC_HIGH_32BIT((UINT64)(UINTN)ERSTBase)
    );
  //
  // Need set IMAN IE bit to enble the ring interrupt
  //
  XhcSetRuntimeRegBit (Xhc, XHC_IMAN_OFFSET, XHC_IMAN_IE);
}

/**
  Create XHCI transfer ring.

  @param  Xhc               The XHCI Instance.
  @param  TrbNum            The number of TRB in the ring.
  @param  TransferRing           The created transfer ring.

**/
VOID
CreateTransferRing (
  IN  USB_XHCI_INSTANCE     *Xhc,
  IN  UINTN                 TrbNum,
  OUT TRANSFER_RING         *TransferRing
  )
{
  VOID                  *Buf;
  LINK_TRB              *EndTrb;

  Buf = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (TRB_TEMPLATE) * TrbNum));
  ASSERT (Buf != NULL);
  ASSERT (((UINTN) Buf & 0x3F) == 0);
  ZeroMem (Buf, sizeof (TRB_TEMPLATE) * TrbNum);

  TransferRing->RingSeg0     = Buf;
  TransferRing->TrbNumber    = TrbNum;
  TransferRing->RingEnqueue  = (TRB_TEMPLATE *) TransferRing->RingSeg0;
  TransferRing->RingDequeue  = (TRB_TEMPLATE *) TransferRing->RingSeg0;
  TransferRing->RingPCS      = 1;
  //
  // 4.9.2 Transfer Ring Management
  // To form a ring (or circular queue) a Link TRB may be inserted at the end of a ring to
  // point to the first TRB in the ring.
  //
  EndTrb        = (LINK_TRB *) ((UINTN)Buf + sizeof (TRB_TEMPLATE) * (TrbNum - 1));
  EndTrb->Type  = TRB_TYPE_LINK;
  EndTrb->PtrLo = XHC_LOW_32BIT (Buf);
  EndTrb->PtrHi = XHC_HIGH_32BIT (Buf);
  //
  // Toggle Cycle (TC). When set to '1', the xHC shall toggle its interpretation of the Cycle bit.
  //
  EndTrb->TC    = 1;
  //
  // Set Cycle bit as other TRB PCS init value
  //
  EndTrb->CycleBit = 0;
}

/**
  Free XHCI event ring.

  @param  Xhc                 The XHCI Instance.
  @param  EventRing           The event ring to be freed.

**/
EFI_STATUS
EFIAPI
XhcFreeEventRing (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  EVENT_RING          *EventRing
)
{
  UINT8                         Index;
  EVENT_RING_SEG_TABLE_ENTRY    *TablePtr;
  VOID                          *RingBuf;
  EVENT_RING_SEG_TABLE_ENTRY    *EventRingPtr;

  if(EventRing->EventRingSeg0 == NULL) {
    return EFI_SUCCESS;
  }

  //
  // Get the Event Ring Segment Table base address
  //
  TablePtr = (EVENT_RING_SEG_TABLE_ENTRY *)(EventRing->ERSTBase);

  //
  // Get all the TRBs Ring and release
  //
  for (Index = 0; Index < ERST_NUMBER; Index++) {
    EventRingPtr = TablePtr + Index;
    RingBuf      = (VOID *)(UINTN)(EventRingPtr->PtrLo | LShiftU64 ((UINT64)EventRingPtr->PtrHi, 32));

    if(RingBuf != NULL) {
      FreePages (RingBuf, EFI_SIZE_TO_PAGES (sizeof (TRB_TEMPLATE) * EVENT_RING_TRB_NUMBER));
      ZeroMem (EventRingPtr, sizeof (EVENT_RING_SEG_TABLE_ENTRY));
    }
  }

  FreePages (TablePtr, EFI_SIZE_TO_PAGES (sizeof (EVENT_RING_SEG_TABLE_ENTRY) * ERST_NUMBER));
  return EFI_SUCCESS;
}

/**
  Free the resouce allocated at initializing schedule.

  @param  Xhc        The XHCI Instance.

**/
VOID
XhcFreeSched (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  UINT32    Index;
  UINT64    *ScratchBuf;

  if (Xhc->ScratchBuf != NULL) {
    ScratchBuf = Xhc->ScratchBuf;
    for (Index = 0; Index < Xhc->MaxScratchpadBufs; Index++) {
      FreeAlignedPages ((VOID*)(UINTN)*ScratchBuf++, EFI_SIZE_TO_PAGES (Xhc->PageSize));
    }
    FreeAlignedPages (Xhc->ScratchBuf, EFI_SIZE_TO_PAGES (Xhc->MaxScratchpadBufs * sizeof (UINT64)));
  }

  if (Xhc->DCBAA != NULL) {
    FreePages (Xhc->DCBAA, EFI_SIZE_TO_PAGES((Xhc->MaxSlotsEn + 1) * sizeof(UINT64)));
    Xhc->DCBAA = NULL;
  }

  if (Xhc->CmdRing.RingSeg0 != NULL){
    FreePages (Xhc->CmdRing.RingSeg0, EFI_SIZE_TO_PAGES (sizeof (TRB_TEMPLATE) * CMD_RING_TRB_NUMBER));
    Xhc->CmdRing.RingSeg0 = NULL;
  }

  XhcFreeEventRing (Xhc,&Xhc->EventRing);
}

/**
  Check if the Trb is a transaction of the URBs in XHCI's asynchronous transfer list.

  @param Xhc    The XHCI Instance.
  @param Trb    The TRB to be checked.
  @param Urb    The pointer to the matched Urb.

  @retval TRUE  The Trb is matched with a transaction of the URBs in the async list.
  @retval FALSE The Trb is not matched with any URBs in the async list.

**/
BOOLEAN
IsAsyncIntTrb (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  TRB_TEMPLATE        *Trb,
  OUT URB                 **Urb
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  TRB_TEMPLATE            *CheckedTrb;
  URB                     *CheckedUrb;
  UINTN                   Index;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Xhc->AsyncIntTransfers) {
    CheckedUrb = EFI_LIST_CONTAINER (Entry, URB, UrbList);
    CheckedTrb = CheckedUrb->TrbStart;
    for (Index = 0; Index < CheckedUrb->TrbNum; Index++) {
      if (Trb == CheckedTrb) {
        *Urb = CheckedUrb;
        return TRUE;
      }
      CheckedTrb++;
      if ((UINTN)CheckedTrb >= ((UINTN) CheckedUrb->Ring->RingSeg0 + sizeof (TRB_TEMPLATE) * CheckedUrb->Ring->TrbNumber)) {
        CheckedTrb = (TRB_TEMPLATE*) CheckedUrb->Ring->RingSeg0;
      }
    }
  }

  return FALSE;
}

/**
  Check if the Trb is a transaction of the URB.

  @param Trb    The TRB to be checked
  @param Urb    The transfer ring to be checked.

  @retval TRUE  It is a transaction of the URB.
  @retval FALSE It is not any transaction of the URB.

**/
BOOLEAN
IsTransferRingTrb (
  IN  TRB_TEMPLATE        *Trb,
  IN  URB                 *Urb
  )
{
  TRB_TEMPLATE  *CheckedTrb;
  UINTN         Index;

  CheckedTrb = Urb->Ring->RingSeg0;

  ASSERT (Urb->Ring->TrbNumber == CMD_RING_TRB_NUMBER || Urb->Ring->TrbNumber == TR_RING_TRB_NUMBER);

  for (Index = 0; Index < Urb->Ring->TrbNumber; Index++) {
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

  @param  Xhc             The XHCI Instance.
  @param  Urb             The URB to check result.

  @return Whether the result of URB transfer is finialized.

**/
EFI_STATUS
XhcCheckUrbResult (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  URB                 *Urb
  )
{
  EVT_TRB_TRANSFER        *EvtTrb;
  TRB_TEMPLATE            *TRBPtr;
  UINTN                   Index;
  UINT8                   TRBType;
  EFI_STATUS              Status;
  URB                     *AsyncUrb;
  URB                     *CheckedUrb;
  UINT64                  XhcDequeue;
  UINT32                  High;
  UINT32                  Low;

  ASSERT ((Xhc != NULL) && (Urb != NULL));

  Status   = EFI_SUCCESS;
  AsyncUrb = NULL;

  if (Urb->Finished) {
    goto EXIT;
  }

  EvtTrb = NULL;

  if (XhcIsHalt (Xhc) || XhcIsSysError (Xhc)) {
    Urb->Result |= EFI_USB_ERR_SYSTEM;
    Status       = EFI_DEVICE_ERROR;
    goto EXIT;
  }

  //
  // Traverse the event ring to find out all new events from the previous check.
  //
  XhcSyncEventRing (Xhc, &Xhc->EventRing);
  for (Index = 0; Index < Xhc->EventRing.TrbNumber; Index++) {
    Status = XhcCheckNewEvent (Xhc, &Xhc->EventRing, ((TRB_TEMPLATE **)&EvtTrb));
    if (Status == EFI_NOT_READY) {
      //
      // All new events are handled, return directly.
      //
      goto EXIT;
    }

    //
    // Only handle COMMAND_COMPLETETION_EVENT and TRANSFER_EVENT.
    //
    if ((EvtTrb->Type != TRB_TYPE_COMMAND_COMPLT_EVENT) && (EvtTrb->Type != TRB_TYPE_TRANS_EVENT)) {
      continue;
    }

    TRBPtr = (TRB_TEMPLATE *)(UINTN)(EvtTrb->TRBPtrLo | LShiftU64 ((UINT64) EvtTrb->TRBPtrHi, 32));

    //
    // Update the status of Urb according to the finished event regardless of whether
    // the urb is current checked one or in the XHCI's async transfer list.
    // This way is used to avoid that those completed async transfer events don't get
    // handled in time and are flushed by newer coming events.
    //
    if (IsTransferRingTrb (TRBPtr, Urb)) {
      CheckedUrb = Urb;
    } else if (IsAsyncIntTrb (Xhc, TRBPtr, &AsyncUrb)) {    
      CheckedUrb = AsyncUrb;
    } else {
      continue;
    }
  
    switch (EvtTrb->Completecode) {
      case TRB_COMPLETION_STALL_ERROR:
        CheckedUrb->Result  |= EFI_USB_ERR_STALL;
        CheckedUrb->Finished = TRUE;
        DEBUG ((EFI_D_ERROR, "XhcCheckUrbResult: STALL_ERROR! Completecode = %x\n",EvtTrb->Completecode));
        break;

      case TRB_COMPLETION_BABBLE_ERROR:
        CheckedUrb->Result  |= EFI_USB_ERR_BABBLE;
        CheckedUrb->Finished = TRUE;
        DEBUG ((EFI_D_ERROR, "XhcCheckUrbResult: BABBLE_ERROR! Completecode = %x\n",EvtTrb->Completecode));
        break;

      case TRB_COMPLETION_DATA_BUFFER_ERROR:
        CheckedUrb->Result  |= EFI_USB_ERR_BUFFER;
        CheckedUrb->Finished = TRUE;
        DEBUG ((EFI_D_ERROR, "XhcCheckUrbResult: ERR_BUFFER! Completecode = %x\n",EvtTrb->Completecode));
        break;

      case TRB_COMPLETION_USB_TRANSACTION_ERROR:
        CheckedUrb->Result  |= EFI_USB_ERR_TIMEOUT;
        CheckedUrb->Finished = TRUE;
        DEBUG ((EFI_D_ERROR, "XhcCheckUrbResult: TRANSACTION_ERROR! Completecode = %x\n",EvtTrb->Completecode));
        break;

      case TRB_COMPLETION_SHORT_PACKET:
      case TRB_COMPLETION_SUCCESS:
        if (EvtTrb->Completecode == TRB_COMPLETION_SHORT_PACKET) {
          DEBUG ((EFI_D_ERROR, "XhcCheckUrbResult: short packet happens!\n"));
        }

        TRBType = (UINT8) (TRBPtr->Type);
        if ((TRBType == TRB_TYPE_DATA_STAGE) ||
            (TRBType == TRB_TYPE_NORMAL) ||
            (TRBType == TRB_TYPE_ISOCH)) {
          CheckedUrb->Completed += (CheckedUrb->DataLen - EvtTrb->Lenth);
        }

        break;

      default:
        DEBUG ((EFI_D_ERROR, "Transfer Default Error Occur! Completecode = 0x%x!\n",EvtTrb->Completecode));
        CheckedUrb->Result  |= EFI_USB_ERR_TIMEOUT;
        CheckedUrb->Finished = TRUE;
        break;
    }

    //
    // Only check first and end Trb event address
    //
    if (TRBPtr == CheckedUrb->TrbStart) {
      CheckedUrb->StartDone = TRUE;
    }

    if (TRBPtr == CheckedUrb->TrbEnd) {
      CheckedUrb->EndDone = TRUE;
    }

    if (CheckedUrb->StartDone && CheckedUrb->EndDone) {
      CheckedUrb->Finished = TRUE;
      CheckedUrb->EvtTrb   = (TRB_TEMPLATE *)EvtTrb;
    }
  }

EXIT:

  //
  // Advance event ring to last available entry
  //
  // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
  // So divide it to two 32-bytes width register access.
  //
  Low  = XhcReadRuntimeReg (Xhc, XHC_ERDP_OFFSET);
  High = XhcReadRuntimeReg (Xhc, XHC_ERDP_OFFSET + 4);
  XhcDequeue = (UINT64)(LShiftU64((UINT64)High, 32) | Low);

  if ((XhcDequeue & (~0x0F)) != ((UINT64)(UINTN)Xhc->EventRing.EventRingDequeue & (~0x0F))) {
    //
    // Some 3rd party XHCI external cards don't support single 64-bytes width register access,
    // So divide it to two 32-bytes width register access.
    //
    XhcWriteRuntimeReg (Xhc, XHC_ERDP_OFFSET, XHC_LOW_32BIT (Xhc->EventRing.EventRingDequeue) | BIT3);
    XhcWriteRuntimeReg (Xhc, XHC_ERDP_OFFSET + 4, XHC_HIGH_32BIT (Xhc->EventRing.EventRingDequeue));
  }

  return Status;
}


/**
  Execute the transfer by polling the URB. This is a synchronous operation.

  @param  Xhc               The XHCI Instance.
  @param  CmdTransfer       The executed URB is for cmd transfer or not.
  @param  Urb               The URB to execute.
  @param  Timeout           The time to wait before abort, in millisecond.

  @return EFI_DEVICE_ERROR  The transfer failed due to transfer error.
  @return EFI_TIMEOUT       The transfer failed due to time out.
  @return EFI_SUCCESS       The transfer finished OK.

**/
EFI_STATUS
XhcExecTransfer (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  BOOLEAN             CmdTransfer,
  IN  URB                 *Urb,
  IN  UINTN               Timeout
  )
{
  EFI_STATUS              Status;
  UINTN                   Index;
  UINTN                   Loop;
  UINT8                   SlotId;
  UINT8                   Dci;

  if (CmdTransfer) {
    SlotId = 0;
    Dci    = 0;
  } else {
    SlotId = XhcBusDevAddrToSlotId (Xhc, Urb->Ep.BusAddr);
    if (SlotId == 0) {
      return EFI_DEVICE_ERROR;
    }
    Dci  = XhcEndpointToDci (Urb->Ep.EpAddr, (UINT8)(Urb->Ep.Direction));
    ASSERT (Dci < 32);
  }

  Status = EFI_SUCCESS;
  Loop   = (Timeout * XHC_1_MILLISECOND / XHC_POLL_DELAY) + 1;
  if (Timeout == 0) {
    Loop = 0xFFFFFFFF;
  }

  XhcRingDoorBell (Xhc, SlotId, Dci);

  for (Index = 0; Index < Loop; Index++) {
    Status = XhcCheckUrbResult (Xhc, Urb);
    if (Urb->Finished) {
      break;
    }
    gBS->Stall (XHC_POLL_DELAY);
  }

  if (Index == Loop) {
    Urb->Result = EFI_USB_ERR_TIMEOUT;
  }

  return Status;
}

/**
  Delete a single asynchronous interrupt transfer for
  the device and endpoint.

  @param  Xhc                   The XHCI Instance.
  @param  BusAddr               The logical device address assigned by UsbBus driver.
  @param  EpNum                 The endpoint of the target.

  @retval EFI_SUCCESS           An asynchronous transfer is removed.
  @retval EFI_NOT_FOUND         No transfer for the device is found.

**/
EFI_STATUS
XhciDelAsyncIntTransfer (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  UINT8               BusAddr,
  IN  UINT8               EpNum
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  URB                     *Urb;
  EFI_USB_DATA_DIRECTION  Direction;

  Direction = ((EpNum & 0x80) != 0) ? EfiUsbDataIn : EfiUsbDataOut;
  EpNum    &= 0x0F;

  Urb = NULL;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Xhc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);
    if ((Urb->Ep.BusAddr == BusAddr) &&
        (Urb->Ep.EpAddr == EpNum) &&
        (Urb->Ep.Direction == Direction)) {
      RemoveEntryList (&Urb->UrbList);
      FreePool (Urb->Data);
      FreePool (Urb);
      return EFI_SUCCESS;
    }
  }

  return EFI_NOT_FOUND;
}

/**
  Remove all the asynchronous interrutp transfers.

  @param  Xhc    The XHCI Instance.

**/
VOID
XhciDelAllAsyncIntTransfers (
  IN USB_XHCI_INSTANCE    *Xhc
  )
{
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  URB                     *Urb;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Xhc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);
    RemoveEntryList (&Urb->UrbList);
    FreePool (Urb->Data);
    FreePool (Urb);
  }
}

/**
  Update the queue head for next round of asynchronous transfer

  @param  Xhc     The XHCI Instance.
  @param  Urb     The URB to update

**/
VOID
XhcUpdateAsyncRequest (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN URB                      *Urb
  )
{
  EFI_STATUS    Status;

  if (Urb->Result == EFI_USB_NOERROR) {
    Status = XhcCreateTransferTrb (Xhc, Urb);
    if (EFI_ERROR (Status)) {
      return;
    }
    Status = RingIntTransferDoorBell (Xhc, Urb);
    if (EFI_ERROR (Status)) {
      return;
    }
  }
}


/**
  Interrupt transfer periodic check handler.

  @param  Event                 Interrupt event.
  @param  Context               Pointer to USB_XHCI_INSTANCE.

**/
VOID
EFIAPI
XhcMonitorAsyncRequests (
  IN EFI_EVENT            Event,
  IN VOID                 *Context
  )
{
  USB_XHCI_INSTANCE       *Xhc;
  LIST_ENTRY              *Entry;
  LIST_ENTRY              *Next;
  UINT8                   *ProcBuf;
  URB                     *Urb;
  UINT8                   SlotId;
  EFI_STATUS              Status;
  EFI_TPL                 OldTpl;

  OldTpl = gBS->RaiseTPL (XHC_TPL);

  Xhc    = (USB_XHCI_INSTANCE*) Context;

  EFI_LIST_FOR_EACH_SAFE (Entry, Next, &Xhc->AsyncIntTransfers) {
    Urb = EFI_LIST_CONTAINER (Entry, URB, UrbList);

    //
    // Make sure that the device is available before every check.
    //
    SlotId = XhcBusDevAddrToSlotId (Xhc, Urb->Ep.BusAddr);
    if (SlotId == 0) {
      continue;
    }

    //
    // Check the result of URB execution. If it is still
    // active, check the next one.
    //
    Status = XhcCheckUrbResult (Xhc, Urb);

    if (!Urb->Finished) {
      continue;
    }

    //
    // Allocate a buffer then copy the transferred data for user.
    // If failed to allocate the buffer, update the URB for next
    // round of transfer. Ignore the data of this round.
    //
    ProcBuf = NULL;
    if (Urb->Result == EFI_USB_NOERROR) {
      ASSERT (Urb->Completed <= Urb->DataLen);

      ProcBuf = AllocateZeroPool (Urb->Completed);

      if (ProcBuf == NULL) {
        XhcUpdateAsyncRequest (Xhc, Urb);
        continue;
      }

      CopyMem (ProcBuf, Urb->Data, Urb->Completed);
    }

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
      OldTpl = gBS->RaiseTPL (XHC_TPL);
    }

    if (ProcBuf != NULL) {
      gBS->FreePool (ProcBuf);
    }

    XhcUpdateAsyncRequest (Xhc, Urb);
  }
  gBS->RestoreTPL (OldTpl);
}

/**
  Monitor the port status change. Enable/Disable device slot if there is a device attached/detached.

  @param  Xhc                   The XHCI Instance.
  @param  ParentRouteChart      The route string pointed to the parent device if it exists.
  @param  Port                  The port to be polled.
  @param  PortState             The port state.

  @retval EFI_SUCCESS           Successfully enable/disable device slot according to port state.
  @retval Others                Should not appear.

**/
EFI_STATUS
EFIAPI
XhcPollPortStatusChange (
  IN  USB_XHCI_INSTANCE     *Xhc,
  IN  USB_DEV_ROUTE         ParentRouteChart,
  IN  UINT8                 Port,
  IN  EFI_USB_PORT_STATUS   *PortState
  )
{
  EFI_STATUS        Status;
  UINT8             Speed;
  UINT8             SlotId;
  USB_DEV_ROUTE     RouteChart;

  Status = EFI_SUCCESS;

  if (ParentRouteChart.Dword == 0) {
    RouteChart.Route.RouteString = 0;
    RouteChart.Route.RootPortNum = Port + 1;
    RouteChart.Route.TierNum     = 1;
  } else {
    if(Port < 14) {
      RouteChart.Route.RouteString = ParentRouteChart.Route.RouteString | (Port << (4 * (ParentRouteChart.Route.TierNum - 1)));
    } else {
      RouteChart.Route.RouteString = ParentRouteChart.Route.RouteString | (15 << (4 * (ParentRouteChart.Route.TierNum - 1)));
    }
    RouteChart.Route.RootPortNum   = ParentRouteChart.Route.RootPortNum;
    RouteChart.Route.TierNum       = ParentRouteChart.Route.TierNum + 1;
  }

  if (((PortState->PortStatus & USB_PORT_STAT_ENABLE) != 0) &&
      ((PortState->PortStatus & USB_PORT_STAT_CONNECTION) != 0)) {
    //
    // Has a device attached, Identify device speed after port is enabled.
    //
    Speed = EFI_USB_SPEED_FULL;
    if ((PortState->PortStatus & USB_PORT_STAT_LOW_SPEED) != 0) {
      Speed = EFI_USB_SPEED_LOW;
    } else if ((PortState->PortStatus & USB_PORT_STAT_HIGH_SPEED) != 0) {
      Speed = EFI_USB_SPEED_HIGH;
    } else if ((PortState->PortStatus & USB_PORT_STAT_SUPER_SPEED) != 0) {
      Speed = EFI_USB_SPEED_SUPER;
    }
    //
    // Execute Enable_Slot cmd for attached device, initialize device context and assign device address.
    //
    SlotId = XhcRouteStringToSlotId (Xhc, RouteChart);
    if (SlotId == 0) {
      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcInitializeDeviceSlot (Xhc, ParentRouteChart, Port, RouteChart, Speed);
      } else {
        Status = XhcInitializeDeviceSlot64 (Xhc, ParentRouteChart, Port, RouteChart, Speed);
      }
      ASSERT_EFI_ERROR (Status);
    }
  } else if ((PortState->PortStatus & USB_PORT_STAT_CONNECTION) == 0) {
    //
    // Device is detached. Disable the allocated device slot and release resource.
    //
    SlotId = XhcRouteStringToSlotId (Xhc, RouteChart);
    if (SlotId != 0) {
      if (Xhc->HcCParams.Data.Csz == 0) {
        Status = XhcDisableSlotCmd (Xhc, SlotId);
      } else {
        Status = XhcDisableSlotCmd64 (Xhc, SlotId);
      }
      ASSERT_EFI_ERROR (Status);
    }
  }
  return Status;
}


/**
  Calculate the device context index by endpoint address and direction.

  @param  EpAddr              The target endpoint number.
  @param  Direction           The direction of the target endpoint.

  @return The device context index of endpoint.

**/
UINT8
XhcEndpointToDci (
  IN  UINT8                   EpAddr,
  IN  UINT8                   Direction
  )
{
  UINT8 Index;

  if (EpAddr == 0) {
    return 1;
  } else {
    Index = (UINT8) (2 * EpAddr);
    if (Direction == EfiUsbDataIn) {
      Index += 1;
    }
    return Index;
  }
}

/**
  Find out the actual device address according to the requested device address from UsbBus.

  @param  Xhc             The XHCI Instance.
  @param  BusDevAddr      The requested device address by UsbBus upper driver.

  @return The actual device address assigned to the device.

**/
UINT8
EFIAPI
XhcBusDevAddrToSlotId (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  UINT8              BusDevAddr
  )
{
  UINT8  Index;

  for (Index = 0; Index < 255; Index++) {
    if (Xhc->UsbDevContext[Index + 1].Enabled &&
        (Xhc->UsbDevContext[Index + 1].SlotId != 0) &&
        (Xhc->UsbDevContext[Index + 1].BusDevAddr == BusDevAddr)) {
      break;
    }
  }

  if (Index == 255) {
    return 0;
  }

  return Xhc->UsbDevContext[Index + 1].SlotId;
}

/**
  Find out the slot id according to the device's route string.

  @param  Xhc             The XHCI Instance.
  @param  RouteString     The route string described the device location.

  @return The slot id used by the device.

**/
UINT8
EFIAPI
XhcRouteStringToSlotId (
  IN  USB_XHCI_INSTANCE  *Xhc,
  IN  USB_DEV_ROUTE      RouteString
  )
{
  UINT8  Index;

  for (Index = 0; Index < 255; Index++) {
    if (Xhc->UsbDevContext[Index + 1].Enabled &&
        (Xhc->UsbDevContext[Index + 1].SlotId != 0) &&
        (Xhc->UsbDevContext[Index + 1].RouteString.Dword == RouteString.Dword)) {
      break;
    }
  }

  if (Index == 255) {
    return 0;
  }

  return Xhc->UsbDevContext[Index + 1].SlotId;
}

/**
  Synchronize the specified event ring to update the enqueue and dequeue pointer.

  @param  Xhc         The XHCI Instance.
  @param  EvtRing     The event ring to sync.

  @retval EFI_SUCCESS The event ring is synchronized successfully.

**/
EFI_STATUS
EFIAPI
XhcSyncEventRing (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN EVENT_RING           *EvtRing
  )
{
  UINTN               Index;
  TRB_TEMPLATE        *EvtTrb1;

  ASSERT (EvtRing != NULL);

  //
  // Calculate the EventRingEnqueue and EventRingCCS.
  // Note: only support single Segment
  //
  EvtTrb1 = EvtRing->EventRingDequeue;

  for (Index = 0; Index < EvtRing->TrbNumber; Index++) {
    if (EvtTrb1->CycleBit != EvtRing->EventRingCCS) {
      break;
    }

    EvtTrb1++;

    if ((UINTN)EvtTrb1 >= ((UINTN) EvtRing->EventRingSeg0 + sizeof (TRB_TEMPLATE) * EvtRing->TrbNumber)) {
      EvtTrb1 = EvtRing->EventRingSeg0;
      EvtRing->EventRingCCS = (EvtRing->EventRingCCS) ? 0 : 1;
    }
  }

  if (Index < EvtRing->TrbNumber) {
    EvtRing->EventRingEnqueue = EvtTrb1;
  } else {
    ASSERT (FALSE);
  }

  return EFI_SUCCESS;
}

/**
  Synchronize the specified transfer ring to update the enqueue and dequeue pointer.

  @param  Xhc         The XHCI Instance.
  @param  TrsRing     The transfer ring to sync.

  @retval EFI_SUCCESS The transfer ring is synchronized successfully.

**/
EFI_STATUS
EFIAPI
XhcSyncTrsRing (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN TRANSFER_RING        *TrsRing
  )
{
  UINTN               Index;
  TRB_TEMPLATE        *TrsTrb;

  ASSERT (TrsRing != NULL);
  //
  // Calculate the latest RingEnqueue and RingPCS
  //
  TrsTrb = TrsRing->RingEnqueue;
  ASSERT (TrsTrb != NULL);

  for (Index = 0; Index < TrsRing->TrbNumber; Index++) {
    if (TrsTrb->CycleBit != (TrsRing->RingPCS & BIT0)) {
      break;
    }
    TrsTrb++;
    if ((UINT8) TrsTrb->Type == TRB_TYPE_LINK) {
      ASSERT (((LINK_TRB*)TrsTrb)->TC != 0);
      //
      // set cycle bit in Link TRB as normal
      //
      ((LINK_TRB*)TrsTrb)->CycleBit = TrsRing->RingPCS & BIT0;
      //
      // Toggle PCS maintained by software
      //
      TrsRing->RingPCS = (TrsRing->RingPCS & BIT0) ? 0 : 1;
      TrsTrb           = (TRB_TEMPLATE *)(UINTN)((TrsTrb->Parameter1 | LShiftU64 ((UINT64)TrsTrb->Parameter2, 32)) & ~0x0F);
    }
  }

  ASSERT (Index != TrsRing->TrbNumber);

  if (TrsTrb != TrsRing->RingEnqueue) {
    TrsRing->RingEnqueue = TrsTrb;
  }

  //
  // Clear the Trb context for enqueue, but reserve the PCS bit
  //
  TrsTrb->Parameter1 = 0;
  TrsTrb->Parameter2 = 0;
  TrsTrb->Status     = 0;
  TrsTrb->RsvdZ1     = 0;
  TrsTrb->Type       = 0;
  TrsTrb->Control    = 0;

  return EFI_SUCCESS;
}

/**
  Check if there is a new generated event.

  @param  Xhc           The XHCI Instance.
  @param  EvtRing       The event ring to check.
  @param  NewEvtTrb     The new event TRB found.

  @retval EFI_SUCCESS   Found a new event TRB at the event ring.
  @retval EFI_NOT_READY The event ring has no new event.

**/
EFI_STATUS
EFIAPI
XhcCheckNewEvent (
  IN  USB_XHCI_INSTANCE       *Xhc,
  IN  EVENT_RING              *EvtRing,
  OUT TRB_TEMPLATE            **NewEvtTrb
  )
{
  EFI_STATUS          Status;
  TRB_TEMPLATE        *EvtTrb;

  ASSERT (EvtRing != NULL);

  EvtTrb     = EvtRing->EventRingDequeue;
  *NewEvtTrb = EvtRing->EventRingDequeue;

  if (EvtRing->EventRingDequeue == EvtRing->EventRingEnqueue) {
    return EFI_NOT_READY;
  }

  Status = EFI_SUCCESS;

  EvtRing->EventRingDequeue++;
  //
  // If the dequeue pointer is beyond the ring, then roll-back it to the begining of the ring.
  //
  if ((UINTN)EvtRing->EventRingDequeue >= ((UINTN) EvtRing->EventRingSeg0 + sizeof (TRB_TEMPLATE) * EvtRing->TrbNumber)) {
    EvtRing->EventRingDequeue = EvtRing->EventRingSeg0;
  }

  return Status;
}

/**
  Ring the door bell to notify XHCI there is a transaction to be executed.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id of the target device.
  @param  Dci           The device context index of the target slot or endpoint.

  @retval EFI_SUCCESS   Successfully ring the door bell.

**/
EFI_STATUS
EFIAPI
XhcRingDoorBell (
  IN USB_XHCI_INSTANCE    *Xhc,
  IN UINT8                SlotId,
  IN UINT8                Dci
  )
{
  if (SlotId == 0) {
    XhcWriteDoorBellReg (Xhc, 0, 0);
  } else {
    XhcWriteDoorBellReg (Xhc, SlotId * sizeof (UINT32), Dci);
  }

  return EFI_SUCCESS;
}

/**
  Ring the door bell to notify XHCI there is a transaction to be executed through URB.

  @param  Xhc           The XHCI Instance.
  @param  Urb           The URB to be rung.

  @retval EFI_SUCCESS   Successfully ring the door bell.

**/
EFI_STATUS
RingIntTransferDoorBell (
  IN  USB_XHCI_INSTANCE   *Xhc,
  IN  URB                 *Urb
  )
{
  UINT8                SlotId;
  UINT8                Dci;

  SlotId = XhcBusDevAddrToSlotId (Xhc, Urb->Ep.BusAddr);
  Dci    = XhcEndpointToDci (Urb->Ep.EpAddr, (UINT8)(Urb->Ep.Direction));
  XhcRingDoorBell (Xhc, SlotId, Dci);
  return EFI_SUCCESS;
}

/**
  Assign and initialize the device slot for a new device.

  @param  Xhc                 The XHCI Instance.
  @param  ParentRouteChart    The route string pointed to the parent device.
  @param  ParentPort          The port at which the device is located.
  @param  RouteChart          The route string pointed to the device.
  @param  DeviceSpeed         The device speed.

  @retval EFI_SUCCESS   Successfully assign a slot to the device and assign an address to it.

**/
EFI_STATUS
EFIAPI
XhcInitializeDeviceSlot (
  IN  USB_XHCI_INSTANCE         *Xhc,
  IN  USB_DEV_ROUTE             ParentRouteChart,
  IN  UINT16                    ParentPort,
  IN  USB_DEV_ROUTE             RouteChart,
  IN  UINT8                     DeviceSpeed
  )
{
  EFI_STATUS                  Status;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  INPUT_CONTEXT               *InputContext;
  DEVICE_CONTEXT              *OutputContext;
  TRANSFER_RING               *EndpointTransferRing;
  CMD_TRB_ADDRESS_DEVICE      CmdTrbAddr;
  UINT8                       DeviceAddress;
  CMD_TRB_ENABLE_SLOT         CmdTrb;
  UINT8                       SlotId;
  UINT8                       ParentSlotId;
  DEVICE_CONTEXT              *ParentDeviceContext;

  ZeroMem (&CmdTrb, sizeof (CMD_TRB_ENABLE_SLOT));
  CmdTrb.CycleBit = 1;
  CmdTrb.Type     = TRB_TYPE_EN_SLOT;

  Status = XhcCmdTransfer (
              Xhc,
              (TRB_TEMPLATE *) (UINTN) &CmdTrb,
              XHC_GENERIC_TIMEOUT,
              (TRB_TEMPLATE **) (UINTN) &EvtTrb
              );
  ASSERT_EFI_ERROR (Status);
  ASSERT (EvtTrb->SlotId <= Xhc->MaxSlotsEn);
  DEBUG ((EFI_D_INFO, "Enable Slot Successfully, The Slot ID = 0x%x\n", EvtTrb->SlotId));
  SlotId = (UINT8)EvtTrb->SlotId;
  ASSERT (SlotId != 0);

  ZeroMem (&Xhc->UsbDevContext[SlotId], sizeof (USB_DEV_CONTEXT));
  Xhc->UsbDevContext[SlotId].Enabled                 = TRUE;
  Xhc->UsbDevContext[SlotId].SlotId                  = SlotId;
  Xhc->UsbDevContext[SlotId].RouteString.Dword       = RouteChart.Dword;
  Xhc->UsbDevContext[SlotId].ParentRouteString.Dword = ParentRouteChart.Dword;

  //
  // 4.3.3 Device Slot Initialization
  // 1) Allocate an Input Context data structure (6.2.5) and initialize all fields to '0'.
  //
  InputContext = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (INPUT_CONTEXT)));
  ASSERT (InputContext != NULL);
  ASSERT (((UINTN) InputContext & 0x3F) == 0);
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT));

  Xhc->UsbDevContext[SlotId].InputContext = (VOID *) InputContext;

  //
  // 2) Initialize the Input Control Context (6.2.5.1) of the Input Context by setting the A0 and A1
  //    flags to '1'. These flags indicate that the Slot Context and the Endpoint 0 Context of the Input
  //    Context are affected by the command.
  //
  InputContext->InputControlContext.Dword2 |= (BIT0 | BIT1);

  //
  // 3) Initialize the Input Slot Context data structure
  //
  InputContext->Slot.RouteString    = RouteChart.Route.RouteString;
  InputContext->Slot.Speed          = DeviceSpeed + 1;
  InputContext->Slot.ContextEntries = 1;
  InputContext->Slot.RootHubPortNum = RouteChart.Route.RootPortNum;

  if (RouteChart.Route.RouteString) {
    //
    // The device is behind of hub device.
    //
    ParentSlotId = XhcRouteStringToSlotId(Xhc, ParentRouteChart);
    ASSERT (ParentSlotId != 0);
    //
    //if the Full/Low device attached to a High Speed Hub, Init the TTPortNum and TTHubSlotId field of slot context
    //
    ParentDeviceContext = (DEVICE_CONTEXT *)Xhc->UsbDevContext[ParentSlotId].OutputContext;
    if ((ParentDeviceContext->Slot.TTPortNum == 0) &&
        (ParentDeviceContext->Slot.TTHubSlotId == 0)) {
      if ((ParentDeviceContext->Slot.Speed == (EFI_USB_SPEED_HIGH + 1)) && (DeviceSpeed < EFI_USB_SPEED_HIGH)) {
        //
        // Full/Low device attached to High speed hub port that isolates the high speed signaling
        // environment from Full/Low speed signaling environment for a device
        //
        InputContext->Slot.TTPortNum   = ParentPort;
        InputContext->Slot.TTHubSlotId = ParentSlotId;
      }
    } else {
      //
      // Inherit the TT parameters from parent device.
      //
      InputContext->Slot.TTPortNum   = ParentDeviceContext->Slot.TTPortNum;
      InputContext->Slot.TTHubSlotId = ParentDeviceContext->Slot.TTHubSlotId;
      //
      // If the device is a High speed device then down the speed to be the same as its parent Hub
      //
      if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
        InputContext->Slot.Speed = ParentDeviceContext->Slot.Speed;
      }
    }
  }

  //
  // 4) Allocate and initialize the Transfer Ring for the Default Control Endpoint.
  //
  EndpointTransferRing = AllocateZeroPool (sizeof (TRANSFER_RING));
  Xhc->UsbDevContext[SlotId].EndpointTransferRing[0] = EndpointTransferRing;
  CreateTransferRing(Xhc, TR_RING_TRB_NUMBER, (TRANSFER_RING *)Xhc->UsbDevContext[SlotId].EndpointTransferRing[0]);
  //
  // 5) Initialize the Input default control Endpoint 0 Context (6.2.3).
  //
  InputContext->EP[0].EPType = ED_CONTROL_BIDIR;

  if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
    InputContext->EP[0].MaxPacketSize = 512;
  } else if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
    InputContext->EP[0].MaxPacketSize = 64;
  } else {
    InputContext->EP[0].MaxPacketSize = 8;
  }
  //
  // Initial value of Average TRB Length for Control endpoints would be 8B, Interrupt endpoints
  // 1KB, and Bulk and Isoch endpoints 3KB.
  //
  InputContext->EP[0].AverageTRBLength = 8;
  InputContext->EP[0].MaxBurstSize     = 0;
  InputContext->EP[0].Interval         = 0;
  InputContext->EP[0].MaxPStreams      = 0;
  InputContext->EP[0].Mult             = 0;
  InputContext->EP[0].CErr             = 3;

  //
  // Init the DCS(dequeue cycle state) as the transfer ring's CCS
  //
  InputContext->EP[0].PtrLo = XHC_LOW_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[0])->RingSeg0) | BIT0;
  InputContext->EP[0].PtrHi = XHC_HIGH_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[0])->RingSeg0);

  //
  // 6) Allocate the Output Device Context data structure (6.2.1) and initialize it to '0'.
  //
  OutputContext = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (DEVICE_CONTEXT)));
  ASSERT (OutputContext != NULL);
  ASSERT (((UINTN) OutputContext & 0x3F) == 0);
  ZeroMem (OutputContext, sizeof (DEVICE_CONTEXT));

  Xhc->UsbDevContext[SlotId].OutputContext = OutputContext;
  //
  // 7) Load the appropriate (Device Slot ID) entry in the Device Context Base Address Array (5.4.6) with
  //    a pointer to the Output Device Context data structure (6.2.1).
  //
  Xhc->DCBAA[SlotId] = (UINT64) (UINTN) OutputContext;

  //
  // 8) Issue an Address Device Command for the Device Slot, where the command points to the Input
  //    Context data structure described above.
  //
  ZeroMem (&CmdTrbAddr, sizeof (CmdTrbAddr));
  CmdTrbAddr.PtrLo    = XHC_LOW_32BIT (Xhc->UsbDevContext[SlotId].InputContext);
  CmdTrbAddr.PtrHi    = XHC_HIGH_32BIT (Xhc->UsbDevContext[SlotId].InputContext);
  CmdTrbAddr.CycleBit = 1;
  CmdTrbAddr.Type     = TRB_TYPE_ADDRESS_DEV;
  CmdTrbAddr.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbAddr,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT (!EFI_ERROR(Status));

  DeviceAddress = (UINT8) ((DEVICE_CONTEXT *) OutputContext)->Slot.DeviceAddress;
  DEBUG ((EFI_D_INFO, "    Address %d assigned successfully\n", DeviceAddress));

  Xhc->UsbDevContext[SlotId].XhciDevAddr = DeviceAddress;

  return Status;
}

/**
  Assign and initialize the device slot for a new device.

  @param  Xhc                 The XHCI Instance.
  @param  ParentRouteChart    The route string pointed to the parent device.
  @param  ParentPort          The port at which the device is located.
  @param  RouteChart          The route string pointed to the device.
  @param  DeviceSpeed         The device speed.

  @retval EFI_SUCCESS   Successfully assign a slot to the device and assign an address to it.

**/
EFI_STATUS
EFIAPI
XhcInitializeDeviceSlot64 (
  IN  USB_XHCI_INSTANCE         *Xhc,
  IN  USB_DEV_ROUTE             ParentRouteChart,
  IN  UINT16                    ParentPort,
  IN  USB_DEV_ROUTE             RouteChart,
  IN  UINT8                     DeviceSpeed
  )
{
  EFI_STATUS                  Status;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  INPUT_CONTEXT_64            *InputContext;
  DEVICE_CONTEXT_64           *OutputContext;
  TRANSFER_RING               *EndpointTransferRing;
  CMD_TRB_ADDRESS_DEVICE      CmdTrbAddr;
  UINT8                       DeviceAddress;
  CMD_TRB_ENABLE_SLOT         CmdTrb;
  UINT8                       SlotId;
  UINT8                       ParentSlotId;
  DEVICE_CONTEXT_64           *ParentDeviceContext;

  ZeroMem (&CmdTrb, sizeof (CMD_TRB_ENABLE_SLOT));
  CmdTrb.CycleBit = 1;
  CmdTrb.Type     = TRB_TYPE_EN_SLOT;

  Status = XhcCmdTransfer (
              Xhc,
              (TRB_TEMPLATE *) (UINTN) &CmdTrb,
              XHC_GENERIC_TIMEOUT,
              (TRB_TEMPLATE **) (UINTN) &EvtTrb
              );
  ASSERT_EFI_ERROR (Status);
  ASSERT (EvtTrb->SlotId <= Xhc->MaxSlotsEn);
  DEBUG ((EFI_D_INFO, "Enable Slot Successfully, The Slot ID = 0x%x\n", EvtTrb->SlotId));
  SlotId = (UINT8)EvtTrb->SlotId;
  ASSERT (SlotId != 0);

  ZeroMem (&Xhc->UsbDevContext[SlotId], sizeof (USB_DEV_CONTEXT));
  Xhc->UsbDevContext[SlotId].Enabled                 = TRUE;
  Xhc->UsbDevContext[SlotId].SlotId                  = SlotId;
  Xhc->UsbDevContext[SlotId].RouteString.Dword       = RouteChart.Dword;
  Xhc->UsbDevContext[SlotId].ParentRouteString.Dword = ParentRouteChart.Dword;

  //
  // 4.3.3 Device Slot Initialization
  // 1) Allocate an Input Context data structure (6.2.5) and initialize all fields to '0'.
  //
  InputContext = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (INPUT_CONTEXT_64)));
  ASSERT (InputContext != NULL);
  ASSERT (((UINTN) InputContext & 0x3F) == 0);
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT_64));

  Xhc->UsbDevContext[SlotId].InputContext = (VOID *) InputContext;

  //
  // 2) Initialize the Input Control Context (6.2.5.1) of the Input Context by setting the A0 and A1
  //    flags to '1'. These flags indicate that the Slot Context and the Endpoint 0 Context of the Input
  //    Context are affected by the command.
  //
  InputContext->InputControlContext.Dword2 |= (BIT0 | BIT1);

  //
  // 3) Initialize the Input Slot Context data structure
  //
  InputContext->Slot.RouteString    = RouteChart.Route.RouteString;
  InputContext->Slot.Speed          = DeviceSpeed + 1;
  InputContext->Slot.ContextEntries = 1;
  InputContext->Slot.RootHubPortNum = RouteChart.Route.RootPortNum;

  if (RouteChart.Route.RouteString) {
    //
    // The device is behind of hub device.
    //
    ParentSlotId = XhcRouteStringToSlotId(Xhc, ParentRouteChart);
    ASSERT (ParentSlotId != 0);
    //
    //if the Full/Low device attached to a High Speed Hub, Init the TTPortNum and TTHubSlotId field of slot context
    //
    ParentDeviceContext = (DEVICE_CONTEXT_64 *)Xhc->UsbDevContext[ParentSlotId].OutputContext;
    if ((ParentDeviceContext->Slot.TTPortNum == 0) &&
        (ParentDeviceContext->Slot.TTHubSlotId == 0)) {
      if ((ParentDeviceContext->Slot.Speed == (EFI_USB_SPEED_HIGH + 1)) && (DeviceSpeed < EFI_USB_SPEED_HIGH)) {
        //
        // Full/Low device attached to High speed hub port that isolates the high speed signaling
        // environment from Full/Low speed signaling environment for a device
        //
        InputContext->Slot.TTPortNum   = ParentPort;
        InputContext->Slot.TTHubSlotId = ParentSlotId;
      }
    } else {
      //
      // Inherit the TT parameters from parent device.
      //
      InputContext->Slot.TTPortNum   = ParentDeviceContext->Slot.TTPortNum;
      InputContext->Slot.TTHubSlotId = ParentDeviceContext->Slot.TTHubSlotId;
      //
      // If the device is a High speed device then down the speed to be the same as its parent Hub
      //
      if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
        InputContext->Slot.Speed = ParentDeviceContext->Slot.Speed;
      }
    }
  }

  //
  // 4) Allocate and initialize the Transfer Ring for the Default Control Endpoint.
  //
  EndpointTransferRing = AllocateZeroPool (sizeof (TRANSFER_RING));
  Xhc->UsbDevContext[SlotId].EndpointTransferRing[0] = EndpointTransferRing;
  CreateTransferRing(Xhc, TR_RING_TRB_NUMBER, (TRANSFER_RING *)Xhc->UsbDevContext[SlotId].EndpointTransferRing[0]);
  //
  // 5) Initialize the Input default control Endpoint 0 Context (6.2.3).
  //
  InputContext->EP[0].EPType = ED_CONTROL_BIDIR;

  if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
    InputContext->EP[0].MaxPacketSize = 512;
  } else if (DeviceSpeed == EFI_USB_SPEED_HIGH) {
    InputContext->EP[0].MaxPacketSize = 64;
  } else {
    InputContext->EP[0].MaxPacketSize = 8;
  }
  //
  // Initial value of Average TRB Length for Control endpoints would be 8B, Interrupt endpoints
  // 1KB, and Bulk and Isoch endpoints 3KB.
  //
  InputContext->EP[0].AverageTRBLength = 8;
  InputContext->EP[0].MaxBurstSize     = 0;
  InputContext->EP[0].Interval         = 0;
  InputContext->EP[0].MaxPStreams      = 0;
  InputContext->EP[0].Mult             = 0;
  InputContext->EP[0].CErr             = 3;

  //
  // Init the DCS(dequeue cycle state) as the transfer ring's CCS
  //
  InputContext->EP[0].PtrLo = XHC_LOW_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[0])->RingSeg0) | BIT0;
  InputContext->EP[0].PtrHi = XHC_HIGH_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[0])->RingSeg0);

  //
  // 6) Allocate the Output Device Context data structure (6.2.1) and initialize it to '0'.
  //
  OutputContext = AllocatePages (EFI_SIZE_TO_PAGES (sizeof (DEVICE_CONTEXT_64)));
  ASSERT (OutputContext != NULL);
  ASSERT (((UINTN) OutputContext & 0x3F) == 0);
  ZeroMem (OutputContext, sizeof (DEVICE_CONTEXT_64));

  Xhc->UsbDevContext[SlotId].OutputContext = OutputContext;
  //
  // 7) Load the appropriate (Device Slot ID) entry in the Device Context Base Address Array (5.4.6) with
  //    a pointer to the Output Device Context data structure (6.2.1).
  //
  Xhc->DCBAA[SlotId] = (UINT64) (UINTN) OutputContext;

  //
  // 8) Issue an Address Device Command for the Device Slot, where the command points to the Input
  //    Context data structure described above.
  //
  ZeroMem (&CmdTrbAddr, sizeof (CmdTrbAddr));
  CmdTrbAddr.PtrLo    = XHC_LOW_32BIT (Xhc->UsbDevContext[SlotId].InputContext);
  CmdTrbAddr.PtrHi    = XHC_HIGH_32BIT (Xhc->UsbDevContext[SlotId].InputContext);
  CmdTrbAddr.CycleBit = 1;
  CmdTrbAddr.Type     = TRB_TYPE_ADDRESS_DEV;
  CmdTrbAddr.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbAddr,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT (!EFI_ERROR(Status));

  DeviceAddress = (UINT8) ((DEVICE_CONTEXT_64 *) OutputContext)->Slot.DeviceAddress;
  DEBUG ((EFI_D_INFO, "    Address %d assigned successfully\n", DeviceAddress));

  Xhc->UsbDevContext[SlotId].XhciDevAddr = DeviceAddress;

  return Status;
}


/**
  Disable the specified device slot.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be disabled.

  @retval EFI_SUCCESS   Successfully disable the device slot.

**/
EFI_STATUS
EFIAPI
XhcDisableSlotCmd (
  IN USB_XHCI_INSTANCE         *Xhc,
  IN UINT8                     SlotId
  )
{
  EFI_STATUS            Status;
  TRB_TEMPLATE          *EvtTrb;
  CMD_TRB_DISABLE_SLOT  CmdTrbDisSlot;
  UINT8                 Index;
  VOID                  *RingSeg;

  //
  // Disable the device slots occupied by these devices on its downstream ports.
  // Entry 0 is reserved.
  //
  for (Index = 0; Index < 255; Index++) {
    if (!Xhc->UsbDevContext[Index + 1].Enabled ||
        (Xhc->UsbDevContext[Index + 1].SlotId == 0) ||
        (Xhc->UsbDevContext[Index + 1].ParentRouteString.Dword != Xhc->UsbDevContext[SlotId].RouteString.Dword)) {
      continue;
    }

    Status = XhcDisableSlotCmd (Xhc, Xhc->UsbDevContext[Index + 1].SlotId);

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "XhcDisableSlotCmd: failed to disable child, ignore error\n"));
      Xhc->UsbDevContext[Index + 1].SlotId = 0;
    }
  }

  //
  // Construct the disable slot command
  //
  DEBUG ((EFI_D_INFO, "Disable device slot %d!\n", SlotId));

  ZeroMem (&CmdTrbDisSlot, sizeof (CmdTrbDisSlot));
  CmdTrbDisSlot.CycleBit = 1;
  CmdTrbDisSlot.Type     = TRB_TYPE_DIS_SLOT;
  CmdTrbDisSlot.SlotId   = SlotId;
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbDisSlot,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT_EFI_ERROR(Status);
  //
  // Free the slot's device context entry
  //
  Xhc->DCBAA[SlotId] = 0;

  //
  // Free the slot related data structure
  //
  for (Index = 0; Index < 31; Index++) {
    if (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Index] != NULL) {
      RingSeg = ((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Index])->RingSeg0;
      if (RingSeg != NULL) {
        FreePages (RingSeg, EFI_SIZE_TO_PAGES (sizeof (TRB_TEMPLATE) * TR_RING_TRB_NUMBER));
      }
      FreePool (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Index]);
    }
  }

  for (Index = 0; Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations; Index++) {
    if (Xhc->UsbDevContext[SlotId].ConfDesc[Index] != NULL) {
      FreePool (Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
    }
  }

  if (Xhc->UsbDevContext[SlotId].InputContext != NULL) {
    FreePages (Xhc->UsbDevContext[SlotId].InputContext, EFI_SIZE_TO_PAGES (sizeof (INPUT_CONTEXT)));
  }

  if (Xhc->UsbDevContext[SlotId].OutputContext != NULL) {
    FreePages (Xhc->UsbDevContext[SlotId].OutputContext, EFI_SIZE_TO_PAGES (sizeof (DEVICE_CONTEXT)));
  }
  //
  // Doesn't zero the entry because XhcAsyncInterruptTransfer() may be invoked to remove the established
  // asynchronous interrupt pipe after the device is disabled. It needs the device address mapping info to
  // remove urb from XHCI's asynchronous transfer list.
  //
  Xhc->UsbDevContext[SlotId].Enabled = FALSE;
  Xhc->UsbDevContext[SlotId].SlotId  = 0;

  return Status;
}

/**
  Disable the specified device slot.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be disabled.

  @retval EFI_SUCCESS   Successfully disable the device slot.

**/
EFI_STATUS
EFIAPI
XhcDisableSlotCmd64 (
  IN USB_XHCI_INSTANCE         *Xhc,
  IN UINT8                     SlotId
  )
{
  EFI_STATUS            Status;
  TRB_TEMPLATE          *EvtTrb;
  CMD_TRB_DISABLE_SLOT  CmdTrbDisSlot;
  UINT8                 Index;
  VOID                  *RingSeg;

  //
  // Disable the device slots occupied by these devices on its downstream ports.
  // Entry 0 is reserved.
  //
  for (Index = 0; Index < 255; Index++) {
    if (!Xhc->UsbDevContext[Index + 1].Enabled ||
        (Xhc->UsbDevContext[Index + 1].SlotId == 0) ||
        (Xhc->UsbDevContext[Index + 1].ParentRouteString.Dword != Xhc->UsbDevContext[SlotId].RouteString.Dword)) {
      continue;
    }

    Status = XhcDisableSlotCmd64 (Xhc, Xhc->UsbDevContext[Index + 1].SlotId);

    if (EFI_ERROR (Status)) {
      DEBUG ((EFI_D_ERROR, "XhcDisableSlotCmd: failed to disable child, ignore error\n"));
      Xhc->UsbDevContext[Index + 1].SlotId = 0;
    }
  }

  //
  // Construct the disable slot command
  //
  DEBUG ((EFI_D_INFO, "Disable device slot %d!\n", SlotId));

  ZeroMem (&CmdTrbDisSlot, sizeof (CmdTrbDisSlot));
  CmdTrbDisSlot.CycleBit = 1;
  CmdTrbDisSlot.Type     = TRB_TYPE_DIS_SLOT;
  CmdTrbDisSlot.SlotId   = SlotId;
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbDisSlot,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT_EFI_ERROR(Status);
  //
  // Free the slot's device context entry
  //
  Xhc->DCBAA[SlotId] = 0;

  //
  // Free the slot related data structure
  //
  for (Index = 0; Index < 31; Index++) {
    if (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Index] != NULL) {
      RingSeg = ((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Index])->RingSeg0;
      if (RingSeg != NULL) {
        FreePages (RingSeg, EFI_SIZE_TO_PAGES (sizeof (TRB_TEMPLATE) * TR_RING_TRB_NUMBER));
      }
      FreePool (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Index]);
    }
  }

  for (Index = 0; Index < Xhc->UsbDevContext[SlotId].DevDesc.NumConfigurations; Index++) {
    if (Xhc->UsbDevContext[SlotId].ConfDesc[Index] != NULL) {
      FreePool (Xhc->UsbDevContext[SlotId].ConfDesc[Index]);
    }
  }

  if (Xhc->UsbDevContext[SlotId].InputContext != NULL) {
    FreePages (Xhc->UsbDevContext[SlotId].InputContext, EFI_SIZE_TO_PAGES (sizeof (INPUT_CONTEXT_64)));
  }

  if (Xhc->UsbDevContext[SlotId].OutputContext != NULL) {
    FreePages (Xhc->UsbDevContext[SlotId].OutputContext, EFI_SIZE_TO_PAGES (sizeof (DEVICE_CONTEXT_64)));
  }
  //
  // Doesn't zero the entry because XhcAsyncInterruptTransfer() may be invoked to remove the established
  // asynchronous interrupt pipe after the device is disabled. It needs the device address mapping info to
  // remove urb from XHCI's asynchronous transfer list.
  //
  Xhc->UsbDevContext[SlotId].Enabled = FALSE;
  Xhc->UsbDevContext[SlotId].SlotId  = 0;

  return Status;
}


/**
  Configure all the device endpoints through XHCI's Configure_Endpoint cmd.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be configured.
  @param  DeviceSpeed   The device's speed.
  @param  ConfigDesc    The pointer to the usb device configuration descriptor.

  @retval EFI_SUCCESS   Successfully configure all the device endpoints.

**/
EFI_STATUS
EFIAPI
XhcSetConfigCmd (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN UINT8                    SlotId,
  IN UINT8                    DeviceSpeed,
  IN USB_CONFIG_DESCRIPTOR    *ConfigDesc
  )
{
  EFI_STATUS                  Status;

  USB_INTERFACE_DESCRIPTOR    *IfDesc;
  USB_ENDPOINT_DESCRIPTOR     *EpDesc;
  UINT8                       Index;
  UINTN                       NumEp;
  UINTN                       EpIndex;
  UINT8                       EpAddr;
  UINT8                       Direction;
  UINT8                       Dci;
  UINT8                       MaxDci;
  UINT32                      PhyAddr;
  UINT8                       Interval;

  TRANSFER_RING               *EndpointTransferRing;
  CMD_TRB_CONFIG_ENDPOINT     CmdTrbCfgEP;
  INPUT_CONTEXT               *InputContext;
  DEVICE_CONTEXT              *OutputContext;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  //
  // 4.6.6 Configure Endpoint
  //
  InputContext  = Xhc->UsbDevContext[SlotId].InputContext;
  OutputContext = Xhc->UsbDevContext[SlotId].OutputContext;
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT));
  CopyMem (&InputContext->Slot, &OutputContext->Slot, sizeof (SLOT_CONTEXT));

  ASSERT (ConfigDesc != NULL);

  MaxDci = 0;

  IfDesc = (USB_INTERFACE_DESCRIPTOR *)(ConfigDesc + 1);
  for (Index = 0; Index < ConfigDesc->NumInterfaces; Index++) {
    while (IfDesc->DescriptorType != USB_DESC_TYPE_INTERFACE) {
      IfDesc = (USB_INTERFACE_DESCRIPTOR *)((UINTN)IfDesc + IfDesc->Length);
    }

    NumEp = IfDesc->NumEndpoints;

    EpDesc = (USB_ENDPOINT_DESCRIPTOR *)(IfDesc + 1);
    for (EpIndex = 0; EpIndex < NumEp; EpIndex++) {
      while (EpDesc->DescriptorType != USB_DESC_TYPE_ENDPOINT) {
        EpDesc = (USB_ENDPOINT_DESCRIPTOR *)((UINTN)EpDesc + EpDesc->Length);
      }

      EpAddr    = (UINT8)(EpDesc->EndpointAddress & 0x0F);
      Direction = (UINT8)((EpDesc->EndpointAddress & 0x80) ? EfiUsbDataIn : EfiUsbDataOut);

      Dci = XhcEndpointToDci (EpAddr, Direction);
      ASSERT (Dci < 32);
      if (Dci > MaxDci) {
        MaxDci = Dci;
      }

      InputContext->InputControlContext.Dword2 |= (BIT0 << Dci);
      InputContext->EP[Dci-1].MaxPacketSize     = EpDesc->MaxPacketSize;

      if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
        //
        // 6.2.3.4, shall be set to the value defined in the bMaxBurst field of the SuperSpeed Endpoint Companion Descriptor.
        //
        InputContext->EP[Dci-1].MaxBurstSize = 0x0;
      } else {
        InputContext->EP[Dci-1].MaxBurstSize = 0x0;
      }

      switch (EpDesc->Attributes & USB_ENDPOINT_TYPE_MASK) {
        case USB_ENDPOINT_BULK:
          if (Direction == EfiUsbDataIn) {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_BULK_IN;
          } else {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_BULK_OUT;
          }

          InputContext->EP[Dci-1].AverageTRBLength = 0x1000;
          if (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] == NULL) {
            EndpointTransferRing = AllocateZeroPool(sizeof (TRANSFER_RING));
            Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] = (VOID *) EndpointTransferRing;
            CreateTransferRing(Xhc, TR_RING_TRB_NUMBER, (TRANSFER_RING *)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1]);
          }

          break;
        case USB_ENDPOINT_ISO:
          if (Direction == EfiUsbDataIn) {
            InputContext->EP[Dci-1].CErr   = 0;
            InputContext->EP[Dci-1].EPType = ED_ISOCH_IN;
          } else {
            InputContext->EP[Dci-1].CErr   = 0;
            InputContext->EP[Dci-1].EPType = ED_ISOCH_OUT;
          }
          break;
        case USB_ENDPOINT_INTERRUPT:
          if (Direction == EfiUsbDataIn) {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_INTERRUPT_IN;
          } else {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_INTERRUPT_OUT;
          }
          InputContext->EP[Dci-1].AverageTRBLength = 0x1000;
          InputContext->EP[Dci-1].MaxESITPayload   = EpDesc->MaxPacketSize;
          //
          // Get the bInterval from descriptor and init the the interval field of endpoint context
          //
          if ((DeviceSpeed == EFI_USB_SPEED_FULL) || (DeviceSpeed == EFI_USB_SPEED_LOW)) {
            Interval = EpDesc->Interval;
            //
            // Hard code the interval to MAX first, need calculate through the bInterval field of Endpoint descriptor.
            //
            InputContext->EP[Dci-1].Interval = 6;
          } else if ((DeviceSpeed == EFI_USB_SPEED_HIGH) || (DeviceSpeed == EFI_USB_SPEED_SUPER)) {
            Interval = EpDesc->Interval;
            ASSERT (Interval >= 1 && Interval <= 16);
            //
            // Refer to XHCI 1.0 spec section 6.2.3.6, table 61
            //
            InputContext->EP[Dci-1].Interval         = Interval - 1;
            InputContext->EP[Dci-1].AverageTRBLength = 0x1000;
            InputContext->EP[Dci-1].MaxESITPayload   = 0x0002;
            InputContext->EP[Dci-1].MaxBurstSize     = 0x0;
            InputContext->EP[Dci-1].CErr             = 3;
          }

          if (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] == NULL) {
            EndpointTransferRing = AllocateZeroPool(sizeof (TRANSFER_RING));
            Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] = (VOID *) EndpointTransferRing;
            CreateTransferRing(Xhc, TR_RING_TRB_NUMBER, (TRANSFER_RING *)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1]);
          }
          break;

        case USB_ENDPOINT_CONTROL:
        default:
          ASSERT (0);
          break;
      }

      PhyAddr  = XHC_LOW_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1])->RingSeg0);
      PhyAddr &= ~(0x0F);
      PhyAddr |= ((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1])->RingPCS;
      InputContext->EP[Dci-1].PtrLo = PhyAddr;
      InputContext->EP[Dci-1].PtrHi = XHC_HIGH_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1])->RingSeg0);

      EpDesc = (USB_ENDPOINT_DESCRIPTOR *)((UINTN)EpDesc + EpDesc->Length);
    }
    IfDesc = (USB_INTERFACE_DESCRIPTOR *)((UINTN)IfDesc + IfDesc->Length);
  }

  InputContext->InputControlContext.Dword2 |= BIT0;
  InputContext->Slot.ContextEntries         = MaxDci;
  //
  // configure endpoint
  //
  ZeroMem (&CmdTrbCfgEP, sizeof (CmdTrbCfgEP));
  CmdTrbCfgEP.PtrLo    = XHC_LOW_32BIT (InputContext);
  CmdTrbCfgEP.PtrHi    = XHC_HIGH_32BIT (InputContext);
  CmdTrbCfgEP.CycleBit = 1;
  CmdTrbCfgEP.Type     = TRB_TYPE_CON_ENDPOINT;
  CmdTrbCfgEP.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  DEBUG ((EFI_D_INFO, "Configure Endpoint\n"));
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbCfgEP,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT_EFI_ERROR(Status);

  return Status;
}

/**
  Configure all the device endpoints through XHCI's Configure_Endpoint cmd.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be configured.
  @param  DeviceSpeed   The device's speed.
  @param  ConfigDesc    The pointer to the usb device configuration descriptor.

  @retval EFI_SUCCESS   Successfully configure all the device endpoints.

**/
EFI_STATUS
EFIAPI
XhcSetConfigCmd64 (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN UINT8                    SlotId,
  IN UINT8                    DeviceSpeed,
  IN USB_CONFIG_DESCRIPTOR    *ConfigDesc
  )
{
  EFI_STATUS                  Status;

  USB_INTERFACE_DESCRIPTOR    *IfDesc;
  USB_ENDPOINT_DESCRIPTOR     *EpDesc;
  UINT8                       Index;
  UINTN                       NumEp;
  UINTN                       EpIndex;
  UINT8                       EpAddr;
  UINT8                       Direction;
  UINT8                       Dci;
  UINT8                       MaxDci;
  UINT32                      PhyAddr;
  UINT8                       Interval;

  TRANSFER_RING               *EndpointTransferRing;
  CMD_TRB_CONFIG_ENDPOINT     CmdTrbCfgEP;
  INPUT_CONTEXT_64            *InputContext;
  DEVICE_CONTEXT_64           *OutputContext;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  //
  // 4.6.6 Configure Endpoint
  //
  InputContext  = Xhc->UsbDevContext[SlotId].InputContext;
  OutputContext = Xhc->UsbDevContext[SlotId].OutputContext;
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT_64));
  CopyMem (&InputContext->Slot, &OutputContext->Slot, sizeof (SLOT_CONTEXT_64));

  ASSERT (ConfigDesc != NULL);

  MaxDci = 0;

  IfDesc = (USB_INTERFACE_DESCRIPTOR *)(ConfigDesc + 1);
  for (Index = 0; Index < ConfigDesc->NumInterfaces; Index++) {
    while (IfDesc->DescriptorType != USB_DESC_TYPE_INTERFACE) {
      IfDesc = (USB_INTERFACE_DESCRIPTOR *)((UINTN)IfDesc + IfDesc->Length);
    }

    NumEp = IfDesc->NumEndpoints;

    EpDesc = (USB_ENDPOINT_DESCRIPTOR *)(IfDesc + 1);
    for (EpIndex = 0; EpIndex < NumEp; EpIndex++) {
      while (EpDesc->DescriptorType != USB_DESC_TYPE_ENDPOINT) {
        EpDesc = (USB_ENDPOINT_DESCRIPTOR *)((UINTN)EpDesc + EpDesc->Length);
      }

      EpAddr    = (UINT8)(EpDesc->EndpointAddress & 0x0F);
      Direction = (UINT8)((EpDesc->EndpointAddress & 0x80) ? EfiUsbDataIn : EfiUsbDataOut);

      Dci = XhcEndpointToDci (EpAddr, Direction);
      ASSERT (Dci < 32);
      if (Dci > MaxDci) {
        MaxDci = Dci;
      }

      InputContext->InputControlContext.Dword2 |= (BIT0 << Dci);
      InputContext->EP[Dci-1].MaxPacketSize     = EpDesc->MaxPacketSize;

      if (DeviceSpeed == EFI_USB_SPEED_SUPER) {
        //
        // 6.2.3.4, shall be set to the value defined in the bMaxBurst field of the SuperSpeed Endpoint Companion Descriptor.
        //
        InputContext->EP[Dci-1].MaxBurstSize = 0x0;
      } else {
        InputContext->EP[Dci-1].MaxBurstSize = 0x0;
      }

      switch (EpDesc->Attributes & USB_ENDPOINT_TYPE_MASK) {
        case USB_ENDPOINT_BULK:
          if (Direction == EfiUsbDataIn) {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_BULK_IN;
          } else {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_BULK_OUT;
          }

          InputContext->EP[Dci-1].AverageTRBLength = 0x1000;
          if (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] == NULL) {
            EndpointTransferRing = AllocateZeroPool(sizeof (TRANSFER_RING));
            Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] = (VOID *) EndpointTransferRing;
            CreateTransferRing(Xhc, TR_RING_TRB_NUMBER, (TRANSFER_RING *)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1]);
          }

          break;
        case USB_ENDPOINT_ISO:
          if (Direction == EfiUsbDataIn) {
            InputContext->EP[Dci-1].CErr   = 0;
            InputContext->EP[Dci-1].EPType = ED_ISOCH_IN;
          } else {
            InputContext->EP[Dci-1].CErr   = 0;
            InputContext->EP[Dci-1].EPType = ED_ISOCH_OUT;
          }
          break;
        case USB_ENDPOINT_INTERRUPT:
          if (Direction == EfiUsbDataIn) {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_INTERRUPT_IN;
          } else {
            InputContext->EP[Dci-1].CErr   = 3;
            InputContext->EP[Dci-1].EPType = ED_INTERRUPT_OUT;
          }
          InputContext->EP[Dci-1].AverageTRBLength = 0x1000;
          InputContext->EP[Dci-1].MaxESITPayload   = EpDesc->MaxPacketSize;
          //
          // Get the bInterval from descriptor and init the the interval field of endpoint context
          //
          if ((DeviceSpeed == EFI_USB_SPEED_FULL) || (DeviceSpeed == EFI_USB_SPEED_LOW)) {
            Interval = EpDesc->Interval;
            //
            // Hard code the interval to MAX first, need calculate through the bInterval field of Endpoint descriptor.
            //
            InputContext->EP[Dci-1].Interval = 6;
          } else if ((DeviceSpeed == EFI_USB_SPEED_HIGH) || (DeviceSpeed == EFI_USB_SPEED_SUPER)) {
            Interval = EpDesc->Interval;
            ASSERT (Interval >= 1 && Interval <= 16);
            //
            // Refer to XHCI 1.0 spec section 6.2.3.6, table 61
            //
            InputContext->EP[Dci-1].Interval         = Interval - 1;
            InputContext->EP[Dci-1].AverageTRBLength = 0x1000;
            InputContext->EP[Dci-1].MaxESITPayload   = 0x0002;
            InputContext->EP[Dci-1].MaxBurstSize     = 0x0;
            InputContext->EP[Dci-1].CErr             = 3;
          }

          if (Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] == NULL) {
            EndpointTransferRing = AllocateZeroPool(sizeof (TRANSFER_RING));
            Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1] = (VOID *) EndpointTransferRing;
            CreateTransferRing(Xhc, TR_RING_TRB_NUMBER, (TRANSFER_RING *)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1]);
          }
          break;

        case USB_ENDPOINT_CONTROL:
        default:
          ASSERT (0);
          break;
      }

      PhyAddr  = XHC_LOW_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1])->RingSeg0);
      PhyAddr &= ~(0x0F);
      PhyAddr |= ((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1])->RingPCS;
      InputContext->EP[Dci-1].PtrLo = PhyAddr;
      InputContext->EP[Dci-1].PtrHi = XHC_HIGH_32BIT (((TRANSFER_RING *)(UINTN)Xhc->UsbDevContext[SlotId].EndpointTransferRing[Dci-1])->RingSeg0);

      EpDesc = (USB_ENDPOINT_DESCRIPTOR *)((UINTN)EpDesc + EpDesc->Length);
    }
    IfDesc = (USB_INTERFACE_DESCRIPTOR *)((UINTN)IfDesc + IfDesc->Length);
  }

  InputContext->InputControlContext.Dword2 |= BIT0;
  InputContext->Slot.ContextEntries         = MaxDci;
  //
  // configure endpoint
  //
  ZeroMem (&CmdTrbCfgEP, sizeof (CmdTrbCfgEP));
  CmdTrbCfgEP.PtrLo    = XHC_LOW_32BIT (InputContext);
  CmdTrbCfgEP.PtrHi    = XHC_HIGH_32BIT (InputContext);
  CmdTrbCfgEP.CycleBit = 1;
  CmdTrbCfgEP.Type     = TRB_TYPE_CON_ENDPOINT;
  CmdTrbCfgEP.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  DEBUG ((EFI_D_INFO, "Configure Endpoint\n"));
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbCfgEP,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT_EFI_ERROR(Status);

  return Status;
}


/**
  Evaluate the endpoint 0 context through XHCI's Evaluate_Context cmd.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be evaluated.
  @param  MaxPacketSize The max packet size supported by the device control transfer.

  @retval EFI_SUCCESS   Successfully evaluate the device endpoint 0.

**/
EFI_STATUS
EFIAPI
XhcEvaluateContext (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN UINT8                    SlotId,
  IN UINT32                   MaxPacketSize
  )
{
  EFI_STATUS                  Status;
  CMD_TRB_EVALUATE_CONTEXT    CmdTrbEvalu;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  INPUT_CONTEXT               *InputContext;

  ASSERT (Xhc->UsbDevContext[SlotId].SlotId != 0);

  //
  // 4.6.7 Evaluate Context
  //
  InputContext = Xhc->UsbDevContext[SlotId].InputContext;
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT));

  InputContext->InputControlContext.Dword2 |= BIT1;
  InputContext->EP[0].MaxPacketSize         = MaxPacketSize;

  ZeroMem (&CmdTrbEvalu, sizeof (CmdTrbEvalu));
  CmdTrbEvalu.PtrLo    = XHC_LOW_32BIT (InputContext);
  CmdTrbEvalu.PtrHi    = XHC_HIGH_32BIT (InputContext);
  CmdTrbEvalu.CycleBit = 1;
  CmdTrbEvalu.Type     = TRB_TYPE_EVALU_CONTXT;
  CmdTrbEvalu.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  DEBUG ((EFI_D_INFO, "Evaluate context\n"));
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbEvalu,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT (!EFI_ERROR(Status));

  return Status;
}

/**
  Evaluate the endpoint 0 context through XHCI's Evaluate_Context cmd.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be evaluated.
  @param  MaxPacketSize The max packet size supported by the device control transfer.

  @retval EFI_SUCCESS   Successfully evaluate the device endpoint 0.

**/
EFI_STATUS
EFIAPI
XhcEvaluateContext64 (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN UINT8                    SlotId,
  IN UINT32                   MaxPacketSize
  )
{
  EFI_STATUS                  Status;
  CMD_TRB_EVALUATE_CONTEXT    CmdTrbEvalu;
  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  INPUT_CONTEXT_64            *InputContext;

  ASSERT (Xhc->UsbDevContext[SlotId].SlotId != 0);

  //
  // 4.6.7 Evaluate Context
  //
  InputContext = Xhc->UsbDevContext[SlotId].InputContext;
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT_64));

  InputContext->InputControlContext.Dword2 |= BIT1;
  InputContext->EP[0].MaxPacketSize         = MaxPacketSize;

  ZeroMem (&CmdTrbEvalu, sizeof (CmdTrbEvalu));
  CmdTrbEvalu.PtrLo    = XHC_LOW_32BIT (InputContext);
  CmdTrbEvalu.PtrHi    = XHC_HIGH_32BIT (InputContext);
  CmdTrbEvalu.CycleBit = 1;
  CmdTrbEvalu.Type     = TRB_TYPE_EVALU_CONTXT;
  CmdTrbEvalu.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  DEBUG ((EFI_D_INFO, "Evaluate context\n"));
  Status = XhcCmdTransfer (
             Xhc,
             (TRB_TEMPLATE *) (UINTN) &CmdTrbEvalu,
             XHC_GENERIC_TIMEOUT,
             (TRB_TEMPLATE **) (UINTN) &EvtTrb
             );
  ASSERT (!EFI_ERROR(Status));

  return Status;
}


/**
  Evaluate the slot context for hub device through XHCI's Configure_Endpoint cmd.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be configured.
  @param  PortNum       The total number of downstream port supported by the hub.
  @param  TTT           The TT think time of the hub device.
  @param  MTT           The multi-TT of the hub device.

  @retval EFI_SUCCESS   Successfully configure the hub device's slot context.

**/
EFI_STATUS
XhcConfigHubContext (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN UINT8                    SlotId,
  IN UINT8                    PortNum,
  IN UINT8                    TTT,
  IN UINT8                    MTT
  )
{
  EFI_STATUS                  Status;

  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  INPUT_CONTEXT               *InputContext;
  DEVICE_CONTEXT              *OutputContext;
  CMD_TRB_CONFIG_ENDPOINT     CmdTrbCfgEP;

  ASSERT (Xhc->UsbDevContext[SlotId].SlotId != 0);
  InputContext  = Xhc->UsbDevContext[SlotId].InputContext;
  OutputContext = Xhc->UsbDevContext[SlotId].OutputContext;

  //
  // 4.6.7 Evaluate Context
  //
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT));

  InputContext->InputControlContext.Dword2 |= BIT0;

  //
  // Copy the slot context from OutputContext to Input context
  //
  CopyMem(&(InputContext->Slot), &(OutputContext->Slot), sizeof (SLOT_CONTEXT));
  InputContext->Slot.Hub     = 1;
  InputContext->Slot.PortNum = PortNum;
  InputContext->Slot.TTT     = TTT;
  InputContext->Slot.MTT     = MTT;

  ZeroMem (&CmdTrbCfgEP, sizeof (CmdTrbCfgEP));
  CmdTrbCfgEP.PtrLo    = XHC_LOW_32BIT (InputContext);
  CmdTrbCfgEP.PtrHi    = XHC_HIGH_32BIT (InputContext);
  CmdTrbCfgEP.CycleBit = 1;
  CmdTrbCfgEP.Type     = TRB_TYPE_CON_ENDPOINT;
  CmdTrbCfgEP.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  DEBUG ((EFI_D_INFO, "Configure Hub Slot Context\n"));
  Status = XhcCmdTransfer (
              Xhc,
              (TRB_TEMPLATE *) (UINTN) &CmdTrbCfgEP,
              XHC_GENERIC_TIMEOUT,
              (TRB_TEMPLATE **) (UINTN) &EvtTrb
              );
  ASSERT (!EFI_ERROR(Status));

  return Status;
}

/**
  Evaluate the slot context for hub device through XHCI's Configure_Endpoint cmd.

  @param  Xhc           The XHCI Instance.
  @param  SlotId        The slot id to be configured.
  @param  PortNum       The total number of downstream port supported by the hub.
  @param  TTT           The TT think time of the hub device.
  @param  MTT           The multi-TT of the hub device.

  @retval EFI_SUCCESS   Successfully configure the hub device's slot context.

**/
EFI_STATUS
XhcConfigHubContext64 (
  IN USB_XHCI_INSTANCE        *Xhc,
  IN UINT8                    SlotId,
  IN UINT8                    PortNum,
  IN UINT8                    TTT,
  IN UINT8                    MTT
  )
{
  EFI_STATUS                  Status;

  EVT_TRB_COMMAND_COMPLETION  *EvtTrb;
  INPUT_CONTEXT_64            *InputContext;
  DEVICE_CONTEXT_64           *OutputContext;
  CMD_TRB_CONFIG_ENDPOINT     CmdTrbCfgEP;

  ASSERT (Xhc->UsbDevContext[SlotId].SlotId != 0);
  InputContext  = Xhc->UsbDevContext[SlotId].InputContext;
  OutputContext = Xhc->UsbDevContext[SlotId].OutputContext;

  //
  // 4.6.7 Evaluate Context
  //
  ZeroMem (InputContext, sizeof (INPUT_CONTEXT_64));

  InputContext->InputControlContext.Dword2 |= BIT0;

  //
  // Copy the slot context from OutputContext to Input context
  //
  CopyMem(&(InputContext->Slot), &(OutputContext->Slot), sizeof (SLOT_CONTEXT_64));
  InputContext->Slot.Hub     = 1;
  InputContext->Slot.PortNum = PortNum;
  InputContext->Slot.TTT     = TTT;
  InputContext->Slot.MTT     = MTT;

  ZeroMem (&CmdTrbCfgEP, sizeof (CmdTrbCfgEP));
  CmdTrbCfgEP.PtrLo    = XHC_LOW_32BIT (InputContext);
  CmdTrbCfgEP.PtrHi    = XHC_HIGH_32BIT (InputContext);
  CmdTrbCfgEP.CycleBit = 1;
  CmdTrbCfgEP.Type     = TRB_TYPE_CON_ENDPOINT;
  CmdTrbCfgEP.SlotId   = Xhc->UsbDevContext[SlotId].SlotId;
  DEBUG ((EFI_D_INFO, "Configure Hub Slot Context\n"));
  Status = XhcCmdTransfer (
              Xhc,
              (TRB_TEMPLATE *) (UINTN) &CmdTrbCfgEP,
              XHC_GENERIC_TIMEOUT,
              (TRB_TEMPLATE **) (UINTN) &EvtTrb
              );
  ASSERT (!EFI_ERROR(Status));

  return Status;
}


