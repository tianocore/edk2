/** @file
*
*  Copyright 2018 NXP
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

#include <Base.h>

/**
  Return clock in for PL011 Uart IP

  @return Pcd PL011UartClkInHz
**/
UINT32
EFIAPI
PL011UartClockGetFreq (
  VOID
  )
{
  return FixedPcdGet32 (PL011UartClkInHz);
}
