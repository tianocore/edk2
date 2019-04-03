/** @file
Header file for the PCH SPI SMM Driver.

Copyright (c) 2013-2015 Intel Corporation.

SPDX-License-Identifier: BSD-2-Clause-Patent

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
