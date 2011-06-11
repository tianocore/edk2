/** @file  SysCfgArmPlatform.c

  Copyright (c) 2011, ARM Ltd. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/IoLib.h>
#include <Library/DebugLib.h>

#include <Library/ArmPlatformSysConfigLib.h>
#include <ArmPlatform.h>

/****************************************************************************
 *
 *  This file makes it easier to access the System Configuration Registers
 *  in the ARM Versatile Express motherboard.
 *
 ****************************************************************************/

EFI_STATUS
ArmPlatformSysConfigInitialize (
  VOID
  )
{
  return EFI_SUCCESS;
}

EFI_STATUS
ArmPlatformSysConfigGet (
  IN  SYS_CONFIG_FUNCTION   Function,
  OUT UINT32*               Value
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  // Intercept some functions
  switch(Function) {

  default:
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

EFI_STATUS
ArmPlatformSysConfigSet (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Value
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  // Intercept some functions
  switch(Function) {

  default:
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}

EFI_STATUS
ArmPlatformSysConfigSetDevice (
  IN  SYS_CONFIG_FUNCTION   Function,
  IN  UINT32                Device,
  IN  UINT32                Value
  )
{
  EFI_STATUS Status;

  Status = EFI_SUCCESS;

  // Intercept some functions
  switch(Function) {

  default:
    Status = EFI_UNSUPPORTED;
  }

  return Status;
}
