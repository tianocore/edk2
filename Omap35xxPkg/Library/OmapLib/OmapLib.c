/** @file

  Copyright (c) 2008 - 2009, Apple Inc. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <Base.h>
#include <Library/DebugLib.h>
#include <Library/OmapLib.h>
#include <Omap3530/Omap3530.h>

UINT32
GpioBase (
  IN  UINTN Port
  )
{
  switch (Port) {
  case 1:  return GPIO1_BASE;
  case 2:  return GPIO2_BASE;
  case 3:  return GPIO3_BASE;
  case 4:  return GPIO4_BASE;
  case 5:  return GPIO5_BASE;
  case 6:  return GPIO6_BASE;
  default: ASSERT(FALSE); return 0;
  }
}

UINT32
TimerBase (
  IN  UINTN Timer
  )
{
  switch (Timer) {
  case  1: return GPTIMER1_BASE;
  case  2: return GPTIMER2_BASE;
  case  3: return GPTIMER3_BASE;
  case  4: return GPTIMER4_BASE;
  case  5: return GPTIMER5_BASE;
  case  6: return GPTIMER6_BASE;
  case  7: return GPTIMER7_BASE;
  case  8: return GPTIMER8_BASE;
  case  9: return GPTIMER9_BASE;
  case 10: return GPTIMER10_BASE;
  case 11: return GPTIMER11_BASE;
  case 12: return GPTIMER12_BASE;
  default: return 0;
  }
}

UINTN
InterruptVectorForTimer (
  IN  UINTN Timer
  )
{
  if ((Timer < 1) || (Timer > 12)) {
    ASSERT(FALSE);
    return 0xFFFFFFFF;
  }

  return 36 + Timer;
}

UINT32
UartBase (
  IN  UINTN Uart
  )
{
  switch (Uart) {
  case 1:  return UART1_BASE;
  case 2:  return UART2_BASE;
  case 3:  return UART3_BASE;
  default: ASSERT(FALSE); return 0;
  }
}

