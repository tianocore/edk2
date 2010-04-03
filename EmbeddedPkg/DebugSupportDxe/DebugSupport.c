/** @file

  Copyright (c) 2008-2009, Apple Inc. All rights reserved.
  
  All rights reserved. This program and the accompanying materials
  are licensed and made available under the terms and conditions of the BSD License
  which accompanies this distribution.  The full text of the license may be found at
  http://opensource.org/licenses/bsd-license.php

  THE PROGRAM IS DISTRIBUTED UNDER THE BSD LICENSE ON AN "AS IS" BASIS,
  WITHOUT WARRANTIES OR REPRESENTATIONS OF ANY KIND, EITHER EXPRESS OR IMPLIED.

**/

#include <Uefi.h>

#include <Library/CacheMaintenanceLib.h>
#include <Library/DebugLib.h>
#include <Library/UefiBootServicesTableLib.h>

#include <Protocol/Cpu.h>
#include <Protocol/DebugSupport.h>
#include <Protocol/TimerDebugSupport.h>

EFI_STATUS
EFIAPI
DebugSupportGetMaximumProcessorIndex (
  IN  EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  OUT UINTN                       *MaxProcessorIndex
  )
{
  if (MaxProcessorIndex == NULL) {
    return EFI_INVALID_PARAMETER;
  }

  *MaxProcessorIndex = 0;

  return EFI_SUCCESS;
}

EFI_STATUS
EFIAPI
DebugSupportRegisterPeriodicCallback (
  IN  EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN  UINTN                       ProcessorIndex,
  IN  EFI_PERIODIC_CALLBACK       PeriodicCallback
  )
{
  TIMER_DEBUG_SUPPORT_PROTOCOL  *Timer;
  EFI_STATUS                    Status;

  Status = gBS->LocateProtocol(&gTimerDebugSupportProtocolGuid, NULL, (VOID **)&Timer);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = Timer->RegisterPeriodicCallback(Timer, PeriodicCallback);

  return Status;
}

EFI_STATUS
EFIAPI
DebugSupportRegisterExceptionCallback (
  IN  EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN  UINTN                       ProcessorIndex,
  IN  EFI_EXCEPTION_CALLBACK      ExceptionCallback,
  IN  EFI_EXCEPTION_TYPE          ExceptionType
  )
{
  EFI_CPU_ARCH_PROTOCOL *Cpu;
  EFI_STATUS            Status;

  Status = gBS->LocateProtocol(&gEfiCpuArchProtocolGuid, NULL, (VOID **)&Cpu);
  if (EFI_ERROR(Status)) {
    return Status;
  }

  Status = Cpu->RegisterInterruptHandler(Cpu, ExceptionType, (EFI_CPU_INTERRUPT_HANDLER)ExceptionCallback);

  return Status;
}

EFI_STATUS
EFIAPI
DebugSupportInvalidateInstructionCache (
  IN  EFI_DEBUG_SUPPORT_PROTOCOL  *This,
  IN  UINTN                       ProcessorIndex,
  IN  VOID                        *Start,
  IN  UINT64                      Length
  )
{
  InvalidateInstructionCacheRange(Start, Length);
  return EFI_SUCCESS;
}

EFI_DEBUG_SUPPORT_PROTOCOL  mDebugSupport = {
  IsaArm,
  DebugSupportGetMaximumProcessorIndex,
  DebugSupportRegisterPeriodicCallback,
  DebugSupportRegisterExceptionCallback,
  DebugSupportInvalidateInstructionCache
};

EFI_STATUS
DebugSupportDxeInitialize (
  IN EFI_HANDLE         ImageHandle,
  IN EFI_SYSTEM_TABLE   *SystemTable
  )
{
  EFI_STATUS  Status;
  EFI_HANDLE  Handle = NULL;

  ASSERT_PROTOCOL_ALREADY_INSTALLED (NULL, &gEfiDebugSupportProtocolGuid);
  Status = gBS->InstallMultipleProtocolInterfaces(&Handle, &gEfiDebugSupportProtocolGuid, &mDebugSupport, NULL);

  return Status;
}

