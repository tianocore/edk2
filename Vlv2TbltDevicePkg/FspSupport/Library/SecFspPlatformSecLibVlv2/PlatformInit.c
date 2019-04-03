/** @file
  This PEIM will parse the hoblist from fsp and report them into pei core.
  This file contains the main entrypoint of the PEIM.

  Copyright (c) 2014, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent

**/


#include <PiPei.h>
#include <Library/DebugLib.h>
#include <Library/SerialPortLib.h>

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
