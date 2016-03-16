/** @file

  Copyright (c) 2016, Dawid Ciecierski

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include "Util.h"

VOID
StrToLowercase(
	IN	CHAR16	*String)
{
	CHAR16      *TmpStr;

	for (TmpStr = String; *TmpStr != L'\0'; TmpStr++) {
		if (*TmpStr >= L'A' && *TmpStr <= L'Z') {
			*TmpStr = (CHAR16)(*TmpStr - L'A' + L'a');
		}
	}
}