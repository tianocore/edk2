/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

    EhciDebug.c

Abstract:
  This file provides the information dump support for EHCI when in debug mode.
You can dynamically adjust the debug level by changing variable mEhcDebugLevel
and mEhcErrorLevel.

Revision History

**/


#include "Ehci.h"

#ifdef EFI_DEBUG
UINTN mEhcDebugMask   = USB_DEBUG_FORCE_OUTPUT;


/**
  EHCI's debug output function. It determines whether
  to output by the mask and level

  @param  Level    The output level
  @param  Format   The format parameters to the print
  @param  ...      The variable length parameters after format

  @return None

**/
VOID
EhciDebugPrint (
  IN  UINTN               Level,
  IN  CHAR8               *Format,
  ...
  )
{

  VA_LIST                 Marker;

  VA_START (Marker, Format);

  if (Level & mEhcDebugMask) {
    if (mEhcDebugMask & USB_DEBUG_FORCE_OUTPUT) {
      DebugVPrint (DEBUG_ERROR, Format, Marker);
    } else {
      DebugVPrint (DEBUG_INFO, Format, Marker);
    }
  }

  VA_END (Marker);
}


/**
  EHCI's debug output function. It determines whether
  to output by the mask and level

  @param  Format   The format parameters to the print
  @param  ...      The variable length parameters after format

  @return None

**/
VOID
EhcDebug (
  IN  CHAR8               *Format,
  ...
  )
{
  VA_LIST                 Marker;

  VA_START (Marker, Format);
  DebugVPrint (DEBUG_INFO, Format, Marker);
  VA_END (Marker);
}


/**
  EHCI's error output function. It determines whether
  to output by the mask and level

  @param  Format   The format parameters to the print
  @param  ...      The variable length parameters after format

  @return None

**/
VOID
EhcError (
  IN  CHAR8               *Format,
  ...
  )
{

  VA_LIST                 Marker;

  VA_START (Marker, Format);
  DebugVPrint (DEBUG_ERROR, Format, Marker);
  VA_END (Marker);
}


/**
  Dump the status byte in QTD/QH to a more friendly
  format

  @param  State    The state in the QTD/QH
  @param  Level    The output level

  @return None

**/
STATIC
VOID
EhcDumpStatus (
  IN UINT32               State,
  IN UINTN                Level
  )
{
  if (EHC_BIT_IS_SET (State, QTD_STAT_DO_PING)) {
    EhciDebugPrint (Level, "  Do_Ping");
  } else {
    EhciDebugPrint (Level, "  Do_Out");
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_DO_CS)) {
    EhciDebugPrint (Level, "  Do_CS");
  } else {
    EhciDebugPrint (Level, "  Do_SS");
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_TRANS_ERR)) {
    EhciDebugPrint (Level, "  Transfer_Error");
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_BABBLE_ERR)) {
    EhciDebugPrint (Level, "  Babble_Error");
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_BUFF_ERR)) {
    EhciDebugPrint (Level, "  Buffer_Error");
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_HALTED)) {
    EhciDebugPrint (Level, "  Halted");
  }

  if (EHC_BIT_IS_SET (State, QTD_STAT_ACTIVE)) {
    EhciDebugPrint (Level, "  Active");
  }

  EhciDebugPrint (Level, "\n");
}


/**
  Dump the fields of a QTD

  @param  Qtd      The QTD to dump
  @param  Msg      The message to print before the dump

  @return None

**/
VOID
EhcDumpQtd (
  IN EHC_QTD              *Qtd,
  IN UINT8                *Msg
  )
{
  QTD_HW                  *QtdHw;
  UINTN                   Index;
  UINTN                   Level;

  Level = EHC_DEBUG_QTD;

  if (Msg != NULL) {
    EhciDebugPrint (Level, Msg);
  }

  EhciDebugPrint (Level, "Queue TD @ 0x%x, data length %d\n", Qtd, Qtd->DataLen);

  QtdHw = &Qtd->QtdHw;

  EhciDebugPrint (Level, "Next QTD     : %x\n", QtdHw->NextQtd);
  EhciDebugPrint (Level, "AltNext QTD  : %x\n", QtdHw->AltNext);
  EhciDebugPrint (Level, "Status       : %x\n", QtdHw->Status);
  EhcDumpStatus (QtdHw->Status, Level);

  if (QtdHw->Pid == QTD_PID_SETUP) {
    EhciDebugPrint (Level, "PID          : Setup\n");

  } else if (QtdHw->Pid == QTD_PID_INPUT) {
    EhciDebugPrint (Level, "PID          : IN\n");

  } else if (QtdHw->Pid == QTD_PID_OUTPUT) {
    EhciDebugPrint (Level, "PID          : OUT\n");

  }

  EhciDebugPrint (Level, "Error Count  : %d\n", QtdHw->ErrCnt);
  EhciDebugPrint (Level, "Current Page : %d\n", QtdHw->CurPage);
  EhciDebugPrint (Level, "IOC          : %d\n", QtdHw->IOC);
  EhciDebugPrint (Level, "Total Bytes  : %d\n", QtdHw->TotalBytes);
  EhciDebugPrint (Level, "Data Toggle  : %d\n", QtdHw->DataToggle);

  for (Index = 0; Index < 5; Index++) {
    EhciDebugPrint (Level, "Page[%d]      : 0x%x\n", Index, QtdHw->Page[Index]);
  }
}


/**
  Dump the queue head

  @param  Qh       The queue head to dump
  @param  Msg      The message to print before the dump
  @param  DumpBuf  Whether to dump the memory buffer of the associated QTD

  @return None

**/
VOID
EhcDumpQh (
  IN EHC_QH               *Qh,
  IN UINT8                *Msg,
  IN BOOLEAN              DumpBuf
  )
{
  EHC_QTD                 *Qtd;
  QH_HW                   *QhHw;
  LIST_ENTRY              *Entry;
  UINTN                   Index;
  UINTN                   Level;

  Level = EHC_DEBUG_QH;

  if (Msg != NULL) {
    EhciDebugPrint (Level, Msg);
  }

  EhciDebugPrint (Level, "Queue head @ 0x%x, interval %d, next qh %x\n",
                                Qh, Qh->Interval, Qh->NextQh);

  QhHw = &Qh->QhHw;

  EhciDebugPrint (Level, "Hoziontal link: %x\n", QhHw->HorizonLink);
  EhciDebugPrint (Level, "Device address: %d\n", QhHw->DeviceAddr);
  EhciDebugPrint (Level, "Inactive      : %d\n", QhHw->Inactive);
  EhciDebugPrint (Level, "EP number     : %d\n", QhHw->EpNum);
  EhciDebugPrint (Level, "EP speed      : %d\n", QhHw->EpSpeed);
  EhciDebugPrint (Level, "DT control    : %d\n", QhHw->DtCtrl);
  EhciDebugPrint (Level, "Reclaim head  : %d\n", QhHw->ReclaimHead);
  EhciDebugPrint (Level, "Max packet len: %d\n", QhHw->MaxPacketLen);
  EhciDebugPrint (Level, "Ctrl EP       : %d\n", QhHw->CtrlEp);
  EhciDebugPrint (Level, "Nak reload    : %d\n", QhHw->NakReload);

  EhciDebugPrint (Level, "SMask         : %x\n", QhHw->SMask);
  EhciDebugPrint (Level, "CMask         : %x\n", QhHw->CMask);
  EhciDebugPrint (Level, "Hub address   : %d\n", QhHw->HubAddr);
  EhciDebugPrint (Level, "Hub port      : %d\n", QhHw->PortNum);
  EhciDebugPrint (Level, "Multiplier    : %d\n", QhHw->Multiplier);

  EhciDebugPrint (Level, "Cur QTD       : %x\n", QhHw->CurQtd);

  EhciDebugPrint (Level, "Next QTD      : %x\n", QhHw->NextQtd);
  EhciDebugPrint (Level, "AltNext QTD   : %x\n", QhHw->AltQtd);
  EhciDebugPrint (Level, "Status        : %x\n", QhHw->Status);
  EhcDumpStatus (QhHw->Status, Level);

  if (QhHw->Pid == QTD_PID_SETUP) {
    EhciDebugPrint (Level, "PID           : Setup\n");

  } else if (QhHw->Pid == QTD_PID_INPUT) {
    EhciDebugPrint (Level, "PID           : IN\n");

  } else if (QhHw->Pid == QTD_PID_OUTPUT) {
    EhciDebugPrint (Level, "PID           : OUT\n");
  }

  EhciDebugPrint (Level, "Error Count   : %d\n", QhHw->ErrCnt);
  EhciDebugPrint (Level, "Current Page  : %d\n", QhHw->CurPage);
  EhciDebugPrint (Level, "IOC           : %d\n", QhHw->IOC);
  EhciDebugPrint (Level, "Total Bytes   : %d\n", QhHw->TotalBytes);
  EhciDebugPrint (Level, "Data Toggle   : %d\n", QhHw->DataToggle);

  for (Index = 0; Index < 5; Index++) {
    EhciDebugPrint (Level, "Page[%d]       : 0x%x\n", Index, QhHw->Page[Index]);
  }

  EhciDebugPrint (Level, "\n");

  EFI_LIST_FOR_EACH (Entry, &Qh->Qtds) {
    Qtd = EFI_LIST_CONTAINER (Entry, EHC_QTD, QtdList);
    EhcDumpQtd (Qtd, NULL);

    if (DumpBuf && (Qtd->DataLen != 0)) {
      EhcDumpBuf (Qtd->Data, Qtd->DataLen);
    }
  }
}


/**
  Dump the buffer in the form of hex

  @param  Buf      The buffer to dump
  @param  Len      The length of buffer

  @return None

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
      EhciDebugPrint (EHC_DEBUG_BUF, "\n");
    }

    EhciDebugPrint (EHC_DEBUG_BUF, "%02x ", Buf[Index]);
  }

  EhciDebugPrint (EHC_DEBUG_BUF, "\n");
}

#endif
