/** @file

  This file contains the definination for host controller debug support routines

Copyright (c) 2007, Intel Corporation. All rights reserved.<BR>
This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

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
  IN UHCI_QH_SW         *QhSw
  );


/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure.

  @return None.

**/
VOID
UhciDumpTds (
  IN UHCI_TD_SW           *TdSw
  );

#endif
