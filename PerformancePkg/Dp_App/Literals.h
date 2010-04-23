/** @file
  * Declarations of ASCII string literals used by DP.
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
#ifndef _LITERALS_H_
#define _LITERALS_H_

// ASCII String literals which probably don't need translation
extern CHAR8 const ALit_TimerLibError[];
extern CHAR8 const ALit_SEC[];
extern CHAR8 const ALit_DXE[];
extern CHAR8 const ALit_SHELL[];
extern CHAR8 const ALit_PEI[];
extern CHAR8 const ALit_BDS[];
extern CHAR8 const ALit_BdsTO[];
extern CHAR8 const ALit_PEIM[];

/// UNICODE String literals which should probably be translated
extern CHAR16  STR_DP_OPTION_UA[];
extern CHAR16  STR_DP_OPTION_LA[];
extern CHAR16  STR_DP_OPTION_LN[];
extern CHAR16  STR_DP_OPTION_LT[];
extern CHAR16  STR_DP_OPTION_UP[];
extern CHAR16  STR_DP_OPTION_UR[];
extern CHAR16  STR_DP_OPTION_LS[];
extern CHAR16  STR_DP_OPTION_US[];
extern CHAR16  STR_DP_OPTION_UT[];
extern CHAR16  STR_DP_OPTION_LV[];
extern CHAR16  STR_DP_OPTION_QH[];
extern CHAR16  STR_DP_OPTION_LH[];
extern CHAR16  STR_DP_OPTION_UH[];
extern CHAR16  STR_DP_OPTION_LX[];

extern CHAR16 const ALit_UNKNOWN[];
extern CHAR16 const STR_DP_INCOMPLETE[];
extern CHAR16 const STR_DP_COMPLETE[];

extern CHAR8 const ALit_TRUE[];
extern CHAR8 const ALit_FALSE[];

#endif  // _LITERALS_H_
