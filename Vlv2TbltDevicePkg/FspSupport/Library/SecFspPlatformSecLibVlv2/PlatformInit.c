/** @file
  This PEIM will parse the hoblist from fsp and report them into pei core.
  This file contains the main entrypoint of the PEIM.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/


#include <PiPei.h>
#include <Library\DebugLib.h>
#include <Library\SerialPortLib.h>

VOID EnableInternalUart ();

VOID
EFIAPI
PlatformInit (
  IN VOID                 *FspHobList,
  IN VOID                 *StartOfRange,
  IN VOID                 *EndOfRange
  )
{
  //
  // Platform initialization
  // Enable Serial port here
  //
  EnableInternalUart ();
  SerialPortInitialize ();

  DEBUG ((DEBUG_INFO, "PlatformInit\n"));
  DEBUG ((DEBUG_INFO, "FspHobList - 0x%x\n", FspHobList));
  DEBUG ((DEBUG_INFO, "StartOfRange - 0x%x\n", StartOfRange));
  DEBUG ((DEBUG_INFO, "EndOfRange - 0x%x\n", EndOfRange));
}
