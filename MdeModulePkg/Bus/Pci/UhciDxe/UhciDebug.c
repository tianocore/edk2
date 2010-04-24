/** @file

  This file provides the information dump support for Uhci when in debug mode.

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  DEBUG ((EFI_D_INFO, "&QhSw @ 0x%p\n", QhSw));
  DEBUG ((EFI_D_INFO, "QhSw.NextQh    - 0x%p\n", QhSw->NextQh));
  DEBUG ((EFI_D_INFO, "QhSw.TDs       - 0x%p\n", QhSw->TDs));
  DEBUG ((EFI_D_INFO, "QhSw.QhHw:\n"));
  DEBUG ((EFI_D_INFO, " Horizon  Link - %x\n", QhSw->QhHw.HorizonLink));
  DEBUG ((EFI_D_INFO, " Vertical Link - %x\n\n", QhSw->QhHw.VerticalLink));
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
    DEBUG ((EFI_D_INFO, "TdSw @ 0x%p\n",           CurTdSw));
    DEBUG ((EFI_D_INFO, "TdSw.NextTd   - 0x%p\n",  CurTdSw->NextTd));
    DEBUG ((EFI_D_INFO, "TdSw.DataLen  - %d\n",    CurTdSw->DataLen));
    DEBUG ((EFI_D_INFO, "TdSw.Data     - 0x%p\n",  CurTdSw->Data));
    DEBUG ((EFI_D_INFO, "TdHw:\n"));
    DEBUG ((EFI_D_INFO, " NextLink     - 0x%x\n",  CurTdSw->TdHw.NextLink));
    DEBUG ((EFI_D_INFO, " ActualLen    - %d\n",    CurTdSw->TdHw.ActualLen));
    DEBUG ((EFI_D_INFO, " Status       - 0x%x\n",  CurTdSw->TdHw.Status));
    DEBUG ((EFI_D_INFO, " IOC          - %d\n",    CurTdSw->TdHw.IntOnCpl));
    DEBUG ((EFI_D_INFO, " IsIsoCh      - %d\n",    CurTdSw->TdHw.IsIsoch));
    DEBUG ((EFI_D_INFO, " LowSpeed     - %d\n",    CurTdSw->TdHw.LowSpeed));
    DEBUG ((EFI_D_INFO, " ErrorCount   - %d\n",    CurTdSw->TdHw.ErrorCount));
    DEBUG ((EFI_D_INFO, " ShortPacket  - %d\n",    CurTdSw->TdHw.ShortPacket));
    DEBUG ((EFI_D_INFO, " PidCode      - 0x%x\n",  CurTdSw->TdHw.PidCode));
    DEBUG ((EFI_D_INFO, " DevAddr      - %d\n",    CurTdSw->TdHw.DeviceAddr));
    DEBUG ((EFI_D_INFO, " EndPoint     - %d\n",    CurTdSw->TdHw.EndPoint));
    DEBUG ((EFI_D_INFO, " DataToggle   - %d\n",    CurTdSw->TdHw.DataToggle));
    DEBUG ((EFI_D_INFO, " MaxPacketLen - %d\n",    CurTdSw->TdHw.MaxPacketLen));
    DEBUG ((EFI_D_INFO, " DataBuffer   - 0x%x\n\n",CurTdSw->TdHw.DataBuffer));

    CurTdSw = CurTdSw->NextTd;
  }
}

