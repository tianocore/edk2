/** @file

Copyright (c) 2007, Intel Corporation
All rights reserved. This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

Module Name:

  EhciDebug.h

Abstract:

  This file contains the definination for host controller debug support routines

Revision History

**/

#ifndef _EFI_EHCI_DEBUG_H_
#define _EFI_EHCI_DEBUG_H_


/**
  Dump the fields of a QTD

  @param  Qtd      The QTD to dump
  @param  Msg      The message to print before the dump

  @return None

**/
VOID
EhcDumpQtd (
  IN EHC_QTD              *Qtd,
  IN CHAR8                *Msg
  )
;



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
  IN CHAR8                *Msg,
  IN BOOLEAN              DumpBuf
  )
;



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
;

#endif
