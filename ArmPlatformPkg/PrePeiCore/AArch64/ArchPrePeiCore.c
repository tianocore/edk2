/** @file
  Main file supporting the transition to PEI Core in Normal World for Versatile Express

  Copyright (c) 2012-2013, ARM Limited. All rights reserved.

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Library/PrintLib.h>
#include <Library/SerialPortLib.h>

#include "PrePeiCore.h"

VOID
PeiCommonExceptionEntry (
  IN UINT32 Entry,
  IN UINTN LR
  )
{
  CHAR8           Buffer[100];
  UINTN           CharCount;

  switch (Entry) {
  case EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Synchronous Exception at 0x%X\n\r", LR);
    break;
  case EXCEPT_AARCH64_IRQ:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"IRQ Exception at 0x%X\n\r", LR);
    break;
  case EXCEPT_AARCH64_FIQ:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"FIQ Exception at 0x%X\n\r", LR);
    break;
  case EXCEPT_AARCH64_SERROR:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"SError/Abort Exception at 0x%X\n\r", LR);
    break;
  default:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Unknown Exception at 0x%X\n\r", LR);
    break;
  }

  SerialPortWrite ((UINT8 *) Buffer, CharCount);

  while(1);
}

