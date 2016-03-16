/** @file

  Copyright (c) 2016, Dawid Ciecierski

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __UTIL_H__
#define __UTIL_H__


/**
  -----------------------------------------------------------------------------
  Constants.
  -----------------------------------------------------------------------------
**/

#define	DEBUG_MESSAGE_LENGTH	0x100



/**
  -----------------------------------------------------------------------------
  Includes.
  -----------------------------------------------------------------------------
**/

#include <Uefi.h>


/**
  -----------------------------------------------------------------------------
  Exported method signatures.
  -----------------------------------------------------------------------------
**/

VOID
StrToLowercase(
	IN			CHAR16	*String);

VOID
EFIAPI
PrintFuncNameMessage(
	IN CONST	BOOLEAN	IsError,
	IN CONST	CHAR8	*FuncName,
	IN CONST	CHAR16	*FormatString,
	...);

#define	PrintDebug(Format, ...) \
		PrintFuncNameMessage(FALSE, __FUNCTION__, Format, ##__VA_ARGS__)

#define	PrintError(Format, ...) \
		PrintFuncNameMessage(TRUE, __FUNCTION__, Format, ##__VA_ARGS__)


#endif