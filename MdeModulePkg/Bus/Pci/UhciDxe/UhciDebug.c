/** @file

  This file provides the information dump support for Uhci when in debug mode.

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Uhci.h"

/**
  Dump the content of QH structure.

  @param  QhSw    Pointer to software QH structure.

**/
VOID
UhciDumpQh (
  IN UHCI_QH_SW  *QhSw
  )
{
  DEBUG ((DEBUG_VERBOSE, "&QhSw @ 0x%p\n", QhSw));
  DEBUG ((DEBUG_VERBOSE, "QhSw.NextQh    - 0x%p\n", QhSw->NextQh));
  DEBUG ((DEBUG_VERBOSE, "QhSw.TDs       - 0x%p\n", QhSw->TDs));
  DEBUG ((DEBUG_VERBOSE, "QhSw.QhHw:\n"));
  DEBUG ((DEBUG_VERBOSE, " Horizon  Link - %x\n", QhSw->QhHw.HorizonLink));
  DEBUG ((DEBUG_VERBOSE, " Vertical Link - %x\n\n", QhSw->QhHw.VerticalLink));
}

/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure.

**/
VOID
UhciDumpTds (
  IN UHCI_TD_SW  *TdSw
  )
{
  UHCI_TD_SW  *CurTdSw;

  CurTdSw = TdSw;

  while (CurTdSw != NULL) {
    DEBUG ((DEBUG_VERBOSE, "TdSw @ 0x%p\n", CurTdSw));
    DEBUG ((DEBUG_VERBOSE, "TdSw.NextTd   - 0x%p\n", CurTdSw->NextTd));
    DEBUG ((DEBUG_VERBOSE, "TdSw.DataLen  - %d\n", CurTdSw->DataLen));
    DEBUG ((DEBUG_VERBOSE, "TdSw.Data     - 0x%p\n", CurTdSw->Data));
    DEBUG ((DEBUG_VERBOSE, "TdHw:\n"));
    DEBUG ((DEBUG_VERBOSE, " NextLink     - 0x%x\n", CurTdSw->TdHw.NextLink));
    DEBUG ((DEBUG_VERBOSE, " ActualLen    - %d\n", CurTdSw->TdHw.ActualLen));
    DEBUG ((DEBUG_VERBOSE, " Status       - 0x%x\n", CurTdSw->TdHw.Status));
    DEBUG ((DEBUG_VERBOSE, " IOC          - %d\n", CurTdSw->TdHw.IntOnCpl));
    DEBUG ((DEBUG_VERBOSE, " IsIsoCh      - %d\n", CurTdSw->TdHw.IsIsoch));
    DEBUG ((DEBUG_VERBOSE, " LowSpeed     - %d\n", CurTdSw->TdHw.LowSpeed));
    DEBUG ((DEBUG_VERBOSE, " ErrorCount   - %d\n", CurTdSw->TdHw.ErrorCount));
    DEBUG ((DEBUG_VERBOSE, " ShortPacket  - %d\n", CurTdSw->TdHw.ShortPacket));
    DEBUG ((DEBUG_VERBOSE, " PidCode      - 0x%x\n", CurTdSw->TdHw.PidCode));
    DEBUG ((DEBUG_VERBOSE, " DevAddr      - %d\n", CurTdSw->TdHw.DeviceAddr));
    DEBUG ((DEBUG_VERBOSE, " EndPoint     - %d\n", CurTdSw->TdHw.EndPoint));
    DEBUG ((DEBUG_VERBOSE, " DataToggle   - %d\n", CurTdSw->TdHw.DataToggle));
    DEBUG ((DEBUG_VERBOSE, " MaxPacketLen - %d\n", CurTdSw->TdHw.MaxPacketLen));
    DEBUG ((DEBUG_VERBOSE, " DataBuffer   - 0x%x\n\n", CurTdSw->TdHw.DataBuffer));

    CurTdSw = CurTdSw->NextTd;
  }
}
