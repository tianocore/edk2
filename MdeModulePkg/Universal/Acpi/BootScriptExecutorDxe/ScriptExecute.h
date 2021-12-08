/** @file
  The header file for Boot Script Executer module.

  This driver is dispatched by Dxe core and the driver will reload itself to ACPI reserved memory
  in the entry point. The functionality is to interpret and restore the S3 boot script

Copyright (c) 2006 - 2018, Intel Corporation. All rights reserved.<BR>
Copyright (c) 2017, AMD Incorporated. All rights reserved.<BR>

SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#ifndef _BOOT_SCRIPT_EXECUTOR_H_
#define _BOOT_SCRIPT_EXECUTOR_H_

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
#include <Library/CpuExceptionHandlerLib.h>
#include <Library/DevicePathLib.h>
#include <Library/DxeServicesTableLib.h>

#include <Guid/AcpiS3Context.h>
#include <Guid/BootScriptExecutorVariable.h>
#include <Guid/MemoryProfile.h>

#include <Protocol/DxeSmmReadyToLock.h>
#include <IndustryStandard/Acpi.h>

#define PAGING_1G_ADDRESS_MASK_64  0x000FFFFFC0000000ull

/**
  a ASM function to transfer control to OS.

  @param  S3WakingVector  The S3 waking up vector saved in ACPI Facs table
  @param  AcpiLowMemoryBase a buffer under 1M which could be used during the transfer
**/
VOID
AsmTransferControl (
  IN   UINT32  S3WakingVector,
  IN   UINT32  AcpiLowMemoryBase
  );

/**
  a 32bit ASM function to transfer control to OS.

  @param  S3WakingVector  The S3 waking up vector saved in ACPI Facs table
  @param  AcpiLowMemoryBase a buffer under 1M which could be used during the transfer
**/
VOID
AsmTransferControl32 (
  IN   UINT32  S3WakingVector,
  IN   UINT32  AcpiLowMemoryBase
  );

/**
  a 16bit ASM function to transfer control to OS.
**/
VOID
AsmTransferControl16 (
  VOID
  );

/**
  Set a IDT entry for interrupt vector 3 for debug purpose.

  @param  AcpiS3Context  a pointer to a structure of ACPI_S3_CONTEXT

**/
VOID
SetIdtEntry (
  IN ACPI_S3_CONTEXT  *AcpiS3Context
  );

extern UINT32   AsmFixAddress16;
extern UINT32   AsmJmpAddr32;
extern BOOLEAN  mPage1GSupport;
extern UINT64   mAddressEncMask;

#endif //_BOOT_SCRIPT_EXECUTOR_H_
