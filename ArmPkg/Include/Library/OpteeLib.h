/** @file
  OP-TEE specific header file.

  Copyright (c) 2018, Linaro Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _OPTEE_H_
#define _OPTEE_H_

/*
 * The 'Trusted OS Call UID' is supposed to return the following UUID for
 * OP-TEE OS. This is a 128-bit value.
 */
#define OPTEE_OS_UID0          0x384fb3e0
#define OPTEE_OS_UID1          0xe7f811e3
#define OPTEE_OS_UID2          0xaf630002
#define OPTEE_OS_UID3          0xa5d5c51b

BOOLEAN
EFIAPI
IsOpteePresent (
  VOID
  );

#endif
