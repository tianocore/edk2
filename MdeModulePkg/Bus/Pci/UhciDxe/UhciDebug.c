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
  IN UHCI_QH_SW    *QhSw
  )
{
  DEBUG ((EFI_D_VERBOSE, "&QhSw @ 0x%p\n", QhSw));
  DEBUG ((EFI_D_VERBOSE, "QhSw.NextQh    - 0x%p\n", QhSw->NextQh));
  DEBUG ((EFI_D_VERBOSE, "QhSw.TDs       - 0x%p\n", QhSw->TDs));
  DEBUG ((EFI_D_VERBOSE, "QhSw.QhHw:\n"));
  DEBUG ((EFI_D_VERBOSE, " Horizon  Link - %x\n", QhSw->QhHw.HorizonLink));
  DEBUG ((EFI_D_VERBOSE, " Vertical Link - %x\n\n", QhSw->QhHw.VerticalLink));
}


/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure.

**/
VOID
UhciDumpTds (
  IN UHCI_TD_SW           *TdSw
  )
{
  UHCI_TD_SW              *CurTdSw;

  CurTdSw = TdSw;

  while (CurTdSw != NULL) {
    DEBUG ((EFI_D_VERBOSE, "TdSw @ 0x%p\n",           CurTdSw));
    DEBUG ((EFI_D_VERBOSE, "TdSw.NextTd   - 0x%p\n",  CurTdSw->NextTd));
    DEBUG ((EFI_D_VERBOSE, "TdSw.DataLen  - %d\n",    CurTdSw->DataLen));
    DEBUG ((EFI_D_VERBOSE, "TdSw.Data     - 0x%p\n",  CurTdSw->Data));
    DEBUG ((EFI_D_VERBOSE, "TdHw:\n"));
    DEBUG ((EFI_D_VERBOSE, " NextLink     - 0x%x\n",  CurTdSw->TdHw.NextLink));
    DEBUG ((EFI_D_VERBOSE, " ActualLen    - %d\n",    CurTdSw->TdHw.ActualLen));
    DEBUG ((EFI_D_VERBOSE, " Status       - 0x%x\n",  CurTdSw->TdHw.Status));
    DEBUG ((EFI_D_VERBOSE, " IOC          - %d\n",    CurTdSw->TdHw.IntOnCpl));
    DEBUG ((EFI_D_VERBOSE, " IsIsoCh      - %d\n",    CurTdSw->TdHw.IsIsoch));
    DEBUG ((EFI_D_VERBOSE, " LowSpeed     - %d\n",    CurTdSw->TdHw.LowSpeed));
    DEBUG ((EFI_D_VERBOSE, " ErrorCount   - %d\n",    CurTdSw->TdHw.ErrorCount));
    DEBUG ((EFI_D_VERBOSE, " ShortPacket  - %d\n",    CurTdSw->TdHw.ShortPacket));
    DEBUG ((EFI_D_VERBOSE, " PidCode      - 0x%x\n",  CurTdSw->TdHw.PidCode));
    DEBUG ((EFI_D_VERBOSE, " DevAddr      - %d\n",    CurTdSw->TdHw.DeviceAddr));
    DEBUG ((EFI_D_VERBOSE, " EndPoint     - %d\n",    CurTdSw->TdHw.EndPoint));
    DEBUG ((EFI_D_VERBOSE, " DataToggle   - %d\n",    CurTdSw->TdHw.DataToggle));
    DEBUG ((EFI_D_VERBOSE, " MaxPacketLen - %d\n",    CurTdSw->TdHw.MaxPacketLen));
    DEBUG ((EFI_D_VERBOSE, " DataBuffer   - 0x%x\n\n",CurTdSw->TdHw.DataBuffer));

    CurTdSw = CurTdSw->NextTd;
  }
}

