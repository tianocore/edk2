/** @file
The header file for Boot Script Executer module.

This driver is dispatched by Dxe core and the driver will reload itself to ACPI NVS memory
in the entry point. The functionality is to interpret and restore the S3 boot script

Copyright (c) 2013-2015 Intel Corporation.

This program and the accompanying materials
are licensed and made available under the terms and conditions of the BSD License
which accompanies this distribution.  The full text of the license may be found at
http://opensource.org/licenses/bsd-license.php

THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/
#ifndef _SCRIPT_EXECUTE_H_
#define _SCRIPT_EXECUTE_H_

#include <PiDxe.h>

#include <Library/BaseLib.h>
#include <Library/UefiDriverEntryPoint.h>
#include <Library/BaseMemoryLib.h>
#include <Library/DebugLib.h>
#include <Library/S3BootScriptLib.h>
#include <Library/PeCoffLib.h>
#include <Library/DxeServicesLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Library/UefiRuntimeServicesTableLib.h>
#include <Library/PcdLib.h>
#include <Library/CacheMaintenanceLib.h>
#include <Library/TimerLib.h>
#include <Library/UefiLib.h>
#include <Library/DebugAgentLib.h>
#include <Library/LockBoxLib.h>
#include <Library/IntelQNCLib.h>
#include <Library/QNCAccessLib.h>

#include <Guid/AcpiS3Context.h>
#include <Guid/BootScriptExecutorVariable.h>
#include <Guid/EventGroup.h>
#include <IndustryStandard/Acpi.h>

/**
  a ASM function to transfer control to OS.

  @param  S3WakingVector  The S3 waking up vector saved in ACPI Facs table
  @param  AcpiLowMemoryBase a buffer under 1M which could be used during the transfer
**/
VOID
AsmTransferControl (
  IN   UINT32           S3WakingVector,
  IN   UINT32           AcpiLowMemoryBase
  );

VOID
SetIdtEntry (
  IN ACPI_S3_CONTEXT     *AcpiS3Context
  );

/**
  Platform specific mechanism to transfer control to 16bit OS waking vector

  @param[in] AcpiWakingVector    The 16bit OS waking vector
  @param[in] AcpiLowMemoryBase   A buffer under 1M which could be used during the transfer

**/
VOID
PlatformTransferControl16 (
  IN UINT32       AcpiWakingVector,
  IN UINT32       AcpiLowMemoryBase
  );

#endif //_SCRIPT_EXECUTE_H_
