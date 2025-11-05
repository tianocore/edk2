/** @file

  This file provides the information dump support for EHCI when in debug mode.

Copyright (c) 2007 - 2013, Intel Corporation. All rights reserved.<BR>
Copyright (c) Microsoft Corporation.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Ehci.h"

/**
  Dump the status byte in QTD/QH to a more friendly format.

  @param  State    The state in the QTD/QH.

**/
VOID
EhcDumpStatus (
  IN UINT32  State
  )
{
  if (EHC_BIT_IS_SET (State, QTD_STAT_DO_PING)) {
    DEBUG ((DEBUG_VERBOSE, "  Do_Ping"));
  } else {
    DEBUG ((DEBUG_VERBOSE, "  Do_Out"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_DO_CS)) {
    DEBUG ((DEBUG_VERBOSE, "  Do_CS"));
  } else {
    DEBUG ((DEBUG_VERBOSE, "  Do_SS"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_TRANS_ERR)) {
    DEBUG ((DEBUG_VERBOSE, "  Transfer_Error"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_BABBLE_ERR)) {
    DEBUG ((DEBUG_VERBOSE, "  Babble_Error"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_BUFF_ERR)) {
    DEBUG ((DEBUG_VERBOSE, "  Buffer_Error"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_HALTED)) {
    DEBUG ((DEBUG_VERBOSE, "  Halted"));
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_ACTIVE)) {
    DEBUG ((DEBUG_VERBOSE, "  Active"));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}

/**
  Dump the fields of a QTD.

  @param  Qtd      The QTD to dump.
  @param  Msg      The message to print before the dump.

**/
VOID
EhcDumpQtd (
  IN EHC_QTD  *Qtd,
  IN CHAR8    *Msg
  )
{
  QTD_HW  *QtdHw;
  UINTN   Index;

  if (Msg != NULL) {
    DEBUG ((DEBUG_VERBOSE, Msg));
  }

  DEBUG ((DEBUG_VERBOSE, "Queue TD @ 0x%p, data length %d\n", Qtd, (UINT32)Qtd->DataLen));

  QtdHw = &Qtd->QtdHw;

  DEBUG ((DEBUG_VERBOSE, "Next QTD     : %x\n", QtdHw->NextQtd));
  DEBUG ((DEBUG_VERBOSE, "AltNext QTD  : %x\n", QtdHw->AltNext));
  DEBUG ((DEBUG_VERBOSE, "Status       : %x\n", QtdHw->Status));
  EhcDumpStatus (QtdHw->Status);

  if (QtdHw->Pid == QTD_PID_SETUP) {
    DEBUG ((DEBUG_VERBOSE, "PID          : Setup\n"));
  } else if (QtdHw->Pid == QTD_PID_INPUT) {
    DEBUG ((DEBUG_VERBOSE, "PID          : IN\n"));
  } else if (QtdHw->Pid == QTD_PID_OUTPUT) {
    DEBUG ((DEBUG_VERBOSE, "PID          : OUT\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "Error Count  : %d\n", QtdHw->ErrCnt));
  DEBUG ((DEBUG_VERBOSE, "Current Page : %d\n", QtdHw->CurPage));
  DEBUG ((DEBUG_VERBOSE, "IOC          : %d\n", QtdHw->Ioc));
  DEBUG ((DEBUG_VERBOSE, "Total Bytes  : %d\n", QtdHw->TotalBytes));
  DEBUG ((DEBUG_VERBOSE, "Data Toggle  : %d\n", QtdHw->DataToggle));

  for (Index = 0; Index < 5; Index++) {
    DEBUG ((DEBUG_VERBOSE, "Page[%d]      : 0x%x\n", (UINT32)Index, QtdHw->Page[Index]));
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
  IN EHC_QH   *Qh,
  IN CHAR8    *Msg,
  IN BOOLEAN  DumpBuf
  )
{
  EHC_QTD     *Qtd;
  QH_HW       *QhHw;
  LIST_ENTRY  *Entry;
  UINTN       Index;

  if (Msg != NULL) {
    DEBUG ((DEBUG_VERBOSE, Msg));
  }

  DEBUG ((
    DEBUG_VERBOSE,
    "Queue head @ 0x%p, interval %ld, next qh %p\n",
    Qh,
    (UINT64)Qh->Interval,
    Qh->NextQh
    ));

  QhHw = &Qh->QhHw;

  DEBUG ((DEBUG_VERBOSE, "Hoziontal link: %x\n", QhHw->HorizonLink));
  DEBUG ((DEBUG_VERBOSE, "Device address: %d\n", QhHw->DeviceAddr));
  DEBUG ((DEBUG_VERBOSE, "Inactive      : %d\n", QhHw->Inactive));
  DEBUG ((DEBUG_VERBOSE, "EP number     : %d\n", QhHw->EpNum));
  DEBUG ((DEBUG_VERBOSE, "EP speed      : %d\n", QhHw->EpSpeed));
  DEBUG ((DEBUG_VERBOSE, "DT control    : %d\n", QhHw->DtCtrl));
  DEBUG ((DEBUG_VERBOSE, "Reclaim head  : %d\n", QhHw->ReclaimHead));
  DEBUG ((DEBUG_VERBOSE, "Max packet len: %d\n", QhHw->MaxPacketLen));
  DEBUG ((DEBUG_VERBOSE, "Ctrl EP       : %d\n", QhHw->CtrlEp));
  DEBUG ((DEBUG_VERBOSE, "Nak reload    : %d\n", QhHw->NakReload));

  DEBUG ((DEBUG_VERBOSE, "SMask         : %x\n", QhHw->SMask));
  DEBUG ((DEBUG_VERBOSE, "CMask         : %x\n", QhHw->CMask));
  DEBUG ((DEBUG_VERBOSE, "Hub address   : %d\n", QhHw->HubAddr));
  DEBUG ((DEBUG_VERBOSE, "Hub port      : %d\n", QhHw->PortNum));
  DEBUG ((DEBUG_VERBOSE, "Multiplier    : %d\n", QhHw->Multiplier));

  DEBUG ((DEBUG_VERBOSE, "Cur QTD       : %x\n", QhHw->CurQtd));

  DEBUG ((DEBUG_VERBOSE, "Next QTD      : %x\n", QhHw->NextQtd));
  DEBUG ((DEBUG_VERBOSE, "AltNext QTD   : %x\n", QhHw->AltQtd));
  DEBUG ((DEBUG_VERBOSE, "Status        : %x\n", QhHw->Status));

  EhcDumpStatus (QhHw->Status);

  if (QhHw->Pid == QTD_PID_SETUP) {
    DEBUG ((DEBUG_VERBOSE, "PID           : Setup\n"));
  } else if (QhHw->Pid == QTD_PID_INPUT) {
    DEBUG ((DEBUG_VERBOSE, "PID           : IN\n"));
  } else if (QhHw->Pid == QTD_PID_OUTPUT) {
    DEBUG ((DEBUG_VERBOSE, "PID           : OUT\n"));
  }

  DEBUG ((DEBUG_VERBOSE, "Error Count   : %d\n", QhHw->ErrCnt));
  DEBUG ((DEBUG_VERBOSE, "Current Page  : %d\n", QhHw->CurPage));
  DEBUG ((DEBUG_VERBOSE, "IOC           : %d\n", QhHw->Ioc));
  DEBUG ((DEBUG_VERBOSE, "Total Bytes   : %d\n", QhHw->TotalBytes));
  DEBUG ((DEBUG_VERBOSE, "Data Toggle   : %d\n", QhHw->DataToggle));

  for (Index = 0; Index < 5; Index++) {
    DEBUG ((DEBUG_VERBOSE, "Page[%d]       : 0x%x\n", Index, QhHw->Page[Index]));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));

  BASE_LIST_FOR_EACH (Entry, &Qh->Qtds) {
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
  IN UINT8  *Buf,
  IN UINTN  Len
  )
{
  UINTN  Index;

  for (Index = 0; Index < Len; Index++) {
    if (Index % 16 == 0) {
      DEBUG ((DEBUG_VERBOSE, "\n"));
    }

    DEBUG ((DEBUG_VERBOSE, "%02x ", Buf[Index]));
  }

  DEBUG ((DEBUG_VERBOSE, "\n"));
}
