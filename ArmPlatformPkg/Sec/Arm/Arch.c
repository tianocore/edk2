/** @file
*
*  Copyright (c) 2013, ARM Limited. All rights reserved.
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

#include <Chipset/ArmV7.h>

VOID
EFIAPI
ArmSecArchTrustzoneInit (
  VOID
  )
{
  // Write to CP15 Non-secure Access Control Register
  ArmWriteNsacr (PcdGet32 (PcdArmNsacr));
}
