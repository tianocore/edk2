/** @file
*
*  Copyright (c) 2011 - 2013, ARM Limited. All rights reserved.
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

#include <PiDxe.h>
#include <Library/UefiLib.h>
#include <Library/ArmLib.h>
#include <Chipset/AArch64.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/EblCmdLib.h>
#include <Library/BaseLib.h>
#include <Library/DebugLib.h>

EFI_STATUS
EblDumpMmu (
  IN UINTN  Argc,
  IN CHAR8  **Argv
  )
{
  AsciiPrint ("\nNot supported on this platform.\n");

  return EFI_SUCCESS;
}
