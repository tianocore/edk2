/** @file
  GUID for Shell Variable for Get/Set via runtime services.

  Copyright (c) 2009 - 2010, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _SHELL_VARIABLE_GUID_H_
#define _SHELL_VARIABLE_GUID_H_

#define SHELL_VARIABLE_GUID \
{ \
  0x158def5a, 0xf656, 0x419c, { 0xb0, 0x27, 0x7a, 0x31, 0x92, 0xc0, 0x79, 0xd2 } \
}

extern EFI_GUID gShellVariableGuid;

#endif
