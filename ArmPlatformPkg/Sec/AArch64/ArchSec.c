/** @file
  Architecture specific handling of CPU exceptions taken while running in PEI.

  Copyright (c) 2012-2013, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include "Sec.h"

/**
  Minimal high level handling of exceptions occurring in PEI.

  @param[in]  Entry   Type of exception
  @param[in]  LR      Address of instruction where the exception was taken
**/
VOID
PeiCommonExceptionEntry (
  IN UINT32  Entry,
  IN UINTN   LR
  )
{
  CHAR8  Buffer[100];
  UINTN  CharCount;

  switch (Entry) {
    case EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS:
      CharCount = AsciiSPrint (Buffer, sizeof (Buffer), "Synchronous Exception at 0x%X\n\r", LR);
      break;
    case EXCEPT_AARCH64_IRQ:
      CharCount = AsciiSPrint (Buffer, sizeof (Buffer), "IRQ Exception at 0x%X\n\r", LR);
      break;
    case EXCEPT_AARCH64_FIQ:
      CharCount = AsciiSPrint (Buffer, sizeof (Buffer), "FIQ Exception at 0x%X\n\r", LR);
      break;
    case EXCEPT_AARCH64_SERROR:
      CharCount = AsciiSPrint (Buffer, sizeof (Buffer), "SError/Abort Exception at 0x%X\n\r", LR);
      break;
    default:
      CharCount = AsciiSPrint (Buffer, sizeof (Buffer), "Unknown Exception at 0x%X\n\r", LR);
      break;
  }

  SerialPortWrite ((UINT8 *)Buffer, CharCount);

  while (1) {
  }
}
