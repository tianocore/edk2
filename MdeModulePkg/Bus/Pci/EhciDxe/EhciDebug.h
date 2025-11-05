/** @file

  This file contains the definination for host controller debug support routines.

Copyright (c) 2007 - 2009, Intel Corporation. All rights reserved.<BR>
SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _EFI_EHCI_DEBUG_H_
#define _EFI_EHCI_DEBUG_H_

/**
  Dump the fields of a QTD.

  @param  Qtd      The QTD to dump.
  @param  Msg      The message to print before the dump.

**/
VOID
EhcDumpQtd (
  IN EHC_QTD  *Qtd,
  IN CHAR8    *Msg
  );

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
  );

/**
  Dump the buffer in the form of hex.

  @param  Buf      The buffer to dump.
  @param  Len      The length of buffer.

**/
VOID
EhcDumpBuf (
  IN UINT8  *Buf,
  IN UINTN  Len
  );

#endif
