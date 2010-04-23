/** @file
  * Definitions of ASCII string literals used by DP.
  *
  * Copyright (c) 2009-2010, Intel Corporation. All rights reserved.<BR>
  * This program and the accompanying materials
  * are licensed and made available under the terms and conditions of the BSD License
  * which accompanies this distribution.  The full text of the license may be found at
  * http://opensource.org/licenses/bsd-license.php
  *
  * THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  * WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
**/
#include <PerformanceTokens.h>

// ASCII String literals which probably don't need translation
CHAR8 const ALit_TimerLibError[] = "Timer library instance error!\n";
CHAR8 const ALit_SEC[]    = SEC_TOK;
CHAR8 const ALit_DXE[]    = DXE_TOK;
CHAR8 const ALit_SHELL[]  = SHELL_TOK;
CHAR8 const ALit_PEI[]    = PEI_TOK;
CHAR8 const ALit_BDS[]    = BDS_TOK;
CHAR8 const ALit_BdsTO[]  = "BdsTimeOut";
CHAR8 const ALit_PEIM[]   = "PEIM";

/// UNICODE String literals which should probably be translated
CHAR16  STR_DP_OPTION_UA[]   = L"-A";
CHAR16  STR_DP_OPTION_LA[]   = L"-a";
CHAR16  STR_DP_OPTION_LN[]   = L"-n";
CHAR16  STR_DP_OPTION_LT[]   = L"-t";
CHAR16  STR_DP_OPTION_UP[]   = L"-P";
CHAR16  STR_DP_OPTION_UR[]   = L"-R";
CHAR16  STR_DP_OPTION_LS[]   = L"-s";
CHAR16  STR_DP_OPTION_US[]   = L"-S";
CHAR16  STR_DP_OPTION_UT[]   = L"-T";
CHAR16  STR_DP_OPTION_LV[]   = L"-v";
CHAR16  STR_DP_OPTION_QH[]   = L"-?";
CHAR16  STR_DP_OPTION_LH[]   = L"-h";
CHAR16  STR_DP_OPTION_UH[]   = L"-H";
CHAR16  STR_DP_OPTION_LX[]   = L"-x";

CHAR16 const ALit_UNKNOWN[]       = L"Unknown";
CHAR16 const STR_DP_INCOMPLETE[]  = L" I ";
CHAR16 const STR_DP_COMPLETE[]    = L"   ";

CHAR8 const ALit_TRUE[]   = "TRUE";
CHAR8 const ALit_FALSE[]  = "FALSE";
