/** @file

  Copyright (c) 2011-2014, ARM Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Base.h>

#include <Library/ArmPlatformSysConfigLib.h>


RETURN_STATUS
ArmPlatformSysConfigInitialize (
  VOID
  )
{
  return RETURN_SUCCESS;
}

/***************************************
 * GENERAL FUNCTION: AccessSysCfgRegister
 * Interacts with
 *    SYS_CFGSTAT
 *    SYS_CFGDATA
 *    SYS_CFGCTRL
 * for setting and for reading out values
 ***************************************/
RETURN_STATUS
AccessSysCfgRegister (
  IN     UINT32   ReadWrite,
  IN     UINT32   Function,
  IN     UINT32   Site,
  IN     UINT32   Position,
  IN     UINT32   Device,
  IN OUT UINT32*  Data
  )
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
ArmPlatformSysConfigGet (
  IN  SYS_CONFIG_FUNCTION   Function,
  OUT UINT32*               Value
  )
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
ArmPlatformSysConfigGetValues (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINTN                 Size,
  OUT UINT32*               Values
  )
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
ArmPlatformSysConfigSet (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Value
  )
{
  return RETURN_UNSUPPORTED;
}

RETURN_STATUS
ArmPlatformSysConfigSetDevice (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Device,
  IN  UINT32                Value
  )
{
  return RETURN_UNSUPPORTED;
}
