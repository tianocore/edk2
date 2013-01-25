/** @file
  Header file for Dxe Core Debug Agent Library instance.

  Copyright (c) 2010 - 2013, Intel Corporation. All rights reserved.<BR>
  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php.

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#ifndef _DXE_CORE_DEBUG_AGENT_LIB_H_
#define _DXE_CORE_DEBUG_AGENT_LIB_H_

#include <PiDxe.h>

#include <Protocol/SerialIo.h>
#include <Protocol/DevicePath.h>
#include <Protocol/PiPcd.h>

#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiLib.h>
#include <Library/DevicePathLib.h>
#include <Library/MemoryAllocationLib.h>

#include "DebugAgent.h"

/**
  Install EFI Serial IO protocol based on Debug Communication Library. 

**/
VOID
InstallSerialIo (
  VOID
  );
  
#endif
