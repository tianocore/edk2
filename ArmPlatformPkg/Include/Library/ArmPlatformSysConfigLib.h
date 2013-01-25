/** @file  ArmPlatformSysConfigLib.h

  Copyright (c) 2011-2012, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef __ARM_PLATFORM_SYS_CONFIG_H__
#define __ARM_PLATFORM_SYS_CONFIG_H__

#include <Base.h>

/* This header file makes it easier to access the System Configuration Registers
 * in the ARM Versatile Express motherboard.
 */

//
// Typedef
//
typedef UINT32  SYS_CONFIG_FUNCTION;

//
// Functions
//
RETURN_STATUS
ArmPlatformSysConfigInitialize (
  VOID
  );

RETURN_STATUS
ArmPlatformSysConfigGet (
  IN  SYS_CONFIG_FUNCTION   Function,
  OUT UINT32*               Value
  );

RETURN_STATUS
ArmPlatformSysConfigGetValues (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINTN                 Size,
  OUT UINT32*               Values
  );

RETURN_STATUS
ArmPlatformSysConfigSet (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Value
  );

RETURN_STATUS
ArmPlatformSysConfigSetDevice (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Device,
  IN  UINT32                Value
  );

#endif /* __SYS_CFG_REGISTERS_H__ */
