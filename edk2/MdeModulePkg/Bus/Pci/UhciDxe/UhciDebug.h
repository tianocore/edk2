/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  UhciDebug.h

Abstract:

  This file contains the definination for host controller debug support routines

Revision History


**/

#ifndef _EFI_UHCI_DEBUG_H_
#define _EFI_UHCI_DEBUG_H_

//
// DEBUG support
//
#define USB_DEBUG_FORCE_OUTPUT  (UINTN) (1 << 0)
#define UHCI_DEBUG_QH           (UINTN) (1 << 2)
#define UHCI_DEBUG_TD           (UINTN) (1 << 3)

VOID
UhciDebugPrint (
  IN  UINTN               Level,
  IN  CHAR8               *Format,
  ...
  )
/*++

Routine Description:

  Debug print interface for UHCI

Arguments:

  Level   - Level to control debug print
  Format  - String to use for the print, followed by print arguments

Returns:

  None

--*/
;


/**
  Debug print interface for UHCI

  @param  Format  String to use for the print, followed by print arguments

  @return None

**/
VOID
UhciDebug (
  IN  CHAR8               *Format,
  ...
  )
;


/**
  Debug error print interface for UHCI

  @param  Format  String to use for the print, followed by print arguments

  @return None

**/
VOID
UhciError (
  IN  CHAR8               *Format,
  ...
  )
;


/**
  Dump the content of QH structure

  @param  QhSw    Pointer to software QH structure

  @return None

**/
VOID
UhciDumpQh (
  IN UHCI_QH_SW         *QhSw
  )
;


/**
  Dump the content of TD structure.

  @param  TdSw    Pointer to software TD structure

  @return None

**/
VOID
UhciDumpTds (
  IN UHCI_TD_SW           *TdSw
  )
;


#ifdef EFI_DEBUG
  #define UHCI_DEBUG(arg)             UhciDebug arg
  #define UHCI_ERROR(arg)             UhciError arg
  #define UHCI_DUMP_TDS(arg)          UhciDumpTds arg
  #define UHCI_DUMP_QH(arg)           UhciDumpQh arg
#else
  #define UHCI_DEBUG(arg)
  #define UHCI_ERROR(arg)
  #define UHCI_DUMP_TDS(arg)
  #define UHCI_DUMP_QH(arg)
#endif

#endif
