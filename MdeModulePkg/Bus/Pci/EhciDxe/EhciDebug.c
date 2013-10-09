/** @file

  This file provides the information dump support for EHCI when in debug mode.

Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include "Ehci.h"

/**
  Dump the status byte in QTD/QH to a more friendly format.

  @param  State    The state in the QTD/QH.

**/
VOID
EhcDumpStatus (
  IN UINT32               State
  )
{
  if (EHC_BIT_IS_SET (State, QTD_STAT_DO_PING)) {
    DEBUG ((EFI_D_VERBOSE, "  Do_Ping"));
  } else {
    DEBUG ((EFI_D_VERBOSE, "  Do_Out"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_DO_CS)) {
    DEBUG ((EFI_D_VERBOSE, "  Do_CS"));
  } else {
    DEBUG ((EFI_D_VERBOSE, "  Do_SS"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_TRANS_ERR)) {
    DEBUG ((EFI_D_VERBOSE, "  Transfer_Error"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_BABBLE_ERR)) {
    DEBUG ((EFI_D_VERBOSE, "  Babble_Error"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_BUFF_ERR)) {
    DEBUG ((EFI_D_VERBOSE, "  Buffer_Error"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_HALTED)) {
    DEBUG ((EFI_D_VERBOSE, "  Halted"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_ACTIVE)) {
    DEBUG ((EFI_D_VERBOSE, "  Active"));
  }

  DEBUG ((EFI_D_VERBOSE, "\n"));
}


/**
  Dump the fields of a QTD.

  @param  Qtd      The QTD to dump.
  @param  Msg      The message to print before the dump.

**/
VOID
EhcDumpQtd (
  IN EHC_QTD              *Qtd,
  IN CHAR8                *Msg
  )
{
  QTD_HW                  *QtdHw;
  UINTN                   Index;

  if (Msg != NULL) {
    DEBUG ((EFI_D_VERBOSE, Msg));
  }

  DEBUG ((EFI_D_VERBOSE, "Queue TD @ 0x%p, data length %d\n", Qtd, (UINT32)Qtd->DataLen));

  QtdHw = &Qtd->QtdHw;

  DEBUG ((EFI_D_VERBOSE, "Next QTD     : %x\n", QtdHw->NextQtd));
  DEBUG ((EFI_D_VERBOSE, "AltNext QTD  : %x\n", QtdHw->AltNext));
  DEBUG ((EFI_D_VERBOSE, "Status       : %x\n", QtdHw->Status));
  EhcDumpStatus (QtdHw->Status);

  if (QtdHw->Pid == QTD_PID_SETUP) {
    DEBUG ((EFI_D_VERBOSE, "PID          : Setup\n"));

  } else if (QtdHw->Pid == QTD_PID_INPUT) {
    DEBUG ((EFI_D_VERBOSE, "PID          : IN\n"));

  } else if (QtdHw->Pid == QTD_PID_OUTPUT) {
    DEBUG ((EFI_D_VERBOSE, "PID          : OUT\n"));

  }

  DEBUG ((EFI_D_VERBOSE, "Error Count  : %d\n", QtdHw->ErrCnt));
  DEBUG ((EFI_D_VERBOSE, "Current Page : %d\n", QtdHw->CurPage));
  DEBUG ((EFI_D_VERBOSE, "IOC          : %d\n", QtdHw->Ioc));
  DEBUG ((EFI_D_VERBOSE, "Total Bytes  : %d\n", QtdHw->TotalBytes));
  DEBUG ((EFI_D_VERBOSE, "Data Toggle  : %d\n", QtdHw->DataToggle));

  for (Index = 0; Index < 5; Index++) {
    DEBUG ((EFI_D_VERBOSE, "Page[%d]      : 0x%x\n", (UINT32)Index, QtdHw->Page[Index]));
  }
}


/**
  Dump the queue head.

  @param  Qh       The queue head to dump.
  @param  Msg      The message to print before the dump.
  @param  DumpBuf  Whether to dump the memory buffer of the associated QTD.

**/
VOID
EhcDumpQh (
  IN EHC_QH               *Qh,
  IN CHAR8                *Msg,
  IN BOOLEAN              DumpBuf
  )
{
  EHC_QTD                 *Qtd;
  QH_HW                   *QhHw;
  LIST_ENTRY              *Entry;
  UINTN                   Index;

  if (Msg != NULL) {
    DEBUG ((EFI_D_VERBOSE, Msg));
  }

  DEBUG ((EFI_D_VERBOSE, "Queue head @ 0x%p, interval %ld, next qh %p\n",
                                Qh, (UINT64)Qh->Interval, Qh->NextQh));

  QhHw = &Qh->QhHw;

  DEBUG ((EFI_D_VERBOSE, "Hoziontal link: %x\n", QhHw->HorizonLink));
  DEBUG ((EFI_D_VERBOSE, "Device address: %d\n", QhHw->DeviceAddr));
  DEBUG ((EFI_D_VERBOSE, "Inactive      : %d\n", QhHw->Inactive));
  DEBUG ((EFI_D_VERBOSE, "EP number     : %d\n", QhHw->EpNum));
  DEBUG ((EFI_D_VERBOSE, "EP speed      : %d\n", QhHw->EpSpeed));
  DEBUG ((EFI_D_VERBOSE, "DT control    : %d\n", QhHw->DtCtrl));
  DEBUG ((EFI_D_VERBOSE, "Reclaim head  : %d\n", QhHw->ReclaimHead));
  DEBUG ((EFI_D_VERBOSE, "Max packet len: %d\n", QhHw->MaxPacketLen));
  DEBUG ((EFI_D_VERBOSE, "Ctrl EP       : %d\n", QhHw->CtrlEp));
  DEBUG ((EFI_D_VERBOSE, "Nak reload    : %d\n", QhHw->NakReload));

  DEBUG ((EFI_D_VERBOSE, "SMask         : %x\n", QhHw->SMask));
  DEBUG ((EFI_D_VERBOSE, "CMask         : %x\n", QhHw->CMask));
  DEBUG ((EFI_D_VERBOSE, "Hub address   : %d\n", QhHw->HubAddr));
  DEBUG ((EFI_D_VERBOSE, "Hub port      : %d\n", QhHw->PortNum));
  DEBUG ((EFI_D_VERBOSE, "Multiplier    : %d\n", QhHw->Multiplier));

  DEBUG ((EFI_D_VERBOSE, "Cur QTD       : %x\n", QhHw->CurQtd));

  DEBUG ((EFI_D_VERBOSE, "Next QTD      : %x\n", QhHw->NextQtd));
  DEBUG ((EFI_D_VERBOSE, "AltNext QTD   : %x\n", QhHw->AltQtd));
  DEBUG ((EFI_D_VERBOSE, "Status        : %x\n", QhHw->Status));

  EhcDumpStatus (QhHw->Status);

  if (QhHw->Pid == QTD_PID_SETUP) {
    DEBUG ((EFI_D_VERBOSE, "PID           : Setup\n"));

  } else if (QhHw->Pid == QTD_PID_INPUT) {
    DEBUG ((EFI_D_VERBOSE, "PID           : IN\n"));

  } else if (QhHw->Pid == QTD_PID_OUTPUT) {
    DEBUG ((EFI_D_VERBOSE, "PID           : OUT\n"));
  }

  DEBUG ((EFI_D_VERBOSE, "Error Count   : %d\n", QhHw->ErrCnt));
  DEBUG ((EFI_D_VERBOSE, "Current Page  : %d\n", QhHw->CurPage));
  DEBUG ((EFI_D_VERBOSE, "IOC           : %d\n", QhHw->Ioc));
  DEBUG ((EFI_D_VERBOSE, "Total Bytes   : %d\n", QhHw->TotalBytes));
  DEBUG ((EFI_D_VERBOSE, "Data Toggle   : %d\n", QhHw->DataToggle));

  for (Index = 0; Index < 5; Index++) {
    DEBUG ((EFI_D_VERBOSE, "Page[%d]       : 0x%x\n", Index, QhHw->Page[Index]));
  }

  DEBUG ((EFI_D_VERBOSE, "\n"));

  EFI_LIST_FOR_EACH (Entry, &Qh->Qtds) {
    Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);
    EhcDumpQtd (Qtd, NULL);

    if (DumpBuf && (Qtd->DataLen != 0)) {
      EhcDumpBuf (Qtd->Data, Qtd->DataLen);
    }
  }
}


/**
  Dump the buffer in the form of hex.

  @param  Buf      The buffer to dump.
  @param  Len      The length of buffer.

**/
VOID
EhcDumpBuf (
  IN UINT8                *Buf,
  IN UINTN                Len
  )
{
  UINTN                   Index;

  for (Index = 0; Index < Len; Index++) {
    if (Index % 16 == 0) {
      DEBUG ((EFI_D_VERBOSE,"\n"));
    }

    DEBUG ((EFI_D_VERBOSE, "%02x ", Buf[Index]));
  }

  DEBUG ((EFI_D_VERBOSE, "\n"));
}

/**
  Dump the EHCI status registers.

  @param  Ehc    USB EHCI Host Controller instance

**/
VOID
EhcDumpRegs (
  IN  USB2_HC_DEV         *Ehc
  )
{
  UINT8   Index;

  DEBUG ((EFI_D_VERBOSE, "  EHC_CAPLENGTH_OFFSET   = 0x%08x\n", EhcReadCapRegister (Ehc, EHC_CAPLENGTH_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_HCSPARAMS_OFFSET   = 0x%08x\n", EhcReadCapRegister (Ehc, EHC_HCSPARAMS_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_HCCPARAMS_OFFSET   = 0x%08x\n", EhcReadCapRegister (Ehc, EHC_HCCPARAMS_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_USBCMD_OFFSET      = 0x%08x\n", EhcReadOpReg (Ehc, EHC_USBCMD_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_USBSTS_OFFSET      = 0x%08x\n", EhcReadOpReg (Ehc, EHC_USBSTS_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_USBINTR_OFFSET     = 0x%08x\n", EhcReadOpReg (Ehc, EHC_USBINTR_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_FRINDEX_OFFSET     = 0x%08x\n", EhcReadOpReg (Ehc, EHC_FRINDEX_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_CTRLDSSEG_OFFSET   = 0x%08x\n", EhcReadOpReg (Ehc,  EHC_CTRLDSSEG_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_FRAME_BASE_OFFSET  = 0x%08x\n", EhcReadOpReg (Ehc,  EHC_FRAME_BASE_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_ASYNC_HEAD_OFFSET  = 0x%08x\n", EhcReadOpReg (Ehc, EHC_ASYNC_HEAD_OFFSET)));
  DEBUG ((EFI_D_VERBOSE, "  EHC_CONFIG_FLAG_OFFSET = 0x%08x\n", EhcReadOpReg (Ehc, EHC_CONFIG_FLAG_OFFSET)));
  for (Index = 0; Index < (UINT8) (Ehc->HcStructParams & HCSP_NPORTS); Index++) {
    DEBUG ((EFI_D_VERBOSE, "  EHC_PORT_STAT_OFFSET(%d)  = 0x%08x\n", Index, EhcReadOpReg (Ehc, EHC_PORT_STAT_OFFSET + (4 * Index))));
  }
}
