/** @file

  Copyright (c) 2017, Linaro, Ltd. All rights reserved.<BR>

  SPDX-License-Identifier: BSD-2-Clause-Patent

**/

#include <PiDxe.h>
#include <Library/DebugLib.h>
#include <Library/DefaultExceptionHandlerLib.h>
#include <Library/UefiBootServicesTableLib.h>
#include <Protocol/Cpu.h>

STATIC EFI_CPU_ARCH_PROTOCOL  *mCpu;

EFI_STATUS
EFIAPI
ArmCrashDumpDxeInitialize (
  IN EFI_HANDLE        ImageHandle,
  IN EFI_SYSTEM_TABLE  *SystemTable
  )
{
  EFI_STATUS  Status;

  Status = gBS->LocateProtocol (&gEfiCpuArchProtocolGuid, NULL, (VOID **)&mCpu);
  ASSERT_EFI_ERROR (Status);

  return mCpu->RegisterInterruptHandler (
                 mCpu,
                 EXCEPT_AARCH64_SYNCHRONOUS_EXCEPTIONS,
                 &DefaultExceptionHandler
                 );
}
