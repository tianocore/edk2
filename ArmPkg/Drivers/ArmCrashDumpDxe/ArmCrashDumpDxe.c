/** @file

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>

  This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DefaultExceptionHandlerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Cpu.h>

STATIC EFI_CPU_ARCH_PROTOCOL      *mCpu;

EFI_STATUS
EFIAPI
ArmCrashDumpDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS      Status;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  ASSERT_EFI_ERROR(Status);

  return mCpu->RegisterInterruptHandler (mCpu,
                                         EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS,
                                         &DefaultExceptionHandler);
}
