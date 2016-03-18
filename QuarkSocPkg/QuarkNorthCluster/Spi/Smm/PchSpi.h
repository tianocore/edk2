/** @file
Header file for the PCH SPI SMM Driver.

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _PCH_SPI_H_
#define _PCH_SPI_H_

#include <Library/IoLib.h>
#include <Library/DebugLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/BaseLib.h>
#include <Protocol/Spi.h>
#include "SpiCommon.h"
#include <Library/SmmServicesTableLib.h>
#include <IntelQNCRegs.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>
#include <Library/TimerLib.h>

VOID
EFIAPI
SpiPhaseInit (
  VOID
  )
/*++
Routine Description:

  This function is a hook for Spi Smm phase specific initialization

Arguments:

  None

Returns:

  None

--*/
;
#endif
