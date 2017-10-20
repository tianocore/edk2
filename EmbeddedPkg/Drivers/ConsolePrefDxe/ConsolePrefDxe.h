/** @file
*
*  Copyright (c) 2017, Linaro Limited. All rights reserved.
*
*  This program and the accompanying materials are licensed and made available
*  under the terms and conditions of the BSD License which accompanies this
*  distribution.  The full text of the license may be found at
*  http://opensource.org/licenses/bsd-license.php
*
*  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
*  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.
*
**/

#ifndef __CONSOLE_PREF_DXE_H__
#define __CONSOLE_PREF_DXE_H__

#include <Guid/HiiPlatformSetupFormset.h>
#include <Guid/ConsolePrefFormSet.h>

#define CONSOLE_PREF_GRAPHICAL      0x0
#define CONSOLE_PREF_SERIAL         0x1

#define CONSOLE_PREF_VARIABLE_NAME  L"ConsolePref"

typedef struct {
  UINT8         Console;
  UINT8         Reserved[3];
} CONSOLE_PREF_VARSTORE_DATA;

#endif
