/** @file
*  Main file supporting the transition to PEI Core in Normal World for Versatile Express
*
*  Copyright (c) 2012, ARM Limited. All rights reserved.
*
*  This program and the accompanying materials
*  are licensed and made available under the terms and conditions of the BSD License
*  which accompanies this distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
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
  case 0:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Reset Exception at 0x%X\n\r",LR);
    break;
  case 1:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Undefined Exception at 0x%X\n\r",LR);
    break;
  case 2:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"SWI Exception at 0x%X\n\r",LR);
    break;
  case 3:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"PrefetchAbort Exception at 0x%X\n\r",LR);
    break;
  case 4:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"DataAbort Exception at 0x%X\n\r",LR);
    break;
  case 5:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Reserved Exception at 0x%X\n\r",LR);
    break;
  case 6:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"IRQ Exception at 0x%X\n\r",LR);
    break;
  case 7:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"FIQ Exception at 0x%X\n\r",LR);
    break;
  default:
    CharCount = AsciiSPrint (Buffer,sizeof (Buffer),"Unknown Exception at 0x%X\n\r",LR);
    break;
  }
  SerialPortWrite ((UINT8 *) Buffer, CharCount);
  while(1);
}

