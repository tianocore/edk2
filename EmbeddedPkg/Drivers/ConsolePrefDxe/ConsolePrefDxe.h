/** @file
*
*  Copyright (c) 2017, Linaro Limited. All rights reserved.
*
*  SPDX-License-Identifier: BSD-2-Clause-Patent
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
