/** @file
  Definitions of ASCII string literals used by DP.

  Copyright (c) 2009 - 2018, Intel Corporation. All rights reserved.<BR>
  SPDX-License-Identifier: BSD-2-Clause-Patent
**/
#include <Guid/ExtendedFirmwarePerformance.h>

// ASCII String literals which probably don't need translation
CHAR8 const  ALit_TimerLibError[] = "Timer library instance error!\n";
CHAR8 const  ALit_SEC[]           = SEC_TOK;
CHAR8 const  ALit_DXE[]           = DXE_TOK;
CHAR8 const  ALit_PEI[]           = PEI_TOK;
CHAR8 const  ALit_BDS[]           = BDS_TOK;
CHAR8 const  ALit_START_IMAGE[]   = START_IMAGE_TOK;
CHAR8 const  ALit_LOAD_IMAGE[]    = LOAD_IMAGE_TOK;
CHAR8 const  ALit_DB_START[]      = DRIVERBINDING_START_TOK;
CHAR8 const  ALit_DB_SUPPORT[]    = DRIVERBINDING_SUPPORT_TOK;
CHAR8 const  ALit_DB_STOP[]       = DRIVERBINDING_STOP_TOK;

CHAR8 const  ALit_BdsTO[] = "BdsTimeOut";
CHAR8 const  ALit_PEIM[]  = "PEIM";
