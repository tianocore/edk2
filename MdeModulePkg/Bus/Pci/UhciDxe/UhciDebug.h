/** @file

  This file contains the definination for host controller debug support routines

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_UHCI_DEBUG_H_
#define _EFI_UHCI_DEBUG_H_

/**
  Dump the content of QH structure.

  @param  QhSw    Pointer to software QH structure.

  @return None.

**/
VOID
UhciDumpQh (
  IN UHCI_QH_SW  *QhSw
  );

/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure.

  @return None.

**/
VOID
UhciDumpTds (
  IN UHCI_TD_SW  *TdSw
  );

#endif
